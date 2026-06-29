# Control MCU Firmware (ESP32)

Firmware for the ESP32.

## Responsibilities
- Reading hardware inputs: 8 footswitches, 8 rotary encoders (via I2C/SPI expanders).
- Hosting a Web Server to serve the `software/web-ui` application.
- Acting as a WiFi Access Point or connecting to an existing network.
- Exposing a REST API or WebSockets for the UI to interact with.
- Sending state changes over UART to the Daisy Seed.

## Build Instructions
(Note: Replace with actual build system commands, e.g., PlatformIO)
```bash
pio run --target upload
```
