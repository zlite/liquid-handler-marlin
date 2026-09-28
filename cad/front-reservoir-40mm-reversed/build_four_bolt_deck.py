"""Extend the liquid-handler deck to the original Ender-3 170 mm bolt square."""
from pathlib import Path
import cadquery as cq
from cadquery import exporters
import build_liquid_handler_deck as base

HERE=Path(__file__).resolve().parent
BOLT_X=(20.0,190.0)
BOLT_Y=(12.0,182.0)
HOLE_D=4.5

def deck():
    s=base.deck()
    # Close the front general-purpose strap slots before cutting exact bolt holes.
    for x in (16,194):
        plug=(cq.Workplane('XY',origin=(x,12,0)).slot2D(14,4.5).extrude(base.BASE).val())
        s=s.fuse(plug)
    for x in BOLT_X:
        s=s.fuse(base.box(x-4,120,0,8,62,base.BASE))
        s=s.fuse(base.slab(x,182,20,16,0,base.BASE))
    # A sparse end tie resists racking between the two extension arms.
    s=s.fuse(base.box(20,171,0,170,6,base.BASE))
    for x in BOLT_X:
        for y in BOLT_Y:
            s=s.cut(cq.Solid.makeCylinder(HOLE_D/2,base.BASE+2,cq.Vector(x,y,-1)))
    return s.clean()

if __name__=='__main__':
    s=deck(); assert s.isValid() and len(s.Solids())==1
    b=s.BoundingBox()
    print(f'Deck: {b.xlen:.2f} x {b.ylen:.2f} x {b.zlen:.2f} mm; {s.Volume()/1000:.2f} cm3')
    for ext in ('step','stl'):
        exporters.export(s,str(HERE/f'LH_deck_four_bolt.{ext}'),tolerance=.04,angularTolerance=.1)
    a=cq.Assembly(name='Four_bolt_deck')
    a.add(s,name='Deck',color=cq.Color(.25,.61,.73))
    a.add(base.reservoir(),name='Reservoir',loc=cq.Location(cq.Vector(*base.RC,base.BASE)),color=cq.Color(.96,.65,.27))
    a.export(str(HERE/'LH_deck_four_bolt_assembly.step'))
