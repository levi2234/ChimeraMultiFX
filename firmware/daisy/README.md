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
