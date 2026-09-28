"""Reproducible geometry and exported mesh checks. Run after both builders."""
import hashlib
import json
import platform
import cadquery as cq
import trimesh
import build_liquid_handler_deck as base
from build_four_bolt_deck import deck, BOLT_X, BOLT_Y, HOLE_D

d, c = deck(), base.reservoir()
installed = c.translate((*base.RC, base.BASE))
assert d.isValid() and len(d.Solids()) == 1
assert c.isValid() and len(c.Solids()) == 1
assert d.intersect(installed).Volume() < 1e-6
for x in BOLT_X:
    for y in BOLT_Y:
        bore = cq.Solid.makeCylinder(HOLE_D/2-0.001, base.BASE+2, cq.Vector(x,y,-1))
        assert d.intersect(bore).Volume() < 1e-6
capacity = base.CUP_IN_LENGTH*base.CUP_INSIDE*(base.CUP_HEIGHT-base.CUP_BOTTOM)/1000
assert abs(capacity-80.408) < 1e-9
assert abs(base.RC[0]-base.CUP_LENGTH/2-(base.PC[0]-base.PLATE_L/2)) < 1e-9
report = {'cadquery': cq.__version__, 'python': platform.python_version(),
          'brim_capacity_ml': capacity, 'reference_fill_ml': 10,
          'photo_bottom_alignment_cad_x_mm': base.RC[0]-base.CUP_LENGTH/2,
          'outside_cup_mm': [base.CUP_LENGTH,base.CUP_OUT,base.CUP_HEIGHT],
          'floor_mm': base.CUP_BOTTOM, 'cup_center_cad_xy': base.RC,
          'plate_center_cad_xy': base.PC, 'assembly_interference_mm3': 0,
          'mounting_holes_clear': True, 'meshes': {}, 'sha256': {}}
for name in ('LH_deck_four_bolt','LH_reservoir_40mm'):
    mesh = trimesh.load_mesh(base.HERE/(name+'.stl'))
    assert mesh.is_watertight and mesh.is_volume
    assert mesh.extents[0] <= 220 and mesh.extents[1] <= 220
    report['meshes'][name] = {'watertight': True, 'bounds_mm': mesh.extents.tolist()}
for path in sorted(base.HERE.rglob('*')):
    if path.suffix in ('.py','.stl','.step'):
        report['sha256'][str(path.relative_to(base.HERE))] = hashlib.sha256(path.read_bytes()).hexdigest()
(base.HERE/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='sha256'},indent=2))
