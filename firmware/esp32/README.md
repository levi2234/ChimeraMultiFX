# ESP32 control firmware

This firmware runs on the ESP32 and provides the pedal's control surface, Wi-Fi bridge, and browser UI host.

## What it does

- Connects to Wi-Fi and exposes a small HTTP/WebSocket API
- Bridges serial commands to the Daisy Seed over UART
- Serves the frontend from LittleFS so the UI can run without Node.js on the device

## Setup

1. Copy [firmware/esp32/src/wifi_credentials.example.h](firmware/esp32/src/wifi_credentials.example.h) to [firmware/esp32/src/wifi_credentials.h](firmware/esp32/src/wifi_credentials.h).
2. Add your local 2.4 GHz Wi-Fi credentials to the new file.
3. Install PlatformIO.

## Build and upload

```bash
cd frontend
npm install
npm run build

cd ../firmware/esp32
pio run
pio run --target upload
pio run --target uploadfs
```

### Upload steps

1. Connect the ESP32 board over USB.
2. Build the frontend assets if you changed the UI:
   ```bash
   cd frontend
   npm install
   npm run build
   ```
3. Upload the firmware:
   ```bash
   cd ../firmware/esp32
   pio run --target upload
   ```
4. Upload the web UI files to LittleFS:
   ```bash
   pio run --target uploadfs
   ```
5. Open the serial monitor if you want to confirm startup:
   ```bash
   pio run --target monitor
   ```

- `upload` flashes the ESP32 firmware.
- `uploadfs` updates the web UI assets in LittleFS.
- `pio run --target buildfs` builds the filesystem image without uploading it.

## Useful endpoints

- `GET /health`
- `GET /api/daisy/command?cmd=ping`
- `GET /api/bridge/selftest`

After boot, the board prints its IP address to the serial monitor. Use that address for the UI and API.

## Quick troubleshooting

If the Daisy bridge times out, run the protocol test from [firmware/daisy/Utils/bridge_protocol_test.py](firmware/daisy/Utils/bridge_protocol_test.py) and verify the UART wiring and ground connection between the two MCUs.
