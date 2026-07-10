# Control MCU Firmware (ESP32)

Minimal ESP32 control surface for ChimeraMultiFX.

This firmware starts a WiFi access point and exposes only a small health-check
HTTP surface. The previous ESP32-to-Daisy serial bridge protocol has been
removed so the connection between the ESP32 and Daisy Seed can be redesigned
from a clean baseline.

The Daisy Seed serial handler remains in the Daisy firmware and is intentionally
not duplicated or defined here.

## WiFi

The ESP32 creates an access point:

- SSID: `ChimeraMultiFX`
- Password: `chimerafx`
- Default URL: `http://192.168.4.1/`

## Endpoints

- `GET /health` -> ESP32 health check

## Build and upload

```bash
cd firmware/esp32
pio run
pio run --target upload
```
