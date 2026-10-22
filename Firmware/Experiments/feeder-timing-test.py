"""
Feeder command timing test for OpenKayo.

Sends feeder commands to the OpenKayo board with controlled gaps between
them, to find out whether the Kayo delays replies when commands arrive
close together. Each command waits for "ok" (like OpenPnP does), then
sleeps for the gap before sending the next one.

Every line from the board is printed with the host time in ms since the
command was sent. Compare that with the firmware's own millis() stamps:
  - firmware "Received" late too   -> delay is on the Kayo side
  - firmware on time, host late    -> output delayed on the USB path

Usage:
  python3 feeder-timing-test.py /dev/cu.usbmodemXXXX [feeder]
"""
import sys
import time
import serial

PORT = sys.argv[1]
FEEDER = sys.argv[2] if len(sys.argv) > 2 else "1"
TIMEOUT = 10.0           # seconds to wait for "ok" (OpenPnP used 8)
HOME_TIMEOUT = 60.0
GAPS = [1.0, 0.3, 0.1, 0.0]
REPEATS = 3

kayo = serial.Serial(PORT, 115200, timeout=0.05)
time.sleep(2)
kayo.reset_input_buffer()


def send(cmd, timeout=TIMEOUT):
    """Send one line and print replies until "ok", "ERR" or timeout."""
    start = time.monotonic()
    kayo.write((cmd + "\n").encode())
    print(f"\n>> {cmd}")
    buf = b""
    while time.monotonic() - start < timeout:
        buf += kayo.read(256)
        while b"\n" in buf:
            line, buf = buf.split(b"\n", 1)
            text = line.decode(errors="replace").strip()
            elapsed = (time.monotonic() - start) * 1000
            print(f"   +{elapsed:7.1f} ms  {text}")
            if text == "ok" or text.startswith("ERR"):
                return elapsed
    print(f"   !! TIMEOUT after {timeout:.0f} s waiting for ok")
    return None


send("G28", HOME_TIMEOUT)

results = []
for gap in GAPS:
    for _ in range(REPEATS):
        # Pattern from the job: close, short gap, close again
        send(f"M600 N{FEEDER}")
        time.sleep(gap)
        t1 = send("M601")
        time.sleep(gap)
        t2 = send("M601")
        results.append((gap, t1, t2))
        time.sleep(1.0)

print("\n=== Summary (ms to ok; None = timeout) ===")
print("gap(s)   1st M601   2nd M601")
for gap, t1, t2 in results:
    fmt = lambda t: f"{t:8.1f}" if t is not None else "    None"
    print(f"{gap:5.1f}  {fmt(t1)}   {fmt(t2)}")

kayo.close()
