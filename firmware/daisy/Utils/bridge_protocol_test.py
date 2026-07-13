#!/usr/bin/env python3
"""Exercise the ChimeraMultiFX serial protocol through USB serial or ESP32 HTTP."""

import argparse
import json
import sys
import time
import urllib.error
import urllib.parse
import urllib.request


DEFAULT_COMMANDS = ["ping", "info", "status"]


def read_serial_line(serial_port, timeout):
    deadline = time.time() + timeout
    line = bytearray()

    while time.time() < deadline:
        byte = serial_port.read(1)
        if byte:
            line.extend(byte)
            if byte == b"\n":
                return line.decode("utf-8", errors="replace")
        else:
            time.sleep(0.01)

    return line.decode("utf-8", errors="replace") if line else ""


def run_serial_tests(port, baudrate, timeout, commands, uart_loopback):
    try:
        import serial
    except ImportError:
        print("pyserial is required for --serial-port tests: pip install pyserial", file=sys.stderr)
        return 2

    with serial.Serial(port, baudrate, timeout=0.05) as serial_port:
        serial_port.reset_input_buffer()
        serial_commands = list(commands)
        if uart_loopback and "loopback" not in serial_commands:
            serial_commands.append("loopback")

        for command in serial_commands:
            serial_port.write((command.strip() + "\n").encode("utf-8"))
            reply = read_serial_line(serial_port, timeout)
            print(f"serial {command!r} -> {reply.rstrip()!r}")
            if not reply:
                print(f"timeout waiting for {command!r}", file=sys.stderr)
                return 1
            if command == "ping" and not reply.startswith("PONG"):
                print(f"expected PONG, got {reply.rstrip()!r}", file=sys.stderr)
                return 1
            if command == "loopback" and not reply.startswith("OK uart loopback"):
                print(f"expected OK uart loopback, got {reply.rstrip()!r}", file=sys.stderr)
                return 1

    return 0


def http_get(base_url, path, timeout):
    url = urllib.parse.urljoin(base_url.rstrip("/") + "/", path.lstrip("/"))
    request = urllib.request.Request(url, headers={"User-Agent": "ChimeraMultiFX-serial-test"})
    with urllib.request.urlopen(request, timeout=timeout) as response:
        return response.status, response.read().decode("utf-8", errors="replace")


def run_http_tests(base_url, timeout, commands):
    checks = [("health", "/health"), ("bridge selftest", "/api/bridge/selftest")]
    for name, path in checks:
        try:
            status, body = http_get(base_url, path, timeout)
        except urllib.error.URLError as error:
            print(f"http {name} failed: {error}", file=sys.stderr)
            return 1
        print(f"http {name} -> {status} {body.rstrip()!r}")
        if status != 200:
            return 1

    for command in commands:
        query = urllib.parse.urlencode({"cmd": command})
        try:
            status, body = http_get(base_url, f"/api/daisy/command?{query}", timeout)
        except urllib.error.URLError as error:
            print(f"http command {command!r} failed: {error}", file=sys.stderr)
            return 1
        print(f"http command {command!r} -> {status} {body.rstrip()!r}")
        if status != 200:
            return 1
        if command == "ping" and not body.startswith("PONG"):
            print(f"expected PONG, got {body.rstrip()!r}", file=sys.stderr)
            return 1
        if command in {"info", "status"}:
            try:
                json.loads(body)
            except json.JSONDecodeError as error:
                print(f"{command!r} did not return valid JSON: {error}", file=sys.stderr)
                return 1

    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base-url", default="http://192.168.4.1", help="ESP32 bridge URL")
    parser.add_argument("--serial-port", help="Daisy USB serial port, for direct parser tests")
    parser.add_argument("--with-http", action="store_true", help="also run ESP32 HTTP bridge tests after serial tests")
    parser.add_argument("--uart-loopback", action="store_true", help="run Daisy USART1 loopback test; short D13 TX to D14 RX first")
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=2.0)
    parser.add_argument("commands", nargs="*", default=DEFAULT_COMMANDS)
    args = parser.parse_args()

    exit_code = 0
    if args.serial_port:
        exit_code = run_serial_tests(args.serial_port, args.baudrate, args.timeout, args.commands, args.uart_loopback)
        if exit_code != 0:
            return exit_code
        if not args.with_http:
            return 0

    return run_http_tests(args.base_url, args.timeout, args.commands)


if __name__ == "__main__":
    raise SystemExit(main())