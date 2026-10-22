"""
Repeat the pick sequence that triggers the OpenKayo "ok" stall.

Every stall in the 2026-10-20 OpenPnP log was a feeder command sent right
after this sequence:
  G0 I-<down>  ->  M6 N1  ->  G0 I-1.0  ->  M601 (or M600)

This script repeats that sequence (no XY moves) and counts commands whose
"ok" doesn't arrive within STALL_MS. When a command stalls, the script
sends a bare newline after PROBE_S. If the missing "ok" arrives straight
after the newline, the output was stuck in the USB path, not lost.

Put the head over an empty area first: nozzle 1 descends to Z_DOWN every
cycle.

Usage:
  python3 pick-loop-test.py /dev/ttyACM0 [cycles] [feeder]
"""
import sys
import time
import serial

PORT = sys.argv[1]
CYCLES = int(sys.argv[2]) if len(sys.argv) > 2 else 500
FEEDER = sys.argv[3] if len(sys.argv) > 3 else "1"
Z_DOWN = "-5.0"          # Keep well clear of anything under the nozzle
STALL_MS = 500
PROBE_S = 2.0
TIMEOUT_S = 5.0

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


def send(cmd, timeout=TIMEOUT_S):
    """Send one line, wait for "ok". Return (ms, status, transcript)."""
    start = time.monotonic()
    kayo.write((cmd + "\n").encode())
    probe_at = None
    transcript = []
    while True:
        now = time.monotonic()
        for text in read_lines():
            transcript.append(f"+{(now - start) * 1000:8.1f} ms  {text}")
            if text == "ok":
                ms = (now - start) * 1000
                if probe_at is None:
                    return ms, "ok", transcript
                released = (now - probe_at) * 1000
                return ms, f"released {released:.0f} ms after probe", transcript
        if probe_at is None and timeout <= TIMEOUT_S and now - start > PROBE_S:
            kayo.write(b"\n")
            probe_at = now
            transcript.append(f"+{(now - start) * 1000:8.1f} ms  [probe: bare newline]")
        if now - start > timeout:
            return (now - start) * 1000, "TIMEOUT", transcript
        time.sleep(0.0005)


def drain(seconds=0.3):
    """Discard leftover output, e.g. the ERR reply to a probe newline."""
    end = time.monotonic() + seconds
    while time.monotonic() < end:
        read_lines()
        time.sleep(0.01)


print(send("G28", 60)[1])
send(f"M600 N{FEEDER}")

stalls = []
for cycle in range(1, CYCLES + 1):
    sequence = ["G0 I" + Z_DOWN, "M6 N1", "G0 I-1.0000",
                "M601", f"M600 N{FEEDER}", "M7 N1"]
    for cmd in sequence:
        ms, status, transcript = send(cmd)
        if ms > STALL_MS:
            stalls.append((cycle, cmd, ms, status))
            print(f"\n*** cycle {cycle}: {cmd} took {ms:.0f} ms ({status})")
            print("\n".join("    " + t for t in transcript))
            drain()
    if cycle % 25 == 0:
        print(f"{cycle} cycles, {len(stalls)} stalls")

send("M601")
print(f"\n=== {len(stalls)} stalls in {CYCLES} cycles ===")
for cycle, cmd, ms, status in stalls:
    print(f"cycle {cycle:4}: {cmd:12} {ms:7.0f} ms  {status}")
kayo.close()
