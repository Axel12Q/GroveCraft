"""Private static surface cache from the user's original COL/IDE/IPL files.
Uses the separately installed GPL DragonFF reader; no game assets are redistributed.
NumPy rasterization happens before either game starts, never in the travel/render loop.
"""
from pathlib import Path
import json,struct,sys,hashlib,math,time,argparse
r=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game-directory',type=Path)
parser.add_argument('--output-directory',type=Path)
parser.add_argument('--dragonff',type=Path,default=r/'runtime/dragonff')
args=parser.parse_args()
import numpy as np
sys.path.insert(0,str(args.dragonff.resolve()))
from gtaLib.col import coll
g=args.game_directory.resolve() if args.game_directory else Path(json.loads((r/'artifacts/installation.json').read_text(encoding='utf-8-sig'))['GameDirectory'])
out=args.output_directory.resolve() if args.output_directory else r/'artifacts/world-surface-private';out.mkdir(parents=True,exist_ok=True)
sources={};models={};names={};instances=[]
def source(path):
 data=path.read_bytes();sources[str(path.relative_to(g))]=hashlib.sha256(data).hexdigest();return data
def text_instances(path):
 section=''
 for line in source(path).decode('latin1').splitlines():
  line=line.split('#')[0].strip()
  if not line:continue
  if ',' not in line:section=line.lower();continue
  a=[s.strip() for s in line.split(',')]
  if section=='inst' and len(a)>=11:
   try:
    model=int(a[0]);interior=int(a[2]);p=list(map(float,a[3:10]))
    if interior==0:instances.append((model,*p))
   except ValueError:pass
def read_col(data):
 c=coll();c.load_memory(data)
 for m in c.models:
  if m.model_name:models[m.model_name.lower()]=m
dat=source(g/'data/gta.dat').decode('latin1')
for line in dat.splitlines():
 a=line.split('#')[0].strip().split()
 if len(a)<2:continue
 path=g/a[-1].replace('\\','/')
 if a[0].upper()=='IDE' and path.exists():
  section=''
  for row in source(path).decode('latin1').splitlines():
   row=row.split('#')[0].strip()
   if ',' not in row:section=row.lower();continue
   parts=[v.strip() for v in row.split(',')]
   if section in ('objs','tobj','anim'):
    try:names[int(parts[0])]=parts[1].lower()
    except (ValueError,IndexError):pass
 elif a[0].upper()=='IPL' and path.exists():text_instances(path)
 elif a[0].upper()=='COLFILE' and path.exists():read_col(source(path))
image=g/'models/gta3.img'
# Hash only relevant embedded entries, rather than copying the multi-gigabyte IMG.
with image.open('rb') as f:
 assert f.read(4)==b'VER2';count=struct.unpack('<I',f.read(4))[0];directory=f.read(count*32)
 for i in range(count):
  off,size,unused,name=struct.unpack_from('<IHH24s',directory,i*32);name=name.split(b'\0')[0].decode('ascii').lower()
  if not name.endswith(('.ipl','.col')):continue
  f.seek(off*2048);data=f.read(size*2048);sources['models/gta3.img:'+name]=hashlib.sha256(data).hexdigest()
  if name.endswith('.col'):read_col(data)
  elif data[:4]==b'bnry':
   n=struct.unpack_from('<I',data,4)[0];start=struct.unpack_from('<I',data,28)[0]
   assert start+n*40<=len(data)
   for j in range(n):
    x,y,z,qx,qy,qz,qw,model,interior,lod=struct.unpack_from('<7f3i',data,start+j*40)
    if interior==0:instances.append((model,x,y,z,qx,qy,qz,qw))
print('Static source records:',len(instances),'COL models:',len(models),flush=True)
fingerprint=hashlib.sha256(('raster-v3:'+json.dumps(sources,sort_keys=True)).encode()).hexdigest()
manifest=out/'manifest.json';binary=out/'surface.bin'
if manifest.exists() and binary.exists() and json.loads(manifest.read_text())['source_sha256']==fingerprint:
 print('Surface cache already current',flush=True);sys.exit(0)
STEP=2;MIN=-3000;N=3000
highest=np.full((N,N),-np.inf,dtype='<f4');grass=highest.copy();solid=highest.copy();faces=0;seen=set();start_time=time.monotonic()
def is_grass(m):return 9<=m<=17 or 80<=m<=82 or 115<=m<=122 or m==125 or 146<=m<=153 or m==160
def is_soil(m):return is_grass(m) or 19<=m<=33 or m in (83,84,87,88) or 74<=m<=79
def raster(v,target):
 x0=max(0,math.ceil((float(v[:,0].min())-MIN-.5)/STEP));x1=min(N-1,math.floor((float(v[:,0].max())-MIN-.5)/STEP))
 z0=max(0,math.ceil((float(v[:,2].min())-MIN-.5)/STEP));z1=min(N-1,math.floor((float(v[:,2].max())-MIN-.5)/STEP))
 if x0>x1 or z0>z1:return
 a,b,c=v;den=(b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2])
 if abs(den)<1e-7:return
 xx=MIN+.5+np.arange(x0,x1+1,dtype=np.float32)*STEP;zz=MIN+.5+np.arange(z0,z1+1,dtype=np.float32)*STEP
 u=((b[2]-c[2])*(xx[None,:]-c[0])+(c[0]-b[0])*(zz[:,None]-c[2]))/den
 w=((c[2]-a[2])*(xx[None,:]-c[0])+(a[0]-c[0])*(zz[:,None]-c[2]))/den
 h=u*a[1]+w*b[1]+(1-u-w)*c[1];mask=(u>=-1e-5)&(w>=-1e-5)&(u+w<=1.00001)
 patch=target[z0:z1+1,x0:x1+1];np.maximum(patch,np.where(mask,h,-np.inf),out=patch)
for index,instance in enumerate(instances):
 if instance in seen:continue
 seen.add(instance);model,x,y,z,qx,qy,qz,qw=instance;m=models.get(names.get(model,''))
 if m is None:continue
 q=np.array([qx,qy,qz],dtype=np.float64);norm=float(np.dot(q,q)+qw*qw)
 if not .99<norm<1.01:continue
 # Box-only tree/building collision must also reserve its footprint.
 for box in m.boxes:
  corners=np.asarray([[xx,yy,zz] for xx in (box.min[0],box.max[0]) for yy in (box.min[1],box.max[1]) for zz in (box.min[2],box.max[2])])
  corners=corners+2*np.cross(q,np.cross(q,corners)-qw*corners);corners+=np.array([x,y,z])
  x0=max(0,math.floor((corners[:,0].min()-MIN-.5)/STEP));x1=min(N,math.ceil((corners[:,0].max()-MIN-.5)/STEP)+1)
  z0=max(0,math.floor((-corners[:,1].max()-MIN-.5)/STEP));z1=min(N,math.ceil((-corners[:,1].min()-MIN-.5)/STEP)+1)
  if x0<x1 and z0<z1:
   np.maximum(highest[z0:z1,x0:x1],corners[:,2].max(),out=highest[z0:z1,x0:x1])
   np.maximum(solid[z0:z1,x0:x1],corners[:,2].max(),out=solid[z0:z1,x0:x1])
 if not m.mesh_verts:continue
 v=np.asarray(m.mesh_verts,dtype=np.float64)
 # Standard quaternion rotation, followed by GTA->Minecraft coordinate conversion.
 v=v+2*np.cross(q,np.cross(q,v)-qw*v);v+=np.array([x,y,z]);v=v[:,[0,2,1]];v[:,2]*=-1
 for face in m.mesh_faces:
  mat=face.material if hasattr(face,'material') else face.surface.material
  t=v[[face.a,face.b,face.c]];normal=np.cross(t[1]-t[0],t[2]-t[0]);length=np.linalg.norm(normal)
  if length<1e-6:continue
  if abs(normal[1])/length>.05:
   raster(t,highest)
   if is_soil(mat) and abs(normal[1])/length>.65:raster(t,grass)
   elif not is_soil(mat):raster(t,solid)
  faces+=1
 if index%5000==0:print('Rasterized',index,'placements;',faces,'faces',flush=True)
# Missing grass or an obstruction above it never becomes a buildable parcel.
valid=np.isfinite(grass)&(highest-grass<=.015)&(solid<=grass+.02)
ground=np.where(valid,grass,np.nan).astype('<f4')
temp=binary.with_suffix('.tmp')
with temp.open('wb') as f:
 f.write(struct.pack('<4sIiiII',b'GSA1',1,MIN,MIN,STEP,N));f.write(ground.tobytes());f.write(highest.astype('<f4').tobytes())
temp.replace(binary)
info={'version':1,'source_sha256':fingerprint,'file_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'step':STEP,'size':N,'valid_ground_columns':int(valid.sum()),'faces':faces,'placements':len(seen),'elapsed_seconds':round(time.monotonic()-start_time,2),'private_game_derived_cache':True,'reader':'DragonFF gtaLib/col.py (GPL dependency, used locally)'}
manifest.write_text(json.dumps(info,indent=2),encoding='utf-8');print(json.dumps(info,indent=2),flush=True)
