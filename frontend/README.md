# Frontend UI

This is the browser-based control surface for the pedal. It is built with Preact and Vite and is served by the ESP32 from LittleFS.

## Development

```bash
cd frontend
npm install
npm run dev
```

The Vite dev server is useful for UI work. Live DSP commands still depend on the ESP32 endpoints, so production testing should use the device URL.

## Build

```bash
cd frontend
npm run build
```

This writes the production bundle into [firmware/esp32/data](firmware/esp32/data), which the ESP32 serves as the web UI.

## Deploy

```bash
cd frontend
npm run build

cd ../firmware/esp32
pio run --target uploadfs
```

UI-only changes normally need only the filesystem upload step. If you changed the ESP32 firmware itself, run `pio run --target upload` as well.

## Structure

- [frontend/src/main.jsx](frontend/src/main.jsx) — app entry point
- [frontend/src/AppController.jsx](frontend/src/AppController.jsx) — main application state
- [frontend/src/components](frontend/src/components) — UI pieces such as lanes, knobs, and effect nodes
