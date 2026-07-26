#!/usr/bin/env python3
"""Live serial plotter for the absonus sensor-test firmware.

Reads the `name:value,name:value,...` lines streamed by sensor-test.cpp over the
Daisy's USB serial port and draws a rolling live plot of every input — the same
job the Arduino IDE Serial Plotter did, but self-contained in this repo.

Use it during assembly to verify solder work and confirm each pot, switch, soft-pot,
and pressure sensor reads as expected BEFORE wiring the audio section.

Setup (project-local venv, from the repo root):
    python3 -m venv .venv
    .venv/bin/pip install -r firmware/sensor-test/requirements.txt

Run:
    .venv/bin/python firmware/sensor-test/plot_sensors.py            # auto-detect port
    .venv/bin/python firmware/sensor-test/plot_sensors.py --list     # list serial ports
    .venv/bin/python firmware/sensor-test/plot_sensors.py --port /dev/tty.usbmodem1234

The Daisy enumerates as USB CDC, so the baud rate is ignored; --baud is accepted
only for compatibility.
"""

import argparse
import sys
from collections import defaultdict, deque

import serial
import serial.tools.list_ports
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# Daisy Seed USB CDC typically shows up as a usbmodem (macOS) or ACM (Linux) port.
_PORT_HINTS = ("usbmodem", "ACM", "usbserial")
_WINDOW = 400  # samples kept on screen (~40 s at the firmware's 100 ms interval)


def find_port():
    """Return the most likely Daisy serial port, or None."""
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        if any(h in p.device for h in _PORT_HINTS):
            return p.device
    return ports[0].device if ports else None


def list_ports():
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("No serial ports found.")
        return
    print("Available serial ports:")
    for p in ports:
        print(f"  {p.device:24}  {p.description}")


def parse_line(line):
    """Parse 'name:value,name:value,...' into a dict of floats. Ignore junk."""
    out = {}
    for field in line.strip().split(","):
        if ":" not in field:
            continue
        name, _, value = field.partition(":")
        try:
            out[name.strip()] = float(value)
        except ValueError:
            continue
    return out


def main():
    ap = argparse.ArgumentParser(description="Live plot of absonus sensor-test serial output.")
    ap.add_argument("--port", help="serial port (default: auto-detect)")
    ap.add_argument("--baud", type=int, default=115200, help="baud (ignored for USB CDC)")
    ap.add_argument("--list", action="store_true", help="list serial ports and exit")
    args = ap.parse_args()

    if args.list:
        list_ports()
        return

    port = args.port or find_port()
    if not port:
        sys.exit("No serial port found. Plug in the Daisy or pass --port. Try --list.")

    try:
        ser = serial.Serial(port, args.baud, timeout=1)
    except serial.SerialException as e:
        sys.exit(f"Could not open {port}: {e}")

    print(f"Reading {port} — close the plot window to stop.")

    series = defaultdict(lambda: deque([float("nan")] * _WINDOW, maxlen=_WINDOW))
    lines = {}

    fig, ax = plt.subplots(figsize=(11, 6))
    ax.set_title(f"absonus sensor-test — {port}")
    ax.set_xlabel(f"samples (window = {_WINDOW})")
    ax.set_ylabel("normalized value")
    ax.set_ylim(-0.05, 1.05)
    ax.grid(True, alpha=0.3)

    def update(_frame):
        # Drain everything buffered so the plot stays current.
        while ser.in_waiting:
            raw = ser.readline().decode("utf-8", errors="replace")
            values = parse_line(raw)
            if not values:
                continue
            for name, val in values.items():
                series[name].append(val)
            # Pad any series that didn't appear this line so lengths stay aligned.
            for name in series:
                if name not in values:
                    series[name].append(series[name][-1])

        for name, buf in series.items():
            if name not in lines:
                (lines[name],) = ax.plot(range(_WINDOW), list(buf), label=name, linewidth=1.2)
                ax.legend(loc="upper left", ncol=3, fontsize=8)
            else:
                lines[name].set_ydata(list(buf))
        return list(lines.values())

    _anim = animation.FuncAnimation(fig, update, interval=50, blit=False, cache_frame_data=False)
    try:
        plt.show()
    finally:
        ser.close()


if __name__ == "__main__":
    main()