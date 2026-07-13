# Daisy firmware

This firmware runs on the Daisy Seed and contains the DSP effect pipeline for the pedal.

## Build

```bash
git submodule update --init --recursive
cd firmware/daisy/deps/libDaisy
make
cd ../DaisySP
make
cd ../../
make
```

## Upload code

1. Connect the Daisy Seed to your computer over USB.
2. Put the board into DFU bootloader mode:
   - Hold the BOOT button.
   - Press and release RESET.
   - Release BOOT.
3. Build the firmware if needed.
4. Flash it with:

```bash
cd firmware/daisy
make program-dfu
```

If the upload fails, confirm the board is in bootloader mode and that `dfu-util` is installed.

## Quick test

The bridge protocol helper can exercise the UART command path from a host PC:

```bash
python Utils/bridge_protocol_test.py --serial-port COM9 --repeat 10 ping info status
```

The ESP32 firmware is the normal host for these commands in the finished system.
