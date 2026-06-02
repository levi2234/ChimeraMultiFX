# Web UI

The graphical user interface for the ChimeraMultiFX.

## Architecture
- This is a Single Page Application (SPA) or optimized static site.
- It communicates with the ESP32 via REST APIs or WebSockets.
- The build artifacts (HTML/CSS/JS) are compressed and uploaded to the ESP32's SPIFFS or LittleFS storage.

## Development
- You can develop this UI locally by mocking the ESP32 endpoints.
- Ensure the design is responsive, but optimized for the 7" Touchscreen resolution used on the Raspberry Pi.
