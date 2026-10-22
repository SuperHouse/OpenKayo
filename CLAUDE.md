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

## Current work: OpenPnP stall fix (started 2026-10-22)

**Status:** branch `fix/usb-stall-single-ok`, committed by the user, not
yet merged into `main`. The firmware (`VERSION "0.3-20261022-stall-fix"`)
is flashed on the board connected to the machine. **Next step:** analyse
the TRACE-level OpenPnP log from the next production job. If it checks
out, merge the branch, after asking the user first.

**Symptoms before the fix:**
- About 2% of feeder commands stalled for OpenPnP's full 8 s command
  timeout, nearly all of them M601 straight after the nozzle came up from a
  pick. There were 47 stalls in about 2 h on 20 Oct and 43 on 22 Oct.
- A "stuck" state: XY moves returned "ok" in about 55 ms but the head
  didn't move, while Z and the lights still worked. Recovery meant stopping
  the job, unloading the board, G28, M21 and reloading. There were 6
  episodes on 22 Oct. Each started on the first XY move after a pick from
  feeder N33 (`G0 X-187.21 Y-13.6`, then `G0 I-11.3`, the deepest pick).
  It never happened on the first N33 pick after homing, and only after at
  least 3 stalls since the last home.

**Root causes found:**
1. USB. With core 2.0.7 in "Hardware CDC and JTAG" mode, a Linux host
   holds a reply whose last USB packet is exactly 64 bytes (Linux cdc-acm
   appears to read in 128-byte chunks). With debug on, M601's reply tail
   came to exactly 64 bytes. `usb64-test.py` on kayo1 showed 64 B stalled
   15/20 and 192 B 6/20, with every other size 0/20. TinyUSB gave 0 stalls
   at every size. macOS never shows the problem.
2. Extra "ok"s. The old firmware sent two "ok"s for every G0 with XY and
   rotation, and ten for M22. Each extra "ok" confirmed OpenPnP's next
   command before it had run, so OpenPnP's idea of "move finished" ran
   early.

The suspected link to the stuck state is that OpenPnP ran ahead of the
firmware after a stall. **This is not proven**, and confirming it is the
point of the next job log.

**Verified on the machine with the fix:**
- One "ok" per command.
- `pick-loop-test.py` stalls: 0.
- `G0 X0 Y0` goes home.
- `G0 Y-50` keeps X.

**OpenPnP settings:** GcodeDriver Command Timeout must be **65000 ms**,
above the firmware's 60 s `KAYO_LONG_TIMEOUT`. There are no per-actuator
timeouts. Command Error Regex is `^ERR:.*`, already set. OpenPnP runs on
host `kayo1` (Linux, port `/dev/ttyACM0`). The user copies its logs into
the repo for analysis.

**How to analyse the next log.** Parse `GcodeDriver DEBUG: ... >> CMD,
timeout` (sent), `ReaderThread TRACE: ... << line` (from the firmware) and
`GcodeDriver TRACE: ... confirmed CMD`. With `DEBUG_KAYO` off, the firmware
sends only `ok` / `ERR: ...` lines, with no `GCODE:` echo. Check these:
1. The number of `>>` sends equals the number of `<< ok` plus `<< ERR`
   lines. Each `confirmed X` should come after its own `>> X`, and not 1 ms
   after a later send.
2. There are no stalls: no send-to-confirm gap of 5 s or more, except G28
   and M20–M28.
3. Every `ERR:` line is explained. "Kayo response timeout" is new
   information: the Kayo itself didn't reply. Note which command it was and
   the commands before it.
4. There are no stuck runs: no 5 or more consecutive `G0 X/Y` moves
   confirmed in under about 70 ms. Normal moves take 250–800 ms, and the
   move from N33 to the camera takes about 785 ms. Also look for
   unexpected G28 recoveries.
5. G28 and the conveyor commands finish within 60 s. M23 took up to about
   26 s before.

**Merge criteria:** a full production run (several boards) with checks 1–4
clean. Then merge `fix/usb-stall-single-ok` into `main`.

**If the stuck state still happens:** read the TRACE lines around the first
fast XY move to see what the Kayo replied. Next, try a shallower pick
depth for N33, to test whether the deep pick makes the Kayo think a
nozzle is still down. Also consider whether the XY "ok" comes too early:
the expected reply `AA A1 00 00 00 00 00 55` matches *any* `A1` reply,
which may be an intermediate position report.

**Follow-ups, separate from this branch:**
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
