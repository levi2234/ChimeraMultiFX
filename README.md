# ChimeraMultiFX

A high-end, multi-effects guitar pedal built for professional DSP performance and seamless usability. The system integrates advanced audio processing on a dedicated DSP chip, robust user control via an ESP32, and a modern touchscreen interface driven by a Raspberry Pi.

## High-Level Architecture

The project is split into three main computation units to ensure stability and high audio fidelity:

- **Audio DSP (Daisy Seed):** Handles 100% of the stereo audio processing, MIDI I/O, and SD Card storage.
- **Control MCU (ESP32):** Manages the hardware interface (8 momentary footswitches, 8 rotary encoders), orchestrates parameter changes via UART to the Daisy Seed, and hosts the web application.
- **Frontend / UI (Raspberry Pi):** Boots into a kiosk-mode web browser to display the GUI served by the ESP32.

## Project Structure

The repository is modularly structured, separating firmware, hardware, software, and general documentation.

```text
ChimeraMultiFX/
├── docs/       # Overall system architecture, block diagrams, specs
├── firmware/   # C++ / Embedded projects
│   ├── audio-dsp/      # Daisy Seed firmware
│   └── control-mcu/    # ESP32 firmware
├── hardware/   # Hardware design files (PCBs, schematics, mechanical)
│   ├── boards/         # Stackable modular boards (EDA files and PDF schematics)
│   │   ├── audio-daisy/
│   │   ├── control-esp32/
│   │   ├── frontend-rpi/
│   │   └── power-supply/
│   └── mechanical/     # 3D models and enclosures
└── software/   # High-level software and UI
    ├── rpi-kiosk/      # Scripts for RPi kiosk mode setup
    └── web-ui/         # The web application hosted on the ESP32
```

## Getting Started

Please refer to the `README.md` files within each specific sub-folder for detailed instructions on building firmware, editing hardware, or running the frontend software.
