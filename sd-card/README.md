# Ender SD-card plate dispensing

Copy `PLATE100.gcode` to the SD card root and select it from the Ender's
Print from Media / Print from SD menu. The file contains the current full-plate
wet run: 100 uL per well, A1-H1 through A12-H12, with twelve 1 mL fills.
It uses the live controller calibration saved on 2026-09-28 in the adjacent JSON.

Before starting, keep the M5 controller idle and ensure the labware matches
this calibration. Provide at least 75 mm of free upward Z travel and clear
all homing paths. Startup raises Z by 75 mm, then homes XYZ and the syringe;
syringe homing empties any fluid at the homing location, so start empty.
The file performs its own homing on every run.

Travel Z is 75 mm, reservoir is X72 Y158 Z40, and dispense Z is 42 mm.
The calibrated full syringe position is A57.333; A uses 1600 steps per unit.
XY requests 500 mm/s, Z 5 mm/s, and syringe A 10 commanded units/s (F600),
matching the current controller routine. Existing acceleration limits apply.
It purges at the reservoir after the final well, restores the A speed limit
to 5, raises to Z100, and ends with M400 / M84 to release all motors.

This is a fixed calibration snapshot. Export a new job after reteaching.
Keep enough liquid to deliver 9.6 mL while maintaining reservoir immersion.
Sequence checks passed; execution from the physical SD card has not been tested.
