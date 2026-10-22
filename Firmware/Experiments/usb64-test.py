"""
Host side of the USB64Test sketch.

Asks the board for replies of various lengths and checks whether each
reply arrives in full without any further host writes. A stuck reply is
then released by sending a dummy request, and the script reports how
long it was held.

Usage:
  python3 usb64-test.py /dev/ttyACM0 [repeats]
"""
import sys
import time
import serial

PORT = sys.argv[1]
REPEATS = int(sys.argv[2]) if len(sys.argv) > 2 else 20
LENGTHS = [60, 62, 63, 64, 65, 66, 127, 128, 129, 192]
WAIT_S = 0.5

board = serial.Serial(PORT, 115200, timeout=0)
time.sleep(2)
board.reset_input_buffer()


def request(n):
    """Return (ok, ms). ok is False if the reply needed a release write."""
    board.write(f"{n}\n".encode())
    start = time.monotonic()
    got = b""
    while time.monotonic() - start < WAIT_S:
        got += board.read(4096)
        if len(got) >= n:
            return True, (time.monotonic() - start) * 1000
        time.sleep(0.0005)
    # Stuck: release it with a 1-byte reply, then discard everything
    board.write(b"1\n")
    end = time.monotonic() + WAIT_S
    while time.monotonic() < end:
        got += board.read(4096)
        time.sleep(0.001)
    board.reset_input_buffer()
    return False, (time.monotonic() - start) * 1000


print(f"{'bytes':>6}  {'stalled':>8}")
for n in LENGTHS:
    stalls = sum(not request(n)[0] for _ in range(REPEATS))
    print(f"{n:>6}  {stalls:>4}/{REPEATS}")
board.close()
