from pathlib import Path
import struct
import sys
import numpy as np
import trimesh
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
OUT=Path(__file__).resolve().parent
def load(name,offset):
    mesh=trimesh.load_mesh(OUT/name)
    vertices,faces=trimesh.remesh.subdivide_to_size(mesh.vertices,mesh.faces,max_edge=5,max_iter=8)
    return vertices[faces]+offset
stem=sys.argv[1] if len(sys.argv)>1 else 'LH_deck'
ymax=194 if stem=='LH_deck_four_bolt' else 140
triangles=[load(stem+'.stl',np.array([0,0,0])),load('LH_reservoir_100mL.stl',np.array([105,151,3.2]))]
fig=plt.figure(figsize=(12,8),dpi=170,facecolor='#f5f6f8')
ax=fig.add_subplot(projection='3d'); ax.set_facecolor('#f5f6f8')
el,az=np.deg2rad([48,-60]); view=np.array([np.cos(el)*np.cos(az),np.cos(el)*np.sin(az),np.sin(el)])
light=np.array([-0.3,-0.6,0.75]);light/=np.linalg.norm(light)
all_tri=[]
all_colors=[]
for tri,color in zip(triangles,([0.25,0.61,0.73],[0.96,0.65,0.27])):
    norm=np.cross(tri[:,1]-tri[:,0],tri[:,2]-tri[:,0]);norm/=np.maximum(np.linalg.norm(norm,axis=1)[:,None],1e-12)
    keep=norm@view>1e-7;tri=tri[keep];norm=norm[keep]
    colors=(0.6+0.4*np.clip(norm@light,0,1))[:,None]*color
    all_tri.append(tri)
    all_colors.append(colors)
colors=np.concatenate(all_colors)
ax.add_collection3d(Poly3DCollection(np.concatenate(all_tri),facecolors=colors,edgecolors=colors,linewidths=.1,antialiased=False))
ax.set(xlim=(0,216),ylim=(0,ymax),zlim=(0,55));ax.set_box_aspect((216,ymax,55));ax.set_proj_type('ortho')
ax.view_init(elev=48,azim=-60);ax.set_axis_off()
fig.subplots_adjust(0,0,1,.94)
fig.text(.05,.93,'Liquid-handler deck',fontsize=23,weight='bold',color='#253441')
fig.text(.05,.885,'Open frame  •  Self-centering plate guide  •  Removable 100 mL reservoir',fontsize=12,color='#52626d')
fig.savefig(OUT/(stem+'_preview.png'),facecolor=fig.get_facecolor())
