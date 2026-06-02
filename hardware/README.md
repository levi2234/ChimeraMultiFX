# Hardware Development

This directory contains all Electronic Design Automation (EDA) files, schematics, Bill of Materials (BOMs), and mechanical designs (enclosures, 3D models) for the ChimeraMultiFX pedal.

## Modular Board System

The hardware is designed to be modular and stackable:
- **`boards/audio-daisy/`**: The main audio DSP board housing the Daisy Seed.
- **`boards/control-esp32/`**: The controller board housing the ESP32 and inputs.
- **`boards/frontend-rpi/`**: The display driver board for the Raspberry Pi and 7" touchscreen.
- **`boards/power-supply/`**: Clean power delivery and distribution for the stack.

## Structure

- **`boards/`**: Contains the EDA project files (e.g., KiCad/Altium) as well as the exported PDF schematics for each specific PCB. Keeping the schematics in their respective board folders ensures that the design files and their documentation are tightly coupled.
- **`mechanical/`**: Contains STEP files, 3D models for the enclosure, and faceplate designs.
