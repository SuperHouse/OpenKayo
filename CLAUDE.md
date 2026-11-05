# OpenKayo

G-code adapter that lets [OpenPnP](https://openpnp.org/) drive a Kayo-A4
pick-and-place machine. An ESP32-S3 sits between the host (USB serial,
G-code in) and the Kayo's internal motion controller (3.3V UART, 115200 8N1,
proprietary 8-byte binary messages). The factory Windows PC/software is
bypassed entirely. Public repo: github.com/SuperHouse/OpenKayo. Author:
Jonathan Oxer / SuperHouse Automation.

The protocol was reverse-engineered, largely from Glen English's Python
driver (`Resources/KayoMsg.py` is a reference copy of his message formats).

## Layout

- `Firmware/OpenKayo/` — main Arduino sketch. Single translation unit:
  `OpenKayo.ino` declares globals and prototypes, then `#include`s the
  `.h` files, which contain function bodies (not just declarations).
  - `config.h` — all tunables and pin assignments
  - `gcode.h` — USB line buffer, G/M-code parser and dispatch
  - `serial_comms.h` — Kayo UART send/receive and response matching
  - `machine_actions.h` — one `cmdXxx()` per machine operation, each
    building an 8-byte Kayo message
  - `can_comms.h` — J1939-style CAN messages to neighbouring conveyors
- `Firmware/CANTest/` — standalone TWAI receive/dump sketch for bring-up
- `Firmware/Experiments/` — test tools. These may be untracked in git:
  - `test-menu-1.py`: early Python serial test menu (2022)
  - `usb64-test.py` + `USB64Test/`: USB packet-size stall test (board
    only, no Kayo needed)
  - `pick-loop-test.py`: repeats the pick sequence and counts stalled "ok"s
  - `feeder-timing-test.py`, `replay-openpnp-log.py`: earlier diagnosis
  - `OpenPnP.log`, `OpenPnP.0.log`: copies of the 20 and 22 Oct 2026 job
    logs. They are large; don't commit them.
- `Hardware/` — OPENKAYO v1.1 board, EAGLE 9.7 (`.sch`/`.brd`)
- `Mechanical/` — Fusion 360 sources (`.f3d`) + STLs: ELP camera mount and
  lens spacer, Gridfinity discard tray and holder
- `Resources/` — reference material (ESP32-S3 datasheet, diagrams)
- `S600/` — notes on the possibility of supporting the Kayo S600 (9-camera
  frame grabber); exploratory only
- `Notes.md` — board design to-dos and notes
- `Timesheet-*.md` — personal, gitignored; don't commit

## Building

Arduino IDE, "esp32 by Espressif Systems" core, board "ESP32-S3 Dev Module"
(full settings in the header comment of `OpenKayo.ino`). USB Mode must
be **USB-OTG (TinyUSB)**. "Hardware CDC and JTAG" on core 2.0.7 holds
replies that end on an exact 64-byte packet when the host is Linux, and
OpenPnP runs on Linux. That caused the 8 s stalls found in Oct 2026. Test
anything USB-related on the Linux PnP host, because macOS doesn't show the
problem.

There are no build scripts or tests, and verification is on real hardware.
A compile check works with `arduino-cli compile -b
"esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,UploadMode=cdc,PartitionScheme=min_spiffs"
Firmware/OpenKayo`. Bump `VERSION` in `OpenKayo.ino` when releasing. It's
reported via M115 for OpenPnP autodiscovery.

## Kayo protocol essentials

- Every message is 8 bytes. Outgoing byte 0 selects the command family
  (`AA` XY move, `CC` Z move, `DD BB` rotate, `DD CC` vac/blow,
  `EE BB` home/reset, `EE CC` conveyor, `EE DD` conveyor width, `BB` lights
  and feeders). Byte 7 is always `0x55`.
- Replies start with `0xAA` and end with `0x55`. The receiver frames them by
  sliding an 8-byte window until both bytes line up. This isn't robust:
  `0x55` can appear inside a payload.
- `sendMessageToKayo()` discards stale bytes, sends, and blocks until a
  reply matches `g_expected_response` or `g_kayo_timeout_ms` passes. A
  `0x00` byte in the expected pattern is a wildcard unless
  `exact_match_required` is set. It returns false on timeout, and it
  sends nothing once `g_command_failed` is set.
- Each `cmdXxx()` must set `g_expected_response` before it sends.
  `clearExpectedResponse()` resets it after a match.
- **Exactly one response per G-code line.** `processGCodeMessage()` prints
  the single "ok" at the end, unless `reportError()` already printed an
  "ERR:" line and set `g_command_failed`. Never print "ok" or "ERR:"
  anywhere else; use `reportError()`. An extra "ok" makes OpenPnP treat its
  next command as finished before it has run. The old firmware sent two
  for every G0 with XY and rotation, and ten for M22.
- Timeouts: `KAYO_RESPONSE_TIMEOUT` by default, `KAYO_LONG_TIMEOUT` for
  G28 and M20–M28, both set in `processGCodeMessage()`.

## Machine behaviour to keep in mind

- Coordinates: home (0,0) is the top-right corner, so the whole working area
  is negative. XY units are mm × 100 (`KAYO_SCALE_FACTOR`).
- The Kayo won't move XY while any nozzle is extended. It retracts all
  nozzles on an XY command. The firmware explicitly retracts tracked nozzles
  first.
- Nozzles 1/2 and 3/4 are ganged pairs. Moving one fully retracts its
  partner.
- Z: 0 = fully retracted, around -9.8 to -10.1 mm touches the PCB.
  Z steps/mm differ between machines (110 vs 220, see `config.h`). Moving
  to exactly 0 is special-cased to send 0 steps.
- Rotation is relative on the Kayo (direction + microsteps, 3200/rev), so
  the firmware tracks absolute angle itself in `g_nozzle_angle[]`.
- Only one feeder can be open at a time. M601 closes whichever one is open.
- Blow-off also needs the "supplement" valve (`EE CC 00 08/09`). The
  existing spelling in code is `Suppliment`.
- Axes exposed to OpenPnP: X, Y; Z (virtual, top camera); A–D rotation
  for nozzles 1–4; I–L Z for nozzles 1–4.

## Known quirks / gotchas in the current code

- "Absent" G-code parameters: X/Y use `NAN` (check with `isnan()`), Z and
  rotation axes use `-9999`. Some M-code parameters still use `NULL` (0)
  as "absent", which is only safe where 0 isn't a valid value (feeder,
  nozzle, conveyor width).
- The README's conveyor table lists "M2" for reset conveyor. The code uses
  **M20** (M2 is close-vac/open-blow).
- M28 (ingest PCB from upstream conveyor via CAN) exists in code but is
  not documented in the README.
- M114 reports the last decoded or stored position. It doesn't query the
  machine.
- CAN is disabled by default (`ENABLE_CAN false`). The Nov 2025 S3 port
  commit notes CAN as "partly broken", but `Notes.md` says the bus
  hardware is validated.
- `DEBUG_KAYO` defaults to `false`. Turn it on for diagnosis only: the
  debug text shares the serial link that OpenPnP parses.
- XY moves are acknowledged on the first reply with `A1` in byte 1, which
  may be an intermediate position report rather than "move complete". This
  hasn't been verified.

## Resolved: OpenPnP stall fix (Oct 2026)

Merged into `main` on 2026-10-23 (`VERSION "0.3-20261022-stall-fix"`,
commit 83c86fc).

**Symptoms before the fix:** about 2% of feeder commands (nearly all M601
after a pick) stalled for OpenPnP's full 8 s timeout. There was also a
"stuck" state: XY moves returned "ok" in about 55 ms but the head didn't
move. Each episode started on the first XY move after a pick from feeder
N33 (`G0 I-11.3`, the deepest pick), and only after 3 or more stalls
since the last home.

**Root causes:**
1. USB: "Hardware CDC and JTAG" on core 2.0.7 holds replies whose last
   USB packet is exactly 64 bytes when the host is Linux. Fixed by building
   with USB-OTG (TinyUSB). `usb64-test.py` reproduces it.
2. Extra "ok"s (two per G0 with XY and rotation, ten for M22) made OpenPnP
   treat moves as finished early. A side effect was that vision photos were
   taken while the head was still moving.

**Verification:** the TRACE log of the 23 Oct job (3 full boards, 7,751
commands) had one "ok" per send, in order, with no `ERR:` lines, no
stalls, and no stuck runs. Feeder M-codes took a median of 3 ms. Vision
photos are now taken while the part is stationary.

**Still open:** that job didn't use N33 or the −11.3 mm pick, so the exact
stuck-state trigger wasn't exercised. Zero stalls means the suspected
precondition (OpenPnP running ahead after a stall) can't occur, but this
isn't proven. If the stuck state comes back, read the TRACE lines around
the first fast XY move and try a shallower N33 pick. Also consider that
the XY expected reply `AA A1 00 00 00 00 00 55` matches *any* `A1` reply,
which may be an intermediate position report.

**Log analysis method** (if needed again): parse `GcodeDriver DEBUG: ...
>> CMD, timeout`, `ReaderThread TRACE: ... << line` and `GcodeDriver TRACE:
... confirmed CMD`. Check sends equal `ok` + `ERR` lines, confirms come in
order, no gap of 5 s or more (except G28, M20–M28), and no run of 5 or
more `G0 X/Y` moves confirmed in under about 70 ms. Fiducial-locator nudges
of under 2 mm can legitimately confirm in under 50 ms.

**OpenPnP settings:** GcodeDriver Command Timeout must be **65000 ms**,
above the firmware's 60 s `KAYO_LONG_TIMEOUT`. Command Error Regex is
`^ERR:.*`. OpenPnP runs on host `kayo1` (Linux, `/dev/ttyACM0`). The user
copies its logs into the repo for analysis.

## Follow-ups

- README: the conveyor table says M2 for reset conveyor, but it's M20. M28
  is undocumented.
- `cmdRotateNozzle` sends a Kayo message even when the angle doesn't
  change. Skipping those would save a round trip on most moves.
- OpenPnP's safe Z of -1.0 is 960 steps, several mm below full retract,
  because of `KAYO_N1_Z_STEP_BASE` 850.
- `CANTest.ino` sets pixel index 1 with only one LED, so the LED never
  lights.
- The `Notes.md` strapping-pin to-dos.

## Conventions

- Match the existing C style: `g_` prefix for globals, `cmd` prefix for
  machine actions, `#if DEBUG_KAYO` around debug prints, the hex message
  layout documented in a comment above each function.
- Keep the README G-code tables in sync with `gcode.h` and the header
  comment in `OpenKayo.ino`. All three list the commands.
- Licensing: hardware under TAPR OHL, software under Simplified BSD.
