# BarcodeUSB-RS232
Wireless Barcode dongle → Raspberry Pi Pico 2 (USB Host) → RS232 machine interface

![Build](https://github.com/Flash2over/BarcodeUSB-RS232/actions/workflows/build.yml/badge.svg)

This project bridges **wireless barcode scanner USB dongles** (HID keyboard mode) to **industrial machines**
that require **RS232 input**.  
The **Raspberry Pi Pico 2** acts as a **USB Host**, decodes HID key events, and outputs them via
**UART0 → MAX3232 → RS232** at selectable baud rates and line endings.

**All configuration is done by scanning special barcodes — no PC required.**

---

## Features

| Feature | Status |
|--------|:------:|
| USB HID Barcode dongle support | ✅ |
| RS232 output via MAX3232 | ✅ |
| Safe 3.3V UART levels | ✅ |
| **Configuration via barcode (no PC, no terminal)** | ✅ |
| Persistent configuration in Flash | ✅ |
| Baudrates supported | 1200 → 1,000,000 |
| Line End options | CR, LF, CRLF, NONE, ETX, STXETX |
| Test/Debug Scan Mode | ✅ |

---

## Hardware Wiring

USB Barcode Dongle → Pico 2 USB Port (Pico operates in USB Host mode)

Pico 2 UART0:
- **GP0 (TX)** → **MAX3232 T1IN** → **T1OUT → RS232 RX** (machine)
- **GP1 (RX)** ← **MAX3232 R1OUT** ← **R1IN ← RS232 TX** (optional)
- **3V3** → **MAX3232 VCC**
- **GND** connected everywhere (Pico ↔ MAX3232 ↔ Machine)

Important notes:
- **Pico GPIO = 3.3V only**
- **Power the MAX3232 from 3.3V**, not 5V
- Pico may be powered via **USB** or **VSYS = 5V**

---

## Configuration via Barcode (no PC)

Enter configuration mode by scanning:
```
###CONFIG###
```

Set baudrate:
```
CFG:BAUD=115200
CFG:BAUD=9600
...
```

Set line ending:
```
CFG:END=CR
CFG:END=LF
CFG:END=CRLF
CFG:END=NONE
CFG:END=ETX
CFG:END=STXETX
```

Save configuration:
```
CFG:SAVE
```

Changes are stored in Flash and applied immediately.

---

## Building (optional — CI already builds automatically)

```bash
git clone https://github.com/Flash2over/BarcodeUSB-RS232
cd BarcodeUSB-RS232
mkdir build && cd build
cmake .. -DPICO_BOARD=pico2
make -j
```

Flash firmware:
- Hold **BOOTSEL**
- Plug in Pico via USB
- Copy `.uf2` to the **RPI-RP2** drive

---

## CI Build Output

Latest firmware is always available at:

```
GitHub → Actions → Latest Workflow Run → Artifacts → pico2-barcode-rs232.uf2
```

No toolchain required on your computer.

---

## Troubleshooting

| Issue | Cause | Solution |
|-------|-------|----------|
| No barcode data | USB dongle may require powered hub | Try powered USB hub |
| Garbled serial output | Wrong baudrate | Change via barcode |
| Extra / missing newlines | Wrong line ending | `CFG:END=...` |
| Pico not responding | 5V applied to GPIO | Use **only VSYS/USB** for 5V |

---

## 📜 License
MIT License — free to use with attribution.
Author: Thomas Störzner (Flash2over)
Date: November 2025
