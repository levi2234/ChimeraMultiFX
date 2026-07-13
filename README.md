# ChimeraMultiFX

ChimeraMultiFX is a dual-MCU effects pedal platform. The Daisy Seed handles audio DSP while the ESP32 manages controls, Wi-Fi, and the browser-based UI.

## Repository layout

- [firmware/daisy](firmware/daisy) — Daisy Seed firmware built with libDaisy and DaisySP
- [firmware/esp32](firmware/esp32) — ESP32 control firmware and LittleFS web UI host
- [frontend](frontend) — Preact/Vite touch interface source
- [hardware/boards](hardware/boards) — KiCad PCB and schematic files
- [docs](docs) — repository notes and debugging write-ups

## Quick start

1. Review [firmware/daisy/README.md](firmware/daisy/README.md) for DSP build and flash steps.
2. Review [firmware/esp32/README.md](firmware/esp32/README.md) for ESP32 build, upload, and Wi-Fi setup.
3. Review [frontend/README.md](frontend/README.md) for local UI development.
4. Open [hardware/boards/daisyBoard.kicad_pro](hardware/boards/daisyBoard.kicad_pro) when working on hardware.

## Uploading code

- Daisy Seed: build with the Daisy make targets, then enter DFU bootloader mode and run the flashing command from [firmware/daisy/README.md](firmware/daisy/README.md).
- ESP32: build and upload from PlatformIO with the commands in [firmware/esp32/README.md](firmware/esp32/README.md).
- Frontend UI: build the web assets and upload them to the ESP32 filesystem using the steps in [frontend/README.md](frontend/README.md).

## Notes

- The ESP32 serves the built frontend from its LittleFS data partition.
- The Daisy and ESP32 communicate over UART; the ESP32 exposes a small HTTP/WebSocket bridge for the UI.

## License

MIT — see [LICENSE](LICENSE).
