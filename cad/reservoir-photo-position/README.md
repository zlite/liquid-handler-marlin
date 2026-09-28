# Reservoir position corrected from the user's photograph

Reference: `/home/chris/Downloads/PXL_20260926_005821466 (1).jpg`.
The black bracket marks the lower-left bay alongside the plate's LONG edge,
not the short-end area used by the two previous revisions. In the photograph,
CAD +X points upward and CAD +Y points leftward.

Cup centre: CAD (37.12,151). Its lower-X edge at X12.12 aligns with the nominal
plate's lower-X edge, corresponding to the bottom/front edge in the photograph.
The lateral bay centre Y151 is retained from the original long-side layout.
The photo establishes the intended bay, not a precision measurement of bracket
position. Plate centre remains (76,70), with unchanged nest and mounting holes.

Reservoir remains 50 x 50 x 40 mm externally, walls/floor 2 mm, cavity
46 x 46 x 38 mm, theoretical brim capacity 80.408 mL. Cup geometry is unchanged
and reusable. The 10 mL mark corresponds to 4.726 mm liquid depth; extra initial
liquid is needed to keep the pickup submerged while dispensing a full 10 mL.

Print the new `LH_deck_four_bolt.stl` flat. The cup STL is included only for
convenience. Individual STEP solids and assembly STEP are provided. The base
`LH_deck.stl` is an intermediate, not the recommended four-bolt version.
Installed rim is Z43.2 above deck underside. CAD coordinates are not machine
coordinates: re-teach reservoir pickup and check clearances before operation.
Previous revisions are preserved. Physical fit/load/leak tests remain outstanding.

Rebuild with Python, CadQuery, trimesh, numpy and matplotlib:

```sh
python build_liquid_handler_deck.py
python build_four_bolt_deck.py
python render_deck.py LH_deck_four_bolt
python render_photo_top.py
python verify.py
```

`verification.json` records single valid solids, no cup/deck interference, clear
mounting bores, front alignment, capacity, closed meshes, printable bounds and
source/output hashes. `photo_orientation_top.png` uses the photograph orientation.
