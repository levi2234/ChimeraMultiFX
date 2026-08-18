# ESP32 Header Mapping Validation

**Date:** 2026-08-19  
**Carrier:** `hardware/boards/daisyBoard.kicad_sch` and `daisyBoard.kicad_pcb`  
**Expected plug-in board:** 30-pin DOIT ESP32 DevKit V1-style board using ESP32-WROOM-32  
**Firmware target:** PlatformIO `esp32dev`

## Verdict

The two 1x15 socket rows match the common 30-pin DOIT ESP32 DevKit V1 pin order, dimensions, and opposing row orientation. The active SD and UART assignments are correct for ESP32-WROOM-32, and the duplicate breakout headers preserve the physical row order.

This is not a footprint for the bare 38-pad ESP32-WROOM-32 module or a 38-pin ESP32-DevKitC. Before assembly, confirm that the purchased development board is the 30-pin variant and that its printed end pins match `VIN/GND` and `3V3/GND` as listed below.

## Physical Geometry

- Header row spacing: 25.40 mm.
- Pin pitch: 2.54 mm.
- Fourteen pitch intervals per row: 35.56 mm.
- Power/USB end: J1 pin 1 `VIN` aligns with J2 pin 15 `3V3`.
- Antenna end: J1 pin 15 `EN` aligns with J2 pin 1 `GPIO23`.

That geometry and opposing numbering direction match the common 30-pin DevKit V1 layout.

## J1 Left Row

| J1 pin | Expected DevKit signal | Carrier net | Status |
|---:|---|---|---|
| 1 | VIN / regulated 5 V input | `VCC_ESP32` | Pass; supplied from +5 V through FB1 |
| 2 | GND | `GND` | Pass |
| 3 | GPIO13 | Breakout only | Pass |
| 4 | GPIO12 / MTDI | Breakout only | Pass; strapping pin, avoid an external high level at reset |
| 5 | GPIO14 | Breakout only | Pass |
| 6 | GPIO27 | Breakout only | Pass |
| 7 | GPIO26 | Breakout only | Pass |
| 8 | GPIO25 | Breakout only | Pass |
| 9 | GPIO33 | Breakout only | Pass |
| 10 | GPIO32 | Breakout only | Pass |
| 11 | GPIO35 | Breakout only | Pass; input-only, no internal pull resistors |
| 12 | GPIO34 | Breakout only | Pass; input-only, no internal pull resistors |
| 13 | GPIO39 / VN | Breakout only | Pass; input-only, no internal pull resistors |
| 14 | GPIO36 / VP | Breakout only | Pass; input-only, no internal pull resistors |
| 15 | EN | Breakout only | Pass |

## J2 Right Row

| J2 pin | Expected DevKit signal | Carrier net/use | Status |
|---:|---|---|---|
| 1 | GPIO23 | `/SD_MOSI` | Pass; standard VSPI MOSI |
| 2 | GPIO22 | Breakout only | Pass |
| 3 | GPIO1 / TX0 | Breakout only | Pass; used by USB serial during programming/logging |
| 4 | GPIO3 / RX0 | Breakout only | Pass; used by USB serial during programming |
| 5 | GPIO21 | Breakout only | Pass |
| 6 | GPIO19 | `/SD_MISO` | Pass; standard VSPI MISO |
| 7 | GPIO18 | `/SD_SCK` | Pass; standard VSPI clock |
| 8 | GPIO5 | `/SD_CS` | Pass; R26 provides a 47 kohm pull-up to `ESP-3.3V` |
| 9 | GPIO17 / TX2 | `/ESP_TO_DAISY` | Pass; firmware UART2 TX |
| 10 | GPIO16 / RX2 | `/DAISY_TO_ESP` | Pass; firmware UART2 RX |
| 11 | GPIO4 | Breakout only | Pass |
| 12 | GPIO2 | Breakout only | Pass; strapping pin |
| 13 | GPIO15 / MTDO | Breakout only | Pass; strapping pin |
| 14 | GND | `GND` | Pass |
| 15 | 3V3 | `ESP-3.3V` and ESP SD-card supply | Pass, subject to the exact DevKit regulator current rating |

## Breakout Headers

J3 and J4 intentionally reverse connector numbering so their physical top-to-bottom order mirrors the corresponding socket rows:

- J3 pin 1 through pin 15 connects to J2 pin 15 through pin 1.
- J4 pin 1 through pin 15 connects to J1 pin 15 through pin 1.

This is electrically correct, but assembly and debug documentation should show signal names because equal pin numbers do not connect between the socket and breakout headers.

## Peripheral Validation

### ESP32 SD card

The SD card uses the conventional VSPI assignment:

```text
GPIO23 -> MOSI
GPIO19 <- MISO
GPIO18 -> SCK
GPIO5  -> CS
```

Card2 is correctly wired in SPI mode: DAT3/CS, CMD/MOSI, CLK/SCK, DAT0/MISO, 3.3 V, and GND. DAT1 and DAT2 are unconnected as expected for SPI mode.

GPIO5 is a strapping pin sampled during reset. The SD card's CS pin should be high or high-impedance while the ESP32 boots. Existing `R26 = 47 kohm` pulls `/SD_CS` up to `ESP-3.3V`, keeping the card deselected and reinforcing the normal GPIO5 boot state. No additional pull-up is required.

### Daisy UART

Firmware uses UART2 with RX=GPIO16 and TX=GPIO17:

```text
ESP32 GPIO17 TX -> /ESP_TO_DAISY -> Daisy RX
ESP32 GPIO16 RX <- /DAISY_TO_ESP <- Daisy TX
```

This agrees with `DaisyBridge::DefaultRxPin = 16` and `DefaultTxPin = 17`. GPIO16 and GPIO17 are available on ESP32-WROOM-32. They may be occupied by PSRAM on ESP32-WROVER variants, so do not substitute a WROVER board without rechecking.

## Pin Restrictions

- GPIO34, GPIO35, GPIO36, and GPIO39 are input-only and have no software-controlled pull-up or pull-down.
- GPIO2, GPIO5, GPIO12, and GPIO15 are boot-strapping pins. External circuits must not force an invalid level during reset.
- GPIO1 and GPIO3 are UART0 and are used by USB serial for flashing and logs.
- GPIO6 through GPIO11 are connected to module flash and are correctly absent from these headers.
- ADC2 pins cannot be used for ADC measurements while Wi-Fi is active. Use ADC1 pins GPIO32-39 for future analog controls.

## Remaining Verification

The carrier does not identify an exact development-board manufacturer or MPN. `ESP32-WROOM-32` identifies the RF module, not the surrounding 30-pin USB/regulator board. Before ordering or soldering sockets:

1. Confirm the board has exactly 15 pins per side.
2. With USB at the power end, confirm one row begins `VIN, GND, GPIO13` and the other end aligns with `3V3, GND, GPIO15` according to the table.
3. Confirm 25.40 mm row spacing and 35.56 mm pin-span length.
4. Confirm the board uses ESP32-WROOM-32 rather than ESP32-WROVER, ESP32-S2, ESP32-S3, or ESP32-C3.
5. Add the exact board MPN and a body/antenna footprint with the vendor-defined keepout.

## Result

**Electrical mapping:** Pass for the common 30-pin DOIT ESP32 DevKit V1-style ESP32-WROOM-32 board.  
**Firmware mapping:** Pass.  
**Mechanical interchangeability:** Conditional on confirming the exact 30-pin board variant.  
**Boot-strapping support:** Pass; GPIO5 `/SD_CS` already has the required pull-up through R26.