# Front-aligned reservoir: 40 mm high

Revision of `../front-reservoir-50mm/`, which is preserved unchanged.
Exterior is 50 x 50 x 40 mm, with 2 mm walls and floor. Interior is
46 x 46 x 38 mm, giving 80.408 mL theoretical brim capacity.
The exterior reference notch now marks 10 mL, 6.726 mm above the cup underside.
Calibrate actual printed capacity with measured water.

At 10 mL, liquid is only 4.726 mm deep. Dispensing 10 mL total will require
additional starting liquid to cover residual volume and keep the tip submerged;
the amount depends on tip clearance and pickup geometry. Lowering the walls
does not reduce the flat-bottom reservoir's residual volume.

The footprint, collar, deck, plate position and mounting holes are unchanged.
Cup centre is CAD (183,52.26), with its front at Y27.26 aligned with the nominal
plate footprint front (not the holder's flared lip). Plate centre is (76,70).
Installed cup underside is Z3.2, internal floor Z5.2, and rim Z43.2.
CAD coordinates are not taught machine coordinates. Verify pickup and travel
clearances before operation.

Print `LH_deck_four_bolt.stl` flat and `LH_reservoir_40mm.stl` upright separately.
The previous 50 mm cube's holder can be reused; only the cup needs reprinting.
STEP solids and an assembly STEP are also provided. `LH_deck.stl` is an
intermediate base-deck output, not the recommended four-bolt version.
Use a solid floor, not vase mode; water-test for leaks over a tray.
Physical fit, load and leak testing remain outstanding.

Rebuild with Python, CadQuery, trimesh, numpy and matplotlib:

```sh
python build_liquid_handler_deck.py
python build_four_bolt_deck.py
python render_deck.py LH_deck_four_bolt
python verify.py
```

`verification.json` records runtime versions and source/output hashes, valid
single solids, zero cup/deck overlap, clear mounting bores, front alignment,
capacity, closed meshes and 220 x 220 mm print-area fit.
