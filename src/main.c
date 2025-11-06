
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/flash.h"
#include "tusb.h"
#include "class/hid/hid.h"

#define UART_PORT   uart0
#define UART_TX_PIN 0   // TX-only

#ifndef FLASH_TARGET_OFFSET
#define FLASH_TARGET_OFFSET (2 * 1024 * 1024 - 4096) // last 4KB block in 2MB
#endif

#define CFG_MAGIC 0xBADC0DEu

typedef enum {
  END_CR=0, END_LF, END_CRLF, END_NONE, END_ETX, END_STXETX,
} line_end_t;

typedef struct {
  uint32_t magic;
  uint32_t baud;
  uint8_t  end;
  uint8_t  reserved[55];
  uint32_t crc;
} cfg_t;

static cfg_t cfg;

// ---------------- CRC32 ----------------
static uint32_t crc32(const uint8_t* data, size_t len){
  uint32_t crc=0xFFFFFFFFu;
  for(size_t i=0;i<len;i++){
    crc ^= data[i];
    for(int j=0;j<8;j++){
      uint32_t mask = -(crc & 1u);
      crc = (crc >> 1) ^ (0xEDB88320u & mask);
    }
  }
  return ~crc;
}

// ---------------- Config persistence ----------------
static void cfg_defaults(void){
  cfg.magic = CFG_MAGIC;
  cfg.baud  = 115200;
  cfg.end   = END_CR;
  cfg.crc   = crc32((uint8_t*)&cfg, sizeof(cfg)-4);
}

static void cfg_load(void){
  memcpy(&cfg, (const void*)(XIP_BASE + FLASH_TARGET_OFFSET), sizeof(cfg));
  uint32_t check = crc32((uint8_t*)&cfg, sizeof(cfg)-4);
  if (cfg.magic != CFG_MAGIC || check != cfg.crc){
    cfg_defaults();
  }
}

static void cfg_save(void){
  cfg.crc = crc32((uint8_t*)&cfg, sizeof(cfg)-4);
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t*)&cfg, FLASH_PAGE_SIZE);
  restore_interrupts(ints);
}

// ---------------- HID Keymap (US) ----------------
static char nm[128]={0}, sm[128]={0};

static void keymap_init(void){
  nm[HID_KEY_SPACE]=' ';
  nm[HID_KEY_MINUS]='-'; sm[HID_KEY_MINUS]='_';
  nm[HID_KEY_EQUAL]='='; sm[HID_KEY_EQUAL]='+';
  nm[HID_KEY_BRACKET_LEFT]='['; sm[HID_KEY_BRACKET_LEFT]='{';
  nm[HID_KEY_BRACKET_RIGHT]=']'; sm[HID_KEY_BRACKET_RIGHT]='}';
  nm[HID_KEY_BACKSLASH]='\\'; sm[HID_KEY_BACKSLASH]='|';
  nm[HID_KEY_SEMICOLON]=';'; sm[HID_KEY_SEMICOLON]=':';
  nm[HID_KEY_APOSTROPHE]='\''; sm[HID_KEY_APOSTROPHE]='"';
  nm[HID_KEY_GRAVE]='`'; sm[HID_KEY_GRAVE]='~';
  nm[HID_KEY_COMMA]=','; sm[HID_KEY_COMMA]='<';
  nm[HID_KEY_PERIOD]='.'; sm[HID_KEY_PERIOD]='>';
  nm[HID_KEY_SLASH]='/'; sm[HID_KEY_SLASH]='?';

  nm[HID_KEY_1]='1'; sm[HID_KEY_1]='!';
  nm[HID_KEY_2]='2'; sm[HID_KEY_2]='@';
  nm[HID_KEY_3]='3'; sm[HID_KEY_3]='#';
  nm[HID_KEY_4]='4'; sm[HID_KEY_4]='$';
  nm[HID_KEY_5]='5'; sm[HID_KEY_5]='%';
  nm[HID_KEY_6]='6'; sm[HID_KEY_6]='^';
  nm[HID_KEY_7]='7'; sm[HID_KEY_7]='&';
  nm[HID_KEY_8]='8'; sm[HID_KEY_8]='*';
  nm[HID_KEY_9]='9'; sm[HID_KEY_9]='(';
  nm[HID_KEY_0]='0'; sm[HID_KEY_0]=')';

  for(int k=HID_KEY_A;k<=HID_KEY_Z;k++){
    nm[k] = 'a' + (k - HID_KEY_A);
    sm[k] = 'A' + (k - HID_KEY_A);
  }
}

static inline bool is_shift(uint8_t m){
  return m & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
}

// ---------------- Scan buffer ----------------
#define BUFSZ 512
static char scan[BUFSZ];
static int slen = 0;
static bool cfg_mode = false;

static void send_line_end(void){
  switch((line_end_t)cfg.end){
    case END_CR:    uart_putc_raw(UART_PORT,'\r'); break;
    case END_LF:    uart_putc_raw(UART_PORT,'\n'); break;
    case END_CRLF:  uart_putc_raw(UART_PORT,'\r'); uart_putc_raw(UART_PORT,'\n'); break;
    case END_ETX:   uart_putc_raw(UART_PORT,0x03); break;
    case END_STXETX: uart_putc_raw(UART_PORT,0x03); break; // ETX only at end
    default: break;
  }
}

static void send_line_start(void){
  if ((line_end_t)cfg.end == END_STXETX){
    uart_putc_raw(UART_PORT, 0x02); // STX
  }
}

static void process_complete_scan(void){
  scan[slen] = 0;

  if (!cfg_mode){
    if (strcmp(scan, "###CONFIG###") == 0){
      cfg_mode = true;
      slen = 0;
      return;
    }
    // Normal output
    send_line_start();
    for (int i=0;i<slen;i++) uart_putc_raw(UART_PORT, (uint8_t)scan[i]);
    send_line_end();
    slen = 0;
    return;
  }

  // In config mode
  if (strncmp(scan, "CFG:", 4) == 0){
    const char* cmd = scan + 4;

    if (strncmp(cmd, "BAUD=", 5) == 0){
      uint32_t b = (uint32_t)atoi(cmd + 5);
      switch(b){
        case 1200: case 2400: case 4800: case 9600: case 19200:
        case 38400: case 57600: case 115200: case 230400:
        case 460800: case 250000: case 500000: case 1000000:
          cfg.baud = b;
          uart_init(UART_PORT, cfg.baud);
          gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
          break;
        default: break;
      }
    } else if (strncmp(cmd, "END=", 4) == 0){
      const char* v = cmd + 4;
      if      (!strcmp(v,"CR"))     cfg.end = END_CR;
      else if (!strcmp(v,"LF"))     cfg.end = END_LF;
      else if (!strcmp(v,"CRLF"))   cfg.end = END_CRLF;
      else if (!strcmp(v,"NONE"))   cfg.end = END_NONE;
      else if (!strcmp(v,"ETX"))    cfg.end = END_ETX;
      else if (!strcmp(v,"STXETX")) cfg.end = END_STXETX;
    } else if (!strcmp(cmd,"SAVE")){
      cfg_save();
      cfg_mode = false;
    }
  }

  slen = 0;
}

// ---------------- TinyUSB Host callbacks ----------------
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len){
  (void)desc_report; (void)desc_len;
  tuh_hid_receive_report(dev_addr, instance);
}
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance){ (void)dev_addr; (void)instance; }
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len){
  (void)dev_addr; (void)instance;
  if (len < sizeof(hid_keyboard_report_t)){
    tuh_hid_receive_report(dev_addr, instance); return;
  }
  hid_keyboard_report_t const* k = (hid_keyboard_report_t const*)report;
  bool shift = is_shift(k->modifier);
  // collect 6 keycodes
  for (int i=0;i<6;i++){
    uint8_t kc = k->keycode[i];
    if (!kc) continue;
    if (kc == HID_KEY_ENTER || kc == HID_KEY_KEYPAD_ENTER){
      process_complete_scan();
      continue;
    }
    char out = shift ? sm[kc] : nm[kc];
    if (out && slen < BUFSZ-1){
      scan[slen++] = out;
    }
  }
  tuh_hid_receive_report(dev_addr, instance);
}

int main(){
  cfg_load();
  uart_init(UART_PORT, cfg.baud);
  gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
  keymap_init();
  tusb_init();
  while (true){
    tuh_task();
  }
  return 0;
}
