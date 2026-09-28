"""Top view oriented like the user's photo: CAD +X up, +Y left."""
from pathlib import Path
import numpy as np
import trimesh
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.collections import PolyCollection
from matplotlib.patches import Rectangle
import build_liquid_handler_deck as base

out=Path(__file__).resolve().parent
fig,ax=plt.subplots(figsize=(8,9),dpi=160)
for filename,offset,color in [
    ('LH_deck_four_bolt.stl',[0,0,0],'#4295aa'),
    ('LH_reservoir_40mm.stl',[*base.RC,base.BASE],'#e5a047')]:
    mesh=trimesh.load_mesh(out/filename)
    tri=mesh.triangles+np.array(offset)
    tri=tri[np.argsort(tri[:,:,2].mean(axis=1))]
    polygons=np.stack([-tri[:,:,1],tri[:,:,0]],axis=-1)
    ax.add_collection(PolyCollection(polygons,facecolors=color,edgecolors='none'))
# Open cup cavity, shown in lighter amber in this plan view.
ax.add_patch(Rectangle((-base.RC[1]-base.CUP_INSIDE/2,
                       base.RC[0]-base.CUP_IN_LENGTH/2),
                      base.CUP_INSIDE,base.CUP_IN_LENGTH,
                      facecolor='#ffe3b2',edgecolor='#b87924',linewidth=1))
ax.add_patch(Rectangle((-base.PC[1]-base.PLATE_W/2,
                       base.PC[0]-base.PLATE_L/2),base.PLATE_W,base.PLATE_L,
                      facecolor='#edf1f4',edgecolor='#536674',linestyle='--'))
ax.text(-base.PC[1],base.PC[0],'96-well\nplate',ha='center',va='center',fontsize=16)
ax.text(-base.RC[1],base.RC[0],'Reservoir\n50 × 50',ha='center',va='center',fontsize=11)
ax.plot([-180,-25],[12.12,12.12],color='#555555',linestyle=':',linewidth=1)
ax.set(xlim=(-198,0),ylim=(-10,216),aspect='equal')
ax.set_axis_off()
ax.set_title('Corrected position — oriented like your photo',pad=16,fontsize=14)
ax.text(-99,-5,'Front / bottom of your photo',ha='center',fontsize=11)
fig.tight_layout()
fig.savefig(out/'photo_orientation_top.png',facecolor='white')
