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

## QSPI boot setup

The application uses `BOOT_QSPI`, giving it nearly 8 MB of external flash. The
Daisy bootloader must be installed in internal flash once before QSPI
applications can run.

1. Connect the Daisy Seed to the computer over USB.
2. Enter the STM32 ROM DFU mode:
   - Hold the BOOT button.
   - Press and release RESET.
   - Release BOOT.
3. Install the Daisy bootloader:

```bash
cd firmware/daisy
make program-boot
```

## Upload code

1. Reset the Daisy and wait for the user LED to pulse during the bootloader's
   two-second DFU window. Press BOOT during this window to keep it open.
2. Build and upload the QSPI application:

```bash
cd firmware/daisy
make clean
make
make program-dfu
```

`program-dfu` writes the application to QSPI at `0x90040000`. `make program`
is intentionally unavailable for QSPI applications. If upload fails, confirm
that the Daisy bootloader is installed, its LED is pulsing, and `dfu-util` is
installed.

## Quick test

The bridge protocol helper can exercise the UART command path from a host PC:

```bash
python Utils/bridge_protocol_test.py --serial-port COM9 --repeat 10 ping info status
```

The ESP32 firmware is the normal host for these commands in the finished system.
