# System Architecture

## Overview

ChimeraMultiFX relies on a distributed processing model to ensure that high-fidelity audio processing is never interrupted by UI rendering or network stacks.

### 1. Audio Processing Unit
- **Hardware:** Daisy Seed
- **Responsibility:** Running DSP algorithms, maintaining real-time audio constraints, reading/writing to SD card for samples or impulse responses, handling MIDI IN/OUT/THRU.
- **Interface:** UART communication with the ESP32 for receiving parameter updates.

### 2. Control Unit
- **Hardware:** ESP32
- **Responsibility:** The central brain of the pedal. It reads hardware inputs (8 footswitches, 8 rotary encoders), updates its internal state, sends DSP parameter changes to the Daisy Seed, and acts as a web server.
- **Networking:** Operates as a WiFi Access Point or Station to serve the Web UI to connected devices.

### 3. Front-End / UI
- **Hardware:** Raspberry Pi (e.g., Pi Zero 2 W or Pi 4) + 7" Touch Screen
- **Responsibility:** Providing a rich, responsive Graphical User Interface.
- **Operation:** Runs in a locked-down Kiosk mode, displaying the web interface hosted by the ESP32 over the local network.

### 4. Power Delivery
- **Hardware:** Custom Power Supply Board
- **Responsibility:** Generating clean, isolated power rails for analog audio, digital DSP (Daisy), MCU (ESP32), and high-current demands (Raspberry Pi + LCD).
