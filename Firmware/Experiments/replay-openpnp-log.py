"""
Replay the G-code commands from an OpenPnP log to an OpenKayo board.

Sends each command that OpenPnP sent (">> CMD, timeout" lines), waits for
"ok" like OpenPnP does, and flags any command that takes longer than
SLOW_MS. Conveyor commands (M20-M28) are skipped, because they wait for a
PCB.

When a command stalls, the script waits STALL_PROBE_S and then sends a
bare newline. If the missing output arrives straight after the newline,
the reply was stuck in the USB path, not lost.

WARNING: this moves the machine through the same XY/Z/vac/feeder sequence
as the logged job. Make sure the machine is in the same setup, or clear.

Usage:
  python3 replay-openpnp-log.py /dev/ttyACM0 OpenPnP.log [start_line]
"""
import re
import sys
import time
import serial

PORT = sys.argv[1]
LOG = sys.argv[2]
START = int(sys.argv[3]) if len(sys.argv) > 3 else 0
SLOW_MS = 500
TIMEOUT = 10.0
STALL_PROBE_S = 3.0
SKIP = re.compile(r"^M2[0-8]\b")

commands = []
with open(LOG, errors="replace") as f:
    for n, line in enumerate(f, 1):
        m = re.search(r"GcodeDriver DEBUG: .*>> (.*?), \d+$", line)
        if m and n >= START and not SKIP.match(m.group(1).strip()):
            commands.append((n, m.group(1).strip()))

print(f"{len(commands)} commands to replay")
kayo = serial.Serial(PORT, 115200, timeout=0)
time.sleep(2)
kayo.reset_input_buffer()

buf = b""


def read_lines():
    global buf
    buf += kayo.read(4096)
    lines = []
    while b"\n" in buf:
        line, buf = buf.split(b"\n", 1)
        lines.append(line.decode(errors="replace").strip())
    return lines


slow = []
for n, cmd in commands:
    start = time.monotonic()
    kayo.write((cmd + "\n").encode())
    probed = False
    transcript = []
    done = False
    while not done:
        elapsed = time.monotonic() - start
        for text in read_lines():
            transcript.append(f"+{(time.monotonic() - start) * 1000:8.1f} ms  {text}")
            if text == "ok" or text.startswith("ERR"):
                done = True
        if done:
            break
        if not probed and elapsed > STALL_PROBE_S:
            kayo.write(b"\n")  # Host-side write, to see if it releases stuck output
            transcript.append(f"+{elapsed * 1000:8.1f} ms  [probe: sent bare newline]")
            probed = True
        if elapsed > TIMEOUT:
            transcript.append("!! TIMEOUT")
            break
        time.sleep(0.001)
    ms = (time.monotonic() - start) * 1000
    if ms > SLOW_MS or not done:
        slow.append((n, cmd, ms))
        print(f"\n*** SLOW: log line {n}: {cmd}  ({ms:.0f} ms)")
        print("\n".join("    " + t for t in transcript))
    else:
        print(f"{ms:7.1f} ms  {cmd}")

print(f"\n=== {len(slow)} slow commands (> {SLOW_MS} ms) ===")
for n, cmd, ms in slow:
    print(f"line {n:5}: {ms:8.0f} ms  {cmd}")
kayo.close()
