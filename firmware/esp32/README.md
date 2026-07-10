# Control MCU Firmware (ESP32)

Minimal ESP32 control surface for ChimeraMultiFX.

This firmware starts a WiFi access point and exposes a small HTTP surface for
health checks and Daisy Seed command forwarding.

The Daisy Seed serial handler remains in the Daisy firmware. The ESP32 forwards
newline-terminated ASCII commands to the Daisy over UART2 and returns the first
newline-terminated Daisy response.

## WiFi

The ESP32 creates an access point:

- SSID: `ChimeraMultiFX`
- Password: `chimerafx`
- Default URL: `http://192.168.4.1/`

## Endpoints

- `GET /health` -> ESP32 health check
- `GET /api/bridge/pins` -> current ESP32 UART bridge RX/TX pin configuration
- `GET /api/uart2/loopback` -> test ESP32 UART2 RX/TX with GPIO17 jumpered to GPIO16
- `GET /api/bridge/selftest` -> send `ping` to the Daisy and expect a `PONG` response
- `GET /api/daisy/command?cmd=ping` -> send `ping` to the Daisy and return its response

## Daisy UART Bridge

- Baud rate: `115200`
- ESP32 GPIO16 / UART2 RX <- Daisy Seed `D13` / `PB6` / USART1 TX
- ESP32 GPIO17 / UART2 TX -> Daisy Seed `D14` / `PB7` / USART1 RX
- ESP32 GND <-> Daisy GND

Examples:

```text
http://192.168.4.1/api/daisy/command?cmd=ping
http://192.168.4.1/api/daisy/command?cmd=info
http://192.168.4.1/api/daisy/command?cmd=status
```

The endpoint returns `400` for a missing or empty `cmd` parameter and `504` if
the Daisy does not return a line before the bridge timeout.

If `/api/daisy/command?cmd=ping` returns `daisy_timeout`:

1. Disconnect the Daisy from ESP32 UART.
2. Jumper ESP32 GPIO17 to GPIO16.
3. Open `http://192.168.4.1/api/uart2/loopback` and expect `OK uart2 loopback`.
4. Remove the jumper, reconnect Daisy TX/RX/GND, and open `http://192.168.4.1/api/bridge/selftest`.
5. If loopback passes but self-test times out, check Daisy firmware, Daisy power/boot state, UART wiring direction, and shared ground.

The diagnostic endpoints also accept runtime pin overrides:

```text
http://192.168.4.1/api/bridge/pins
http://192.168.4.1/api/uart2/loopback?rx=16&tx=17
http://192.168.4.1/api/uart2/loopback?rx=3&tx=1
http://192.168.4.1/api/bridge/selftest?rx=16&tx=17
```

For a loopback test, jumper the selected TX pin to the selected RX pin. If
`rx=16&tx=17` fails but another pair passes, update `DaisyUartRxPin` and
`DaisyUartTxPin` in `src/main.cpp` to match the actual board wiring.

## Build and upload

```bash
cd firmware/esp32
pio run
pio run --target upload
```
