# Liquid Handler firmware build record

## Pinned upstream source

- Repository: `https://github.com/MarlinFirmware/Marlin.git`
- Branch at checkout: `bugfix-2.1.x`
- Commit: `ed5863d337df50652520ecc2d057d8b27749acb6`
- Commit date: `2026-09-22T06:30:09+00:00`
- Commit subject: `[cron] Bump distribution date (2026-09-22)`

The commit hash, rather than the moving branch name, is the reproducible source
identifier. To recreate the source checkout:

```sh
git clone https://github.com/MarlinFirmware/Marlin.git marlin
cd marlin
git checkout --detach ed5863d337df50652520ecc2d057d8b27749acb6
```

Apply the tracked changes to `Marlin/Configuration.h` and
`Marlin/Configuration_adv.h` and `Marlin/Version.h`, then build with:

```sh
buildroot/bin/mftest -a -n1
```

## Hardware assumptions

- Creality V4.2.2 with GD32F303RET6, selected as
  `BOARD_CREALITY_V422_GD32_MFL`.
- H8 / HR4988 fixed-step drivers are configured with A4988-compatible timing.
- Stock Ender 3 X/Y/Z mechanics: 235 x 235 x 250 mm.
- Stock Creality 12864 rotary-encoder LCD (`CR10_STOCKDISPLAY`).
- No extruders, hotends, temperature sensors, or heater outputs.
- The original fan output remains available with the board-required software
  PWM; it is not used as a heater output.
- E0 stepper socket is auto-assigned to the fourth linear axis, exposed in
  G-code as `A`.
- R6 A-axis calibration: 3200 steps/mm for the new 1 mm pitch screw, calculated
  as 1600 * 2 / 1 from the user-confirmed old 2 mm and new 1 mm pitches.
  This assumes equal numbers of starts and unchanged motor/driver/gearing.
  The new value has not yet been verified by physical measurement.
- Historical A-axis calibration: 1600 steps/mm, measured on 2026-09-25. Repeated complete
  upward moves of 3200 pulses each produced 2 mm of measured travel. This
  supersedes the earlier 320 steps/mm estimate and the 3200 steps/mm value
  found in EEPROM. Downward moves that stopped early were not used to calibrate.
  A commanded 2 mm upward move at A1600 was confirmed physically. Saved with
  M500 and reloaded with M501 on 2026-09-25; M503 confirmed A1600 after reload.
  See `../artifacts/direct_ender_diagnostic.log` (save-and-reload-a1600).
- A travel is provisionally 0-100 mm, homing toward minimum at 1 mm/s.
- A-min switch uses PB1 (connector silkscreen OUT) and GND, internal pull-up,
  triggered HIGH. The center IN pin is PB0 and must not be used for this switch.
- R5 sets INVERT_Z_DIR=true and INVERT_I_DIR=false following explicit physical
  tests that positive Z and A jogs in the direction-fix build moved down/toward
  home. Confirm positive jogs now move up/away before homing. HOME_DIR remains -1.

## Mandatory checks before homing or dispensing

1. With motors disabled, issue `M119` and confirm A reports open away from the
   switch and triggered when the switch is pressed. Reverse
   `I_MIN_ENDSTOP_HIT_STATE` if necessary.
2. Jog A a very small distance away from the switch. If motion is reversed,
   change `INVERT_I_DIR` before attempting `G28 A`.
3. Confirm the actual collision-free syringe travel. Reduce `I_MAX_POS` if it
   is less than 100 mm.
4. Calibrate plunger displacement gravimetrically before relying on volume.

No firmware should be flashed until the build succeeds and these assumptions
have been reviewed.

## Historical R5 build (before the 1600 steps/mm correction)

- Environment: `GD32F303RE_creality_mfl`
- Result: success
- RAM: 8,880 / 65,536 bytes (13.5%)
- Flash: 125,560 / 495,616 bytes (25.3%)
- Binary size: 125,956 bytes
- Startup splash revision: `Biokea R5` (also included in M115).
- Machine name: `Biokea R5`; normal status message: `Biokea R5 Ready.`
- Stable artifact:
  `../artifacts/Biokea-R5.bin`
- SHA-256:
  `2276a95c4f06053e8e4f2dd276f99fedfbee0bbdf43fc9ffc7e58f39efebbf9f`
- ELF vector-table address: `0x08007000` (the required Creality bootloader
  offset)
- Embedded M115 identity: `MACHINE_TYPE:Biokea R5`,
  `EXTRUDER_COUNT:0`, `AXIS_COUNT:4`

The binary has been built and inspected, but not flashed.

For the historical 1600 steps/mm calibration revision, either reset all persisted settings
to the compiled defaults with `M502` followed by `M500`, or update only the A
axis with `M92 A1600` followed by `M500`. Use `M503` to confirm the active value.
The historical binary above predates this correction; changing EEPROM through
G-code does not require reflashing that binary.


## R6: 1 mm pitch syringe screw (2026-09-28)

- Build command: `pio run -e GD32F303RE_creality_mfl` from `marlin/`.
- Result: success; RAM 8,880 / 65,536 bytes, flash 125,560 / 495,616 bytes.
- Artifact: `../artifacts/Biokea-R6-A3200.bin` (125,956 bytes).
- SHA-256: `ce92f445f20447e1bfb9e2d2751ea4ebe9ca8ec205039f403e4ad2d62d31b172`.
- Identity: `Biokea R6`; default XYZ/A steps/mm: 80, 80, 400, 3200.
- Binary inspection verified the compiled calibration, R6 identity, and reset
  vector within the image linked at the Creality bootloader offset 0x08007000.
- Built and inspected only; not flashed or motion-tested.

Copy the binary to the printer SD card root with a filename distinct from the
previous flash, then boot the printer with that card to install it. Existing
EEPROM settings override compiled defaults. With the machine idle, run
`../artifacts/SET_A3200.gcode` after flashing, or send its commands through the
trusted controller job queue. It sets only A steps/mm, saves with M500, reports
with M503, and ends with M400 / M84. Confirm A3200 in the report and Biokea R6
with M115. Re-home before trusting any coordinates.

Compatibility: `m5stack-controller/src/main.cpp` and `sd-card/PLATE100.gcode`
still explicitly send M92 A1600. Those existing workflows retain their old
coordinate scale and will override R6's A3200 setting at runtime. Before using
them with the new physical-mm scale, update the override and re-teach syringe
positions / regenerate the SD job together. Do not simply replace M92 in an
existing dispensing job while keeping its A coordinates: doubling steps/mm
would double its commanded physical travel. The saved A57.333 full position
was captured at A1600 on the finer screw and is not valid unchanged at A3200.
Controller firmware and historical SD jobs were not modified by this build.
