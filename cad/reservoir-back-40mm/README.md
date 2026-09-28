# Reservoir moved back 40 mm

Revision of `../reservoir-photo-position/LH_deck.stl`, using its CadQuery source.
"Back" follows the existing photograph orientation: CAD +X / upward in the
plan view. Reservoir center moves from (37.12, 151) to (77.12, 151) mm.
The collar, support cross ribs, and its two frame connectors move together.
Plate nest and mounting-feature definitions are retained. Cup dimensions and
capacity are unchanged; the existing reservoir cup can be reused.

- `LH_deck.stl`: revised version of the exact base-deck file requested.
- `LH_deck.step`: matching editable solid.
- `LH_deck_assembly.step`: deck with the cup installed in the new position.
- `reservoir_back_40mm_top.png`: plan view with the old position dashed.
- `LH_deck_four_bolt.*`: companion four-bolt variant, also rebuilt.

The original revision has been preserved. Base-deck bounds are approximately
202.58 × 174.70 × 14.20 mm. Geometry/mesh checks pass: valid connected solids,
watertight positive-volume meshes, no installed cup interference for the
four-bolt assembly, and clear four-bolt bores. A STEP Boolean comparison confirms
all added/removed base-deck material lies outside the plate nest, in the
reservoir-side region (CAD Y > 115 mm). See `verification.json` for details.

Rebuild from this directory using Python with CadQuery, trimesh and matplotlib:

```sh
python build_liquid_handler_deck.py
python build_four_bolt_deck.py
python verify.py
python render_photo_top.py
python render_deck.py LH_deck
```
