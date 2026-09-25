# 96-well teaching controller

M5Stack Core Basic + USB Module V1.2 + Ender Liquid Handler R5.
The controller provides a browser page for homing, incremental XYZ jogging,
and recording a standard 8 × 12, 9 mm pitch plate and reservoir.
It does not move automatically at boot. Dry runs leave the syringe stationary;
Home All and the explicit A diagnostics can home/move the syringe.

## Hardware and connection

USB Module switches: SS G5 ON, G0/G13 OFF; INT G35 ON, G34 OFF.
The CH340 Ender adapter (1a86:7523) runs at 250000 baud.
The controller uses USB Host Shield 2.0 with a custom CH340 driver.
Read one endpoint-sized packet at a time: larger reads can receive data then
return NAK without committing the data toggle, losing replies.

At connection, numbered/checksummed commands check firmware identification,
ten complete M114 replies, and acknowledged M400/M84. Motion controls remain
disabled until this passes. No timed-out movement is automatically replayed.
USB reconnect runs the checks again and invalidates session homing.

Open http://dispenser.local or the IP address displayed on the Core.
The network hostname is `dispenser`. The Core screen shows the current task,
connection state, IP address, session homing, last reported XYZ position, and
A limit switch (green **OPEN**, red **TRIGGERED**, yellow **UNKNOWN** when
stale or unavailable). Background switch polling does not replace task status.
It polls `M119` every 200 ms while the command queue is idle. These are
read-only queries and do not move or energize motors. Polling pauses behind
other operations; Marlin cannot service these queries during blocking homing.
Readings expire after one second, and polling can miss shorter pulses.
Use **Read limit switches** in the connection log to query `M119` without
moving. The syringe A-axis switch appears as `a_min`; the diagnostic finishes
with acknowledged motor release.
The web connection section also provides **Read motion settings** (`M503`)
and **Run A-only homing test**. The latter uses `G28 A R0` so XYZ stay put,
records switch/position reports before and after, and ends in `M400`/`M84`.
It requires a fresh OPEN switch reading. Its separate diagnostic log is at
`/api/a-homing-trace` and survives background polling (but not M5 restarts).
These standard G-code diagnostics do not capture brief triggers during homing.
The diagnostic API action `move_a_down_5` makes one 5 mm relative A move toward
home at 1 mm/s using the measured 1600 steps/mm. Because failed homing leaves
an untrustworthy A zero, this test temporarily disables software endstops;
`M120` keeps physical endstops enabled. It restores absolute mode and software
endstops, reports position/switches, and ends with `M400`/`M84`. Run only while
observing clear syringe travel. Physical endstops remain enabled afterward.
The diagnostic API action `hold_a_test` enables all drivers with `M17` without
commanding motion, reads `M119` about every 200 ms for ten seconds, then reads
position and finishes with acknowledged `M400`/`M84` release. The saved
diagnostic log includes timestamped A-switch samples and a trigger count.
Other jobs cannot interleave with the test. This cannot rule out pulses
shorter than the polling interval.
The companion `motion_a_test` uses the same ten-second sampling window after
queueing `G1 A-5 F30` (5 mm downward at 0.5 mm/s, A1600 steps/mm). It retains
physical endstops, temporarily bypasses the untrusted software A zero, and
restores absolute mode/software limits before reporting position and releasing
all motors. An endstop can stop the motion before the sampling window ends;
the samples alone must not be taken as proof of continuous movement.
If Wi-Fi does not connect, join `LiquidHandler-Teach`, password `teachplate96`,
and open http://192.168.4.1. The Core continues trying its configured network.
The page is intended for the local lab network; there is no authentication.

## Teach the deck

1. Secure the plate and reservoir. Clear the homing paths, including below
   the tip. Use the page's Home X/Y and Home Z controls.
2. Jog with 10, 1, or 0.1 mm steps. Raise Z clear of obstacles before XY moves.
3. Align over the **well centers** of A1, A12, and H1, capturing each.
   These define the grid's XY origin, rotation, and spacing.
4. Independently align over H12 and capture the check point. It must agree
   within 1 mm. Recapturing any anchor invalidates the old H12 check.
5. Capture reservoir XYZ at the aspiration location and immersion depth.
   Capture dispense Z at the desired height in a well, then travel Z at a
   height that clears every obstacle. The software checks travel Z exceeds
   the working heights; it cannot detect obstacles or liquid levels.
6. Save setup notes and export the calibration JSON. Once grid and height
   checks pass, the export includes all 96 well XYZ positions.

Each capture waits for M400, reads M114, and waits for M84 acknowledgement
before saving. Each completed jog/homing operation also ends in M400/M114/M84.
All motors release as requested. Released axes can shift without detection;
physically pushing them does not update the reported coordinates. Re-home
if that occurs. Front-panel buttons have no motion-control bindings. The
earlier button-A shortcut was removed after button events during homing
caused a false connection fault and repeated unnumbered release commands.

Points persist in ESP32 NVS across restarts; session homing does not. Recheck
alignment after moving labware or changing the tip. Dispense Z is a single
shared height, assuming a level plate. Nominal center spans are 99 and 63 mm;
geometry checks allow ±3 mm and 3 degrees of squareness. These checks catch
large teaching mistakes, not guarantee pipetting accuracy.

## Dry trial

The dry-run control visits the reservoir, then A1 through H1, and repeats for
columns 2 through 12 (12 reservoir visits and 96 distinct wells). It lowers to
the taught reservoir/dispense Z at each location. Z stays at dispense height
between the eight wells of each group; reservoir trips lift to travel height.
Requested XY speed is 500 mm/s and Z speed is 5 mm/s, matching the verified
M203 limits. Existing acceleration limits remain active, so short moves will
not reach the requested maximum. No A moves occur.
The operation is continuous: motors release only at completion or a requested
stop, with acknowledged `M400` / `M84`. One visit is queued at a time. Stop
finishes that visit and then lifts to travel Z; it is not an emergency stop. No job resumes
after a connection fault or reboot. The screen/web status names the group and
current destination; position reports update at each working location and after
the final lift.

All seven points and session XYZ homing are required. Normal H12 tolerance
remains 1 mm. The explicit trial checkbox permits up to 2 mm for that run only;
it does not mark the calibration verified or change saved coordinates. Bounds,
grid geometry, finite coordinates, and travel-height checks still apply.
After firmware installation, re-home from clear paths before starting.

Home All homes X/Y/Z/A, with a separate upward clearance move first. Individual
teaching Home X/Y and Home Z use the same pre-lift. If session Z is referenced,
the lift targets max(current Z, travel Z); otherwise it is a relative lift by
the saved travel height (currently 55 mm), explicitly approved for this machine.
The operator must leave that much free upward travel when Z is unreferenced.
The subsequent G28 uses R0 to avoid a second lift. Missing travel calibration
blocks these home controls. All sequences finish with M400 / M84. Homing speeds
remain the firmware's homing speeds, independent of dry-run feedrates.

## Build

Copy `include/network_secrets.example.h` to `include/network_secrets.h` and
set Wi-Fi credentials (this file is ignored). Without it the hotspot works.

```sh
pio run -d m5stack-controller
pio run -d m5stack-controller -t upload --upload-port /dev/ttyUSB0
g++ -std=c++11 -I m5stack-controller/include m5stack-controller/test/test_teaching.cpp -o /tmp/test-teaching
/tmp/test-teaching
```

Close serial monitors before uploading. Diagnostic serial output is 115200.
The build script selects Core Basic SS5/INT35 in the USB library.

## Validation and limits

Live Ender checks received the full R5 M115 report, ten consecutive complete
M114 replies, and M400/M84 acknowledgements; see
`../artifacts/teaching-ui-boot.log`. Twenty additional HTTP-triggered
position/release jobs passed, and unhomed jog/capture requests were rejected;
see `../artifacts/teaching-http-validation.json`. Geometry/parser tests cover rotation,
malformed replies, skew, and travel limits. Physical homing, jogging, and
labware calibration require operator observation and have not been exercised
by this installation. This supersedes the older TinyUSB probe for teaching.
