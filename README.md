# My Embedded Synth Project

## Overview
Multi-platform embedded synthesizer project using ESP32, Daisy Seed, and Raspberry Pi Zero.

## Hardware

### Wiring Diagrams
TODO: Add wiring diagrams

### Power Specifications
TODO: Add power specs

### Bill of Materials (BOM)
| Component | Quantity | Notes |
|-----------|----------|-------|
| ESP32     | 1        |       |
| Daisy Seed| 1        |       |
| Pi Zero   | 1        |       |

## Building

### ESP32 Firmware
```bash
cd firmware-esp32
pio run
pio run --target upload
```

### Daisy Firmware
```bash
cd firmware-daisy
make
make program-dfu
```

### Pi Zero Software
```bash
cd software-pi-zero
pip install -r requirements.txt
```
