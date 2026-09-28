# Corrected front-side reservoir

Corrects the reversed front/back interpretation in `../front-reservoir-40mm/`.
The user's front is the positive-Y end of the plate in source CAD coordinates.
The reservoir stays on the same side (X183), but moves from Y52.26 to Y87.74.
Its +Y edge aligns with the plate's +Y footprint edge at Y112.74.
The nominal plate centre remains (76,70). These are CAD, not machine coordinates.

Reservoir remains 50 x 50 x 40 mm externally, with 2 mm walls and floor.
Capacity is 80.408 mL brim-full. The exterior mark indicates 10 mL, corresponding
to only 4.726 mm liquid depth. Allow extra starting liquid to keep the pickup
submerged while dispensing a full 10 mL. Physical fit and leak tests remain.

The cup geometry is unchanged and can be reused. The deck's locating collar
and its two support links have moved; print the new `LH_deck_four_bolt.stl`.
Plate nest and four mounting holes are unchanged. Print the deck flat.
`LH_reservoir_40mm.stl` is included for convenience; print it upright if needed.
Individual STEP solids and assembly STEP are also included. `LH_deck.stl` is
the intermediate base deck, not the recommended four-bolt version.
Installed cup rim remains Z43.2 above deck underside. Re-teach reservoir XY
and check travel clearance before use. Earlier revisions are preserved.

Rebuild using Python with CadQuery, trimesh, numpy and matplotlib:

```sh
python build_liquid_handler_deck.py
python build_four_bolt_deck.py
python render_deck.py LH_deck_four_bolt
python verify.py
```

Verification records valid single solids, no cup/deck interference, clear bolt
bores, corrected edge alignment, capacity, closed STL meshes, printable bounds
and source/output hashes in `verification.json`.
