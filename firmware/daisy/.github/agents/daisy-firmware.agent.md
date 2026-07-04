---
name: Daisy Firmware Agent
description: Use when working on ChimeraMultiFX Daisy firmware, routing, serial commands, effects, USB CDC, DFU, libDaisy, DaisySP, or real-time audio behavior.
tools: [read, search, edit, execute, todo]
---

# ChimeraMultiFX Daisy Firmware Agent Guide

## Project Purpose

This workspace contains the Daisy Seed firmware for ChimeraMultiFX: a real-time, USB-serial-controlled multi-effect audio router for the Electrosmith Daisy Seed / STM32H750.

The firmware is built around dynamically adding, removing, routing, and parameterizing mono effects at runtime over USB CDC serial. The audio callback is intentionally small: it reads stereo input, asks the router to process it, and writes stereo output.

## Main Entry Points

- `main.cpp`: hardware initialization, USB CDC receive callback, audio callback, main loop, deferred serial actions.
- `SerialController.h`: human-readable USB serial command parser and dispatcher.
- `Router.h`: four-lane effect routing engine, lane input/output selection, lane-to-lane feeds, effect slot management.
- `Effect.h`: base effect interface and parameter metadata contract.
- `effects/**`: header-only effect implementations grouped by category.
- `Makefile`: firmware build configuration. `CPP_SOURCES = main.cpp`; effects are included through headers.
- `SerialPCInterface.py`: simple Python serial terminal for sending commands to the device.

## Runtime Architecture

`main.cpp` owns these globals:

- `DaisySeed hw`
- `Router router`
- `SerialController serial`

USB receive flow:

1. `UsbRxCallback()` receives bytes from `hw.usb_handle`.
2. Each byte is passed to `serial.Feed()`.
3. `SerialController` buffers until newline or carriage return.
4. A complete command is tokenized and dispatched.

Audio flow:

1. `AudioCallback()` receives input block buffers.
2. For each sample, it calls `router.Process(in[0][i], in[1][i])`.
3. Router processes active lanes in order.
4. Lane outputs are summed to `out1` and/or `out2`.
5. If no lane is active, router passes through `in1` and `in2`.

Deferred actions:

- Commands that reset or otherwise disrupt USB must not execute directly inside `UsbRxCallback()`.
- `SerialController::ProcessPendingActions()` is called from the main loop for deferred work such as rebooting to DFU.

## Serial Command Interface

Commands are newline-terminated ASCII strings. Current command families:

- `add <lane> <effect>`
- `insert <lane> <slot> <effect>`
- `remove <lane> <slot>`
- `swap <lane> <slotA> <slotB>`
- `move <from_lane> <from_slot> <to_lane> <to_slot>`
- `set <lane> <slot> <param> <value>`
- `get <lane> <slot> <param>`
- `bypass <lane> <slot> <0|1>`
- `clear <lane>`
- `route <lane> <input> <output>`
- `level <lane> <value>`
- `params <lane> <slot>`
- `status`
- `info`
- `effect <effect>`
- `dfu`

Effect names currently advertised by `info`:

- `distortion`
- `bitcrusher`
- `overdrive`
- `chorus`
- `tremolo`
- `delay`
- `compressor`
- `lowpass`

Input names:

- `in1`, `in2`, `mix`, `lane0`, `lane1`, `lane2`, `lane3`

Output names:

- `out1`, `out2`, `both`, `none`

Important serial implementation notes:

- `SerialController::MAX_TOKENS` is 8, so keep new commands short or increase it deliberately.
- Replies are sent with `UsbHandle::TransmitInternal()` through `SendBuffer()`.
- `status` and `info` emit JSON-like single-line responses for host tooling.
- `effect <effect>` returns parameter metadata for one effect.
- The `dfu` command should only request DFU, not reset immediately from the USB callback.

## Router Contract

`Router` has:

- `MAX_LANES = 4`
- `MAX_SLOTS = 8`
- `Lane lanes[MAX_LANES]`

Each lane has:

- `Effect* slots[MAX_SLOTS]`
- `count`
- `level`
- `active`
- `input`
- `output`
- `last_out`

Lane-to-lane routing depends on processing order. A lane can consume another lane's `last_out`; backwards or self-referential routing may use the previous processed value depending on order, so be careful when changing routing behavior.

Memory ownership:

- `SerialController::CreateFromName()` dynamically allocates effects with `new`.
- Commands that remove or clear effects must delete owned pointers before removing them from lanes.
- `Router::Lane::Remove()` and `Clear()` only alter slots/count; they do not delete effects.

## Effect Contract

All effects derive from `Effect` in `Effect.h` and must implement:

- `Init(float sample_rate)`
- `Process(float in)`
- `GetName() const`
- `GetCategory() const`

Optional but expected for serial/UI integration:

- `SetParam(const char* name, float value)`
- `GetParam(const char* name)`
- `SetBoolParam(const char* name, bool value)`
- `GetBoolParam(const char* name)`
- `GetParamList() const`
- `GetParamCount() const`
- `GetParamInfo(int index, EffectParamInfo& info) const`

`Tick(float in)` wraps `Process()` with per-effect bypass behavior. Effects should generally put audio processing in `Process()` and rely on `Tick()` for bypass.

When adding a new effect:

1. Add the header under the appropriate `effects/<category>/` folder.
2. Include it in `SerialController.h`.
3. Add a name mapping to `CreateFromName()`.
4. Add the name to the `info` command effect list.
5. Implement useful parameter metadata so host tools can build controls.
6. Build with `make clean; make`.

## Build And Flash

Primary workspace path:

```bash
cd firmware/daisy
```

Build firmware:

```bash
make clean; make
```

Build dependencies if needed:

```bash
cd deps/libDaisy && make
cd ../DaisySP && make
```

Flash via STM DFU:

```bash
make program-dfu
```

Flash via ST-Link:

```bash
make program
```

VS Code tasks exist for:

- `build`
- `build_and_program`
- `build_and_program_dfu`
- `build_all`
- `build_all_debug`
- `program`
- `program-dfu`
- `build_libdaisy`
- `build_daisysp`

Use `build` for normal validation after code changes.

## DFU Notes

The project currently uses `APP_TYPE ?= BOOT_NONE`, so `program-dfu` writes to internal flash at `0x08000000` and expects STM DFU PID `0483:df11`.

The serial `dfu` command should enter STM ROM DFU mode using:

```cpp
daisy::System::ResetToBootloader(daisy::System::BootloaderMode::STM);
```

Do not call that reset directly inside `UsbRxCallback()` or directly inside command dispatch called by the USB callback. Request it, return from the callback, then handle reset from the main loop after a short delay so the USB acknowledgement and host disconnect can settle.

## Python Serial Host

`SerialPCInterface.py` opens `COM9` at `115200` baud by default. Change the COM port as needed for the machine.

Run from `firmware/daisy`:

```bash
python SerialPCInterface.py
```

The script sends newline-terminated commands and drains the immediate response.

## Coding Guidelines For Agents

- Keep audio callback work deterministic and allocation-free.
- Do not allocate or delete effects in the audio callback.
- Treat serial command handling as host-control code, not audio-rate code.
- Keep command responses short and newline-terminated.
- Be careful with dynamic allocation; this firmware currently uses `new`/`delete` for effects, but avoid adding background allocation churn.
- Preserve the header-only effect pattern unless intentionally refactoring the build.
- Prefer existing libDaisy and DaisySP primitives over hand-rolling hardware/DSP utilities.
- Avoid broad formatting churn; existing files have uneven formatting in places.
- After firmware edits, run the `build` task or `make clean; make`.
- Do not assume README architecture details are fully current; verify against `main.cpp`, `SerialController.h`, `Router.h`, and `Effect.h`.
