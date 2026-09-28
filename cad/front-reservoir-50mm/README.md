# Front-aligned 50 mm reservoir

This revision preserves the earlier designs in `../side-reservoir/`.
Reservoir exterior is 50 x 50 x 50 mm. Walls and floor are 2 mm, giving
a 46 x 46 x 48 mm cavity and 101.568 mL theoretical brim capacity.
The shallow exterior mark now indicates 90 mL (44.533 mm above cup bottom),
leaving 5.467 mm headspace. At 100 mL only 0.741 mm headspace remains:
do not treat 100 mL as a practical moving-bed fill level. Calibrate after printing.

The cup sits to the right of the plate. Its exterior front edge aligns with
the nominal plate footprint front edge, NOT the flared holder lip: CAD Y27.26.
Cup centre is (183,52.26), plate centre remains (76,70). Four 4.5 mm mounting
holes remain on the original 170 x 170 mm square. Cup underside sits at Z3.2;
installed rim is Z53.2. All coordinates are CAD datums, not machine positions.
Re-teach reservoir pickup and check raised rim clearance before robot motion.

Print `LH_deck_four_bolt.stl` flat and `LH_reservoir_50mm.stl` upright separately.
Individual STEP solids and an assembly STEP are included. `LH_deck.stl` is
the base builder's intermediate output, not the recommended four-bolt deck.
Use a solid floor, not vase mode; leak-test with water over a tray and test
gently for sloshing. Physical fit, load and leak tests have not been performed.

Rebuild with Python, CadQuery, trimesh, numpy and matplotlib:

```sh
python build_liquid_handler_deck.py
python build_four_bolt_deck.py
python render_deck.py LH_deck_four_bolt
python verify.py
```

Verification checks valid single solids, no cup/deck solid overlap, clear bolt
bores, front-edge alignment, capacity, closed meshes and 220 x 220 mm print-area
fit. `verification.json` records results, runtime versions and source/output hashes.
