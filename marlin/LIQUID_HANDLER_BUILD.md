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
- A-axis calibration: 1600 steps/mm, measured on 2026-09-25. Repeated complete
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

After installing this calibration revision, either reset all persisted settings
to the compiled defaults with `M502` followed by `M500`, or update only the A
axis with `M92 A1600` followed by `M500`. Use `M503` to confirm the active value.
The historical binary above predates this correction; changing EEPROM through
G-code does not require reflashing that binary.
