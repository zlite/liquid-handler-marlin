# Side-reservoir revision

Based on the original four-bolt deck from
`/home/chris/Documents/Codex/2026-09-21/can/outputs`.
Unmodified source snapshots are in `original/`; existing files were not overwritten.

The reservoir now runs alongside the long edge of the plate, on the +Y side
of the CAD model (formerly the open rear bay). Plate nest geometry, plate
centre (76,70), seating height 3.2 mm and four mounting holes on a 170 mm square
are unchanged. Reservoir centre moves from (183,70) to (105,151).
These are CAD coordinates, not taught machine coordinates.

Reservoir outside dimensions: **104 x 52 x 25 mm**, formerly 52 x 52 x 50 mm.
Walls remain 2 mm. Floor is 1.96 mm rather than 2 mm to preserve the exact
110.592 mL theoretical brim capacity while meeting the exterior size ratios.
Working volume remains 100 mL. The exterior reference mark is 22.7933 mm above
the cup underside. Installed cup rim is 28.2 mm above deck underside, with
the internal floor at 5.16 mm. Actual printed volume requires calibration;
layer rounding will slightly alter these dimensions. Headspace at 100 mL is
only 2.207 mm, so test gently for spills during bed motion.

Print `LH_deck_four_bolt.stl` flat and `LH_reservoir_100mL.stl` upright as
separate parts. Corresponding STEP files are editable solids; the assembly
STEP shows placement and is not a single printable part. The base builder's
`LH_deck.stl` is an intermediate, not the recommended four-bolt deck.
Keep the cup floor solid (about 10 layers at 0.2 mm), not vase mode.
Water-test over a tray before use. No physical fit, leak or load tests performed.
The larger, shallower cup remains removable, with 0.6 mm total collar clearance.
Re-teach reservoir pickup coordinates and verify safe Z clearance before motion.

## Rebuild and validate

Requires Python, CadQuery, trimesh, numpy and matplotlib.

```sh
python build_liquid_handler_deck.py
python build_four_bolt_deck.py
python render_deck.py LH_deck_four_bolt
python verify.py
```

`verification.json` records runtime versions, source/output hashes, calculated
capacity, mesh bounds, watertightness, mounting bore checks and zero cup/deck
solid interference. Preview is `LH_deck_four_bolt_preview.png`.
