# Barcode → RS232 (TX-only) using Raspberry Pi Pico 2 (RP2350)

Wireless-Scanner (USB HID Keyboard Dongle) → **Pico 2 (USB Host)** → **MAX3232** → **RS232 (TX-only)**

- **TX-only:** Es wird nur gesendet (GP0 → MAX3232 → RS232 RX). RX wird nicht benutzt.
- **Konfiguration per Barcodes** – ganz ohne PC:
  - `###CONFIG###` (Config-Modus starten)
  - `CFG:BAUD=<rate>`: 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 250000, 500000, 1000000
  - `CFG:END=<mode>`: `CR`, `LF`, `CRLF`, `NONE`, `ETX`, `STXETX`
  - `CFG:SAVE` (persistente Speicherung im Flash)
- **Default:** 115200 8N1, Zeilenende = CR

## Verdrahtung (TX-only)

```
USB-Dongle → Pico 2 (USB Host)

Pico 2 UART0:
  GP0 (TX) → MAX3232 T1IN → T1OUT → RS232 RX (Zielgerät)
  3V3 → MAX3232 VCC
  GND ↔ MAX3232 GND ↔ Zielgerät GND

Hinweis: GP1 (RX), R1IN/R1OUT am MAX3232 unbeschaltet.
```

## Build (lokal, Pico SDK)

```bash
# vorausgesetzt: PICO_SDK_PATH zeigt auf pico-sdk (mit RP2350-Unterstützung)
mkdir build && cd build
cmake .. -DPICO_BOARD=pico2
make -j
# ergebnis: pico2-barcode-rs232.uf2
```

## Build per Docker (ohne lokale Toolchain)
```bash
# im Repo-Root
docker build -t pico-fw .
docker run --rm -v "$PWD:/work" pico-fw /bin/bash -lc "cd build && cmake .. -DPICO_BOARD=pico2 && make -j"
```

## Flashen
- BOOTSEL gedrückt halten und Pico 2 anstecken → Laufwerk **RPI-RP2**
- `build/pico2-barcode-rs232.uf2` auf das Laufwerk kopieren

## Nutzung
- Normalbetrieb: Barcode scannen → Text wird an RS232 (TX) gesendet, Abschluss gemäß Einstellung.
- Konfiguration: `###CONFIG###` scannen → dann `CFG:BAUD=...`, `CFG:END=...` → `CFG:SAVE`.
- A4-Konfigkarten (PNG & PDF) liegen in **assets/**

## Hinweis zu „Full (mit .uf2)”
In dieser Umgebung kann kein Cross-Compile gegen das Pico SDK ausgeführt werden, daher ist **keine bereits gebaute `.uf2`** beigelegt.  
Dafür ist ein **Dockerfile** enthalten, mit dem du die `.uf2` **reproduzierbar** bauen kannst – ohne lokale Installation von Toolchains.
