# Power Supply Board

This board provides stable, noise-free power to the entire stack.

## Requirements
- Input: 9V DC (standard center-negative pedal power) barrel jack or 12V DC.
- Output 1: Clean, isolated power for the Audio DSP (Daisy Seed) (takes anywhere from 3.3-5V DC).
- Output 2: Clean, isolated power for the Control MCU (ESP32) (takes 3.3V DC).
- Output 3: Clean, isolated power for the Frontend (Raspberry Pi + LCD) (takes 5V DC for normal pi's but rpi zero takes 3.3V DC).
- Filtering: Must aggressively filter out noise to ensure studio-grade audio quality.!
- Must include reverse polarity protection.
- Must include a led indicating that the power supply is on.
- Must include a button for on/off.
- Must supply 9V DC 2A (max current draw for all components combined is around 1A, leaving some overhead for expansion)
- Must supply 5V DC 1A for pi + lcd (rpi pico takes 3.3V DC)
- Must supply 3.3V DC 1A for esp32
- Must supply 3.3V DC 1A for daisy
- Must supply 9V, 5V and 3.3V DC to the other boards for their own power regulation.


