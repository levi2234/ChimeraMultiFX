# Daisy Seed DSP Firmware

Real-time audio effects firmware for the **Electrosmith Daisy Seed** (STM32H750, ARM Cortex-M7).

Built with [libDaisy](https://github.com/electro-smith/libDaisy) (hardware abstraction) and [DaisySP](https://github.com/electro-smith/DaisySP) (DSP algorithms).

---

## Prerequisites

| Tool | Install |
|---|---|
| `arm-none-eabi-gcc` | [ARM GNU Toolchain](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain) |
| `make` | via your package manager or Git for Windows |
| `dfu-util` | [dfu-util.sourceforge.net](http://dfu-util.sourceforge.net/) |
| `git` | for submodule init |

---

## Repository Layout

```
firmware/daisy/
├── deps/
│   ├── libDaisy/       # Hardware abstraction library (submodule)
│   └── DaisySP/        # DSP algorithm library (submodule)
├── Delay.cpp           # Delay effect
├── Distortion.cpp      # Distortion effect
├── Makefile            # Build system
└── README.md           ← you are here
```

---

## First-Time Setup — Build the Dependencies

Both `libDaisy` and `DaisySP` must be built once before the project can compile.

```bash
# From the repository root — initialise submodules
git submodule update --init --recursive

# Build libDaisy
cd firmware/daisy/deps/libDaisy
make -j4

# Build DaisySP
cd ../DaisySP
make -j4
```

> **Windows users:** Use Git Bash or WSL. The ARM toolchain must be on your `PATH`.

---

## Selecting an Effect

Open `Makefile` and set `CPP_SOURCES` to the effect you want to build:

```makefile
# Build the Delay effect
CPP_SOURCES = Delay.cpp

# — or — build the Distortion effect
CPP_SOURCES = Distortion.cpp
```

Only one effect is compiled at a time. See [`docs/effects-spec.md`](../../docs/effects-spec.md) for effect documentation.

---

## Build

```bash
cd firmware/daisy
make -j4
```

Output binaries are placed in `build/` (ignored by `.gitignore`).

---

## Serial Control

The firmware accepts newline-terminated ASCII commands through both USB CDC and
a hardware UART for the ESP32 control bridge. Both paths feed the same
`SerialController::Feed()` parser, and replies are sent back to both enabled
links.

Default ESP32 UART wiring:

- Daisy Seed physical pin 14 / `D13` / `PB6` / USART1 TX -> ESP32 GPIO16 / UART2 RX
- Daisy Seed physical pin 15 / `D14` / `PB7` / USART1 RX <- ESP32 GPIO17 / UART2 TX
- Daisy GND -> ESP32 GND
- Baud rate: `115200`

Before reconnecting the Daisy to a freshly flashed ESP32 bridge, test the ESP32
UART2 pins alone by disconnecting the Daisy, jumpering ESP32 GPIO17 to GPIO16,
and opening `http://192.168.4.1/api/uart2/loopback`. Only reconnect the Daisy
after that endpoint returns `OK uart2 loopback`.

The ESP32 bridge exposes HTTP endpoints that forward commands such as `info`,
`status`, and `add 0 delay` over this UART link.

Use `Utils/bridge_protocol_test.py` to validate the link. With the Daisy plugged
into the PC over USB serial, `python Utils/bridge_protocol_test.py --serial-port
COM9` tests the parser directly. Add `--with-http` to run the ESP32 bridge checks
after direct serial. With the ESP32 flashed and your PC connected to
the `ChimeraMultiFX` AP, `python Utils/bridge_protocol_test.py` tests `/health`,
`/api/bridge/selftest`, and forwarded Daisy commands.

---

## Flash

### Via USB DFU (recommended)

1. Hold the **BOOT** button on the Daisy Seed.
2. While holding BOOT, tap **RESET** (or power cycle).
3. Release **BOOT** — the Daisy is now in DFU bootloader mode.
4. Flash:
   ```bash
   make program-dfu
   ```

### Via ST-Link (SWD)

```bash
make program
```

Requires an ST-Link v2 connected to the SWD header on the carrier board.

---

## Adding a New Effect

1. Copy an existing `.cpp` (e.g., `Distortion.cpp`) as your starting point.
2. Rename it (e.g., `Reverb.cpp`).
3. Implement `AudioCallback` and `main()`. Follow the parameter block convention at the top.
4. Update `CPP_SOURCES` in `Makefile`.
5. Document it in [`docs/effects-spec.md`](../../docs/effects-spec.md).

---

## Useful Make Targets

| Target | Description |
|---|---|
| `make` | Compile the project |
| `make program-dfu` | Flash via USB DFU |
| `make program` | Flash via ST-Link |
| `make clean` | Remove build artifacts |
