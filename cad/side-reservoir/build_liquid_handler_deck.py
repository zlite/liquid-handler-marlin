"""Open-frame Ender-3 liquid-handler deck, mm; requires CadQuery.
Standard skirted SLAS microplate assumption; see DECK_README.md.
"""
from pathlib import Path
import json
import cadquery as cq
from cadquery import exporters

HERE=Path(__file__).resolve().parent
PLATE_L,PLATE_W=127.76,85.48
FIT=0.6  # total clearance, 0.3 per side on nominal plate
LEAD=6.0
WALL=2.4
BASE=3.2
SEAT=3.2
THROAT_Z=6.2
TOP=14.2
PC=(76.0,70.0)
RC=(105.0,151.0)
CUP_OUT=52.0
CUP_LENGTH=104.0
CUP_WALL=2.0
CUP_HEIGHT=25.0
CUP_INSIDE=CUP_OUT-2*CUP_WALL
CUP_IN_LENGTH=CUP_LENGTH-2*CUP_WALL
CUP_BOTTOM=CUP_HEIGHT-110592/(CUP_IN_LENGTH*CUP_INSIDE)
FILL_Z=CUP_BOTTOM+100000/(CUP_IN_LENGTH*CUP_INSIDE)
SOCKET_CLEARANCE=0.6

def box(x,y,z,dx,dy,dz):
    return cq.Solid.makeBox(dx,dy,dz,cq.Vector(x,y,z))

def slab(cx,cy,l,w,z,t):
    return box(cx-l/2,cy-w/2,z,l,w,t)

def deck():
    throat=(PLATE_L+FIT,PLATE_W+FIT)
    mouth=(throat[0]+2*LEAD,throat[1]+2*LEAD)
    outer=(mouth[0]+2*WALL,mouth[1]+2*WALL)
    lower_outer=(throat[0]+2*WALL,throat[1]+2*WALL)
    s=slab(*PC,*lower_outer,0,THROAT_Z)
    flare=(cq.Workplane('XY',origin=(*PC,THROAT_Z)).rect(*lower_outer)
           .workplane(offset=TOP-THROAT_Z).rect(*outer).loft().val())
    s=s.fuse(flare)
    # Open underside, leaving a 4 mm skirt-support ledge on the plate perimeter.
    s=s.cut(slab(*PC,PLATE_L-8,PLATE_W-8,-1,SEAT+1))
    s=s.cut(slab(*PC,*throat,SEAT,THROAT_Z-SEAT+0.02))
    funnel=(cq.Workplane('XY',origin=(*PC,THROAT_Z)).rect(*throat)
            .workplane(offset=TOP-THROAT_Z).rect(*mouth).loft().val())
    s=s.cut(funnel)
    # Removable cup collar with open floor and two load-bearing cross ribs.
    inner=CUP_OUT+SOCKET_CLEARANCE
    inner_l=CUP_LENGTH+SOCKET_CLEARANCE
    ring=slab(*RC,inner_l+4.8,inner+4.8,0,8)
    ring=ring.cut(slab(*RC,inner_l,inner,-1,10))
    s=s.fuse(ring,slab(*RC,CUP_LENGTH,6,0,BASE),slab(*RC,6,CUP_OUT,0,BASE))
    # Collar joins the rear frame tie; no obsolete end-reservoir links.
    # Four mounting feet, flat undersides for thin mounting tape; slots for straps
    # or fasteners into a separate fixture, never into the printer heater plate.
    for x in (16,194):
        for y in (12,128):
            foot=slab(x,y,24,16,0,BASE)
            slot=(cq.Workplane('XY',origin=(x,y,-1)).slot2D(14,4.5).extrude(BASE+2).val())
            foot=foot.cut(slot)
            s=s.fuse(foot)
    # Edge ribs tie mounting feet into each functional ring, leaving centre open.
    for x in (16,194):
        s=s.fuse(box(x-3,18,0,6,104,BASE))
    # Front/rear ties resist racking, while keeping most of the bed uncovered.
    for y in (18,122):
        s=s.fuse(box(16,y-3,0,178,6,BASE))
    return s.clean()

def reservoir():
    s=slab(0,0,CUP_LENGTH,CUP_OUT,0,CUP_HEIGHT)
    cavity=slab(0,0,CUP_IN_LENGTH,CUP_INSIDE,CUP_BOTTOM,CUP_HEIGHT)
    s=s.cut(cavity)
    # External 100 mL reference notch, 0.3 mm deep; fluid wall stays >=1.7 mm.
    s=s.cut(box(-8,-CUP_OUT/2-0.1,FILL_Z-0.25,16,0.4,0.5))
    return s.clean()

if __name__=='__main__':
    d,c=deck(),reservoir()
    gauge=slab(0,0,PLATE_L+FIT+4.8,PLATE_W+FIT+4.8,0,2)
    gauge=gauge.cut(slab(0,0,PLATE_L+FIT,PLATE_W+FIT,-1,4))
    exporters.export(gauge,str(HERE/'LH_optional_plate_fit_gauge.stl'),tolerance=0.04)
    bounds=d.BoundingBox()
    bounds_mm=[round(v,2) for v in (bounds.xlen,bounds.ylen,bounds.zlen)]
    for name,s in [('LH_deck',d),('LH_reservoir_100mL',c)]:
        assert s.isValid() and len(s.Solids())==1, name
        for ext in ('step','stl'):
            exporters.export(s,str(HERE/f'{name}.{ext}'),tolerance=0.04,angularTolerance=0.1)
    a=cq.Assembly(name='Liquid_handler_deck')
    a.add(d,name='Deck',color=cq.Color(0.25,0.55,0.68))
    a.add(c,name='Removable_reservoir',loc=cq.Location(cq.Vector(*RC,BASE)),color=cq.Color(0.9,0.67,0.28))
    a.export(str(HERE/'LH_deck_assembly.step'))
    metrics={'deck_bounds_mm':bounds_mm,
             'deck_solid_volume_cm3':d.Volume()/1000,'cup_solid_volume_cm3':c.Volume()/1000,
             'reservoir_brim_ml':CUP_IN_LENGTH*CUP_INSIDE*(CUP_HEIGHT-CUP_BOTTOM)/1000,
             'fill_100ml_height_from_cup_bottom_mm':FILL_Z,
             'plate_seating_z_mm':SEAT,'reservoir_rim_z_mm':BASE+CUP_HEIGHT,
             'plate_center_xy_mm':PC,'reservoir_center_xy_mm':RC,
             'mouth_mm':[PLATE_L+FIT+12,PLATE_W+FIT+12]}
    (HERE/'LH_dimensions.json').write_text(json.dumps(metrics,indent=2)+'\n')
    print(json.dumps(metrics,indent=2))
