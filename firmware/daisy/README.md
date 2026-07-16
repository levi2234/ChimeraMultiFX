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

### DFU modes

The Seed has two different DFU implementations. Both can appear as USB device
`0483:df11`, but they expose different memory and are not interchangeable.

| DFU mode | Provided by | Memory exposed | Purpose | Command |
| --- | --- | --- | --- | --- |
| STM32 ROM DFU | Factory code inside the STM32 | Internal flash at `0x08000000` | Install or recover the Daisy bootloader | `make program-boot` |
| Daisy bootloader DFU | Daisy bootloader stored in internal flash | External QSPI flash from `0x90000000`, including the application at `0x90040000` | Upload the Chimera application | `make program-dfu` |

Enter STM32 ROM DFU by holding BOOT, pressing and releasing RESET, then
releasing BOOT. Enter Daisy bootloader DFU by pressing RESET without holding
BOOT, waiting for the user LED to pulse, then briefly pressing BOOT during the
two-second window.

Use `dfu-util -l` to tell them apart. STM32 ROM DFU reports `@Internal Flash
/0x08000000/...`; Daisy bootloader DFU reports an external `@Flash
/0x90000000/...` map. STM32 ROM DFU cannot upload the Chimera application to
QSPI, while the Daisy bootloader DFU is the normal mode for firmware updates.

### One-time bootloader installation

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

The Daisy bootloader must be active before uploading a QSPI application. This
is different from the STM32 ROM DFU mode used to install the bootloader.

1. Connect the Daisy Seed over USB and release both buttons.
2. Press and release RESET only. Do not hold BOOT while pressing RESET.
3. Wait for the user LED to pulse during the Daisy bootloader's two-second DFU
   window, then briefly press BOOT to keep that window open.
4. Confirm that the Daisy bootloader is exposing QSPI flash:

```bash
dfu-util -l
```

The device description must contain an external flash map similar to:

```text
@Flash /0x90000000/64*4Kg/0x90040000/60*64Kg/...
```

If it reports only `@Internal Flash /0x08000000/...`, the Seed is in STM32 ROM
DFU mode and cannot write the QSPI application. Reset the Seed and repeat the
RESET-only, wait-for-pulse, then BOOT sequence above.

5. Build and upload the QSPI application:

```bash
cd firmware/daisy
make clean
make
make program-dfu
```

`program-dfu` writes the application to QSPI at `0x90040000`. `make program`
is intentionally unavailable for QSPI applications. If upload fails, confirm
that the Daisy bootloader is installed, its LED is pulsing, and `dfu-util` is
installed. The warning `Invalid DFU suffix signature` is harmless. An error
such as `Last page at 0x900... is not writeable`, together with an `Internal
Flash` interface name, means the Seed entered STM32 ROM DFU instead of the
Daisy bootloader.

For later uploads, the bootloader does not need to be reinstalled. If the Daisy
bootloader LED never pulses after a RESET-only press, enter STM32 ROM DFU using
the one-time setup sequence above and run `make program-boot` again.

## Quick test

The bridge protocol helper can exercise the UART command path from a host PC:

```bash
python Utils/bridge_protocol_test.py --serial-port COM9 ping info status "status lane 0"
```

State reads are intentionally split into bounded JSON responses:

- `status` returns only `lane_count` and `max_slots`.
- `status lane <lane>` returns routing and effect topology for one lane, without parameters.
- `status slot <lane> <slot>` returns live values and parameter definitions for one effect.

Clients should refresh only the lane or slot affected by a mutation. This keeps
response size independent of the total number of effect parameters in the rig.

The ESP32 firmware is the normal host for these commands in the finished system.
