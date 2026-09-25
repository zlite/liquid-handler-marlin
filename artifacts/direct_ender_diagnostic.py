"""Interactive, timestamped serial diagnostics. One acknowledged command at a time."""
import json
import queue
import sys
import threading
import time
from pathlib import Path

import serial

port = serial.Serial(baudrate=250000, timeout=0.1, write_timeout=2)
port.dtr = False
port.rts = False
port.port = "/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0"
port.open()
replies = queue.Queue()
started = time.monotonic()
log = Path(__file__).with_suffix(".log").open("a", buffering=1)


def record(direction, value):
    line = f"{time.monotonic() - started:9.3f} {direction} {value}"
    print(line, flush=True)
    log.write(line + "\n")


def receive():
    pending = bytearray()
    while port.is_open:
        data = port.read(256)
        for byte in data:
            if byte == 10:
                line = pending.decode("ascii", errors="replace").strip()
                pending.clear()
                if line:
                    record("RX", line)
                    replies.put(line)
            elif byte != 13:
                pending.append(byte)


threading.Thread(target=receive, daemon=True).start()
time.sleep(3)
print("READY: submit a JSON object with commands, optional repeat and interval_s", flush=True)
for request in sys.stdin:
    try:
        task = json.loads(request)
        commands = task["commands"]
        for iteration in range(task.get("repeat", 1)):
            cycle_start = time.monotonic()
            for command in commands:
                if "\n" in command or "\r" in command:
                    raise ValueError("Only one command per entry")
                while not replies.empty():
                    replies.get_nowait()
                record("TX", command)
                port.write((command + "\n").encode("ascii"))
                deadline = time.monotonic() + task.get("timeout_s", 15)
                while True:
                    remaining = deadline - time.monotonic()
                    if remaining <= 0:
                        raise TimeoutError(f"No acknowledgement: {command}; do not replay")
                    try:
                        line = replies.get(timeout=remaining)
                    except queue.Empty:
                        raise TimeoutError(f"No acknowledgement: {command}; do not replay")
                    if line.startswith(("Error:", "Resend:", "!!")):
                        raise RuntimeError(line)
                    if line == "ok" or line.startswith("ok "):
                        break
            delay = task.get("interval_s", 0) - (time.monotonic() - cycle_start)
            if delay > 0:
                time.sleep(delay)
        record("DONE", task.get("name", "command batch"))
    except Exception as error:
        record("FAILED", str(error))
