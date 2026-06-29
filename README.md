# ChimeraMultiFX

> A dual-MCU, open-hardware multi-effects guitar/bass pedal built around the **Daisy Seed** (DSP) and **ESP32** (control).

---

## Overview

ChimeraMultiFX is a modular, programmable effects pedal that runs high-quality audio DSP on an Electrosmith Daisy Seed while the ESP32 handles all user-facing controls — footswitches, encoders, and a browser-based configuration UI served over Wi-Fi.

### Key Features

- 🎸 **Real-time audio effects** — Delay, Distortion, and an extensible effect slot system
- 🎛️ **8 footswitches + 8 rotary encoders** via I²C/SPI expanders on the ESP32
- 📡 **Wi-Fi web UI** — ESP32 acts as an access point (or station) and hosts a configuration interface
- ⚡ **96 kHz / 32-bit** audio processing pipeline via libDaisy
- 🔧 **Custom KiCad PCB** — Daisy Seed carrier board with all I/O

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        ChimeraMultiFX                           │
│                                                                 │
│   Guitar IN ──► [ Daisy Seed (DSP) ] ──► Audio OUT             │
│                        ▲                                        │
│                        │ UART (state changes)                   │
│                        ▼                                        │
│              [ ESP32 (Control MCU) ]                            │
│                   │         │                                   │
│        Footswitches       Encoders                              │
│        (via expanders)    (via expanders)                       │
│                   │                                             │
│              [ Wi-Fi ]                                          │
│                   │                                             │
│          [ Browser Web UI ]                                     │
└─────────────────────────────────────────────────────────────────┘
```

See [`docs/architecture.md`](docs/architecture.md) for a detailed breakdown.

---

## Repository Layout

```
ChimeraMultiFX/
├── firmware/
│   ├── daisy/          # Daisy Seed DSP firmware (C++, libDaisy/DaisySP)
│   └── esp32/          # ESP32 control firmware (C++, PlatformIO/Arduino)
├── hardware/
│   ├── boards/         # KiCad PCB project (Daisy carrier board)
│   └── enclosure/      # Enclosure design files
├── docs/               # Architecture, hardware notes, effect specs
├── software/           # Web UI application (served by ESP32)
├── .gitignore
├── CHANGELOG.md
├── CONTRIBUTING.md
├── LICENSE
└── README.md           ← you are here
```

---

## Getting Started

### Prerequisites

| Tool | Purpose |
|---|---|
| `arm-none-eabi-gcc` | Daisy Seed cross-compiler |
| `dfu-util` | Daisy flash tool (USB DFU) |
| `make` | Daisy build system |
| PlatformIO CLI / IDE | ESP32 build & flash |
| KiCad 7+ | Open PCB design files |

---

### Building the Daisy Firmware

The Daisy firmware depends on **libDaisy** and **DaisySP** as local submodule copies inside `firmware/daisy/deps/`.

```bash
# 1. Initialize submodules (first time only)
git submodule update --init --recursive

# 2. Build libDaisy
cd firmware/daisy/deps/libDaisy
make

# 3. Build DaisySP
cd ../DaisySP
make

# 4. Build the main project
cd ../../
make
```

To flash over USB DFU, put the Daisy into bootloader mode (hold BOOT, tap RESET), then:

```bash
make program-dfu
```

See [`firmware/daisy/README.md`](firmware/daisy/README.md) for full details.

---

### Building the ESP32 Firmware

```bash
cd firmware/esp32
pio run --target upload
```

See [`firmware/esp32/README.md`](firmware/esp32/README.md) for full details.

---

## Hardware

The custom PCB is a Daisy Seed carrier board designed in KiCad. It provides:

- Audio I/O jacks and analog signal conditioning
- UART header connecting Daisy ↔ ESP32
- I²C/SPI header for input expanders
- Power regulation (9V DC jack → 3.3V/5V rails)

See [`docs/hardware.md`](docs/hardware.md) and [`hardware/boards/`](hardware/boards/) for schematics and BOM.

---

## Effects

| Effect | Status | File |
|---|---|---|
| Delay | ✅ Implemented | `firmware/daisy/Delay.cpp` |
| Distortion | ✅ Implemented | `firmware/daisy/Distortion.cpp` |
| *More TBD* | 🔲 Planned | — |

Effect parameter specs live in [`docs/effects-spec.md`](docs/effects-spec.md).

---

## Contributing

Please read [`CONTRIBUTING.md`](CONTRIBUTING.md) before opening a pull request.

---

## License

This project is licensed under the **MIT License** — see [`LICENSE`](LICENSE) for details.
