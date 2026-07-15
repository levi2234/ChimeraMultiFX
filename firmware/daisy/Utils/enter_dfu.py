import argparse
import sys
import time

import serial
from serial.tools import list_ports


DAISY_USB_CDC_VID = 0x0483
DAISY_USB_CDC_PID = 0x5740
DEFAULT_BAUD = 115200


def find_daisy_port():
    for port in list_ports.comports():
        if port.vid == DAISY_USB_CDC_VID and port.pid == DAISY_USB_CDC_PID:
            return port.device
    return None


def read_response(serial_port, timeout_seconds):
    deadline = time.time() + timeout_seconds
    chunks = []

    while time.time() < deadline:
        try:
            waiting = serial_port.in_waiting
        except serial.SerialException:
            break

        if waiting:
            chunks.append(serial_port.read(waiting))
        elif chunks:
            break
        else:
            time.sleep(0.02)

    return b"".join(chunks).decode("utf-8", errors="replace")


def enter_dfu(port, baud, timeout_seconds):
    with serial.Serial(port, baud, timeout=0.1, write_timeout=1) as serial_port:
        time.sleep(0.2)
        serial_port.reset_input_buffer()
        serial_port.write(b"dfu\n")
        serial_port.flush()
        return read_response(serial_port, timeout_seconds)


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Send the ChimeraMultiFX serial dfu command to enter the Daisy bootloader's "
            "persistent QSPI DFU mode."
        )
    )
    parser.add_argument("--port", help="Serial port to use, such as COM9. Auto-detects Daisy USB CDC if omitted.")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD, help=f"Serial baud rate. Default: {DEFAULT_BAUD}.")
    parser.add_argument("--timeout", type=float, default=1.0, help="Seconds to wait for the acknowledgment before the port drops.")
    args = parser.parse_args()

    port = args.port or find_daisy_port()
    if not port:
        print("Could not find a Daisy USB serial device. Pass --port COMx if it is using a different port.", file=sys.stderr)
        return 1

    try:
        response = enter_dfu(port, args.baud, args.timeout)
    except serial.SerialException as error:
        print(f"Failed to send dfu command on {port}: {error}", file=sys.stderr)
        return 1

    if response:
        print(response, end="" if response.endswith("\n") else "\n")
    else:
        print("dfu command sent; no acknowledgment received before USB handoff")

    print(
        f"Sent dfu command on {port}. The serial port should disappear and the Daisy "
        "bootloader should expose QSPI flash at 0x90000000."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
