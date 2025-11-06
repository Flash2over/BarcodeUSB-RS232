cmake_minimum_required(VERSION 3.13)
project(pico2-barcode-rs232 C CXX ASM)

# Tell Pico SDK which board we are using
set(PICO_BOARD pico2)

# Disable software spinlocks (RP2350 fix)
set(PICO_USE_SW_SPIN_LOCKS 0)

# TinyUSB: tell it we are on RP2350 (important!)
add_definitions(-DCFG_TUSB_MCU=OPT_MCU_RP2350)

# Import Pico SDK
if (NOT DEFINED PICO_SDK_PATH)
  message(FATAL_ERROR "Please set PICO_SDK_PATH to your pico-sdk path (RP2350)")
endif()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Init SDK *after* board + config definitions
pico_sdk_init()

add_executable(pico2-barcode-rs232 src/main.c)

target_compile_definitions(pico2-barcode-rs232 PRIVATE
  PICO_TINYUSB_HOST=1
  PICO_TINYUSB_DEVICE=0
)

target_link_libraries(pico2-barcode-rs232
  pico_stdlib
  tinyusb_host
  hardware_uart
  hardware_flash
)

pico_add_extra_outputs(pico2-barcode-rs232)
pico_enable_stdio_usb(pico2-barcode-rs232 0)
pico_enable_stdio_uart(pico2-barcode-rs232 0)
