# Control MCU Board (ESP32)

This board acts as the central router and user input reader.

## Requirements
- Host an ESP32 module (e.g., ESP32-WROOM-32).
- Daisy-chain interface (e.g., I2C expanders or shift registers) to read:
  - 8x Momentary Footswitches.
  - 8x Rotary Encoders.
- UART headers for communication with the `audio-daisy` board.
- WiFi antenna placement considerations (keep away from metal).
- Power input headers coming from the `power-supply` board.
