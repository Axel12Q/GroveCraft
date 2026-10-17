"""64-bit synthetic producer -> actual x86 ASI -> hidden D3D9 depth test."""
import ctypes as c
import argparse
import json
import hashlib
import struct
import subprocess
import time
from pathlib import Path

root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--renderer-only', action='store_true')
parser.add_argument('--scene', action='store_true', help='Draw the entity scene packet instead of a block section')
parser.add_argument('--startup-timeout',type=float,default=30,help='Seconds to allow cold executable/DLL startup before the shared mapping appears')
args=parser.parse_args()
assert 1<=args.startup_timeout<=45
cwd=root/'build/host'
k=c.WinDLL('kernel32',use_last_error=True)
k.OpenFileMappingW.argtypes=[c.c_uint32,c.c_int,c.c_wchar_p]; k.OpenFileMappingW.restype=c.c_void_p
k.MapViewOfFile.argtypes=[c.c_void_p,c.c_uint32,c.c_uint32,c.c_uint32,c.c_size_t]; k.MapViewOfFile.restype=c.c_void_p
k.UnmapViewOfFile.argtypes=[c.c_void_p]
k.CloseHandle.argtypes=[c.c_void_p]
k.GetTickCount64.restype=c.c_ulonglong
existing=k.OpenFileMappingW(0xF001F,0,'Local\\GroveCraft_v1')
if existing:
    k.CloseHandle(existing)
    raise SystemExit('A GroveCraft host already owns the mapping. Close it before testing.')
result_path=cwd/'smoke-result.json'
if result_path.exists():result_path.unlink()
exe='renderer-unit.exe' if args.renderer_only else 'render-smoke.exe'
proc=subprocess.Popen([str(cwd/exe)],cwd=cwd,creationflags=0x08000000)
handle=None;ptr=None
try:
    startup_until=time.monotonic()+args.startup_timeout
    while time.monotonic()<startup_until:
        handle=k.OpenFileMappingW(0xF001F,0,'Local\\GroveCraft_v1')
        if handle:break
        if proc.poll() is not None:raise RuntimeError(f'Host failed: {proc.returncode}')
        time.sleep(.05)
    assert handle,'Host did not create mapping'
    ptr=k.MapViewOfFile(handle,0xF001F,0,0,0);assert ptr
    for _ in range(100):
        if c.c_uint32.from_address(ptr).value==0x564F5247:break
        time.sleep(.02)
    assert c.c_uint32.from_address(ptr).value==0x564F5247
    assert c.c_uint32.from_address(ptr+4).value==11
    ring=0x20000+(32<<20)+3840*2160*4*3
    def message(kind,payload):
        data=struct.pack('<II',kind,len(payload))+payload
        return data+b'\0'*((-len(data))%8)
    atlas=message(1,struct.pack('<II',1,1)+bytes([0,255,0,255]))
    vertices=b''.join(struct.pack('<fffffIII',x,y,z,0.,0.,0xffffffff,0,0) for x,y,z in [(-1,5,1),(1,5,1),(0,5,-1)])
    mesh=message(2,struct.pack('<iiiI',0,0,0,3)+vertices)
    if args.scene:
        texture=message(4,struct.pack('<IIII',1,1,1,0)+bytes([0,255,0,255]))
        mesh=message(6,struct.pack('<dddII',0.,0.,0.,1,3)+struct.pack('<IIII',1,0,3,0)+vertices)
        # Reject an invalid batch and keep the preceding complete scene.
        invalid=message(6,struct.pack('<dddII',0.,0.,0.,1,3)+struct.pack('<IIII',1,2,3,0)+vertices)
        data=atlas+texture+mesh+invalid
    else:
        data=atlas+mesh
    c.memmove(ptr+ring+0x80,data,len(data))
    # Header/counters are published before marking this synthetic peer live.
    c.c_uint64.from_address(ptr+ring).value=len(data)
    c.c_uint64.from_address(ptr+24).value=k.GetTickCount64()
    c.c_uint32.from_address(ptr+12).value=99999
    published=len(data);refresh=time.monotonic()+.3
    until=time.monotonic()+15
    while proc.poll() is None and time.monotonic()<until:
        # Real entities are streamed continuously. Keep this synthetic frame fresh
        # during both depth phases instead of tripping the renderer's 2s stale guard.
        if args.scene and time.monotonic()>=refresh:
            c.memmove(ptr+ring+0x80+published,mesh,len(mesh))
            published+=len(mesh);c.c_uint64.from_address(ptr+ring).value=published
            refresh=time.monotonic()+.3
        c.c_uint64.from_address(ptr+24).value=k.GetTickCount64();time.sleep(.05)
    assert proc.poll() is not None,'Synthetic host timed out'
    result=json.loads(result_path.read_text()) if result_path.exists() else {'exit':proc.returncode}
    result['ring_bytes_consumed']=c.c_uint64.from_address(ptr+ring+0x40).value
    result['state_seq']=c.c_uint32.from_address(ptr+0x100).value
    result['hooked_asi_test']=not args.renderer_only
    result['scene_packets']=args.scene
    result['host_sha256']=hashlib.sha256((cwd/'GroveCraft.asi').read_bytes()).hexdigest()
    assert proc.returncode==0,result
    # A final producer update can race the host's normal exit; all initial
    # packets, including malformed-frame rejection, must still have been read.
    assert len(data)<=result['ring_bytes_consumed']<=published,result
    if not args.scene:assert result['ring_bytes_consumed']==published,result
    assert result['state_seq']>0 and result['state_seq']%2==0,result
    (root/'artifacts').mkdir(exist_ok=True)
    (root/('artifacts/synthetic-scene-render.json' if args.scene else 'artifacts/synthetic-render.json')).write_text(json.dumps(result,indent=2))
    print(json.dumps(result,indent=2))
finally:
    if ptr:k.UnmapViewOfFile(ptr)
    if handle:k.CloseHandle(handle)
    if proc.poll() is None:proc.terminate();proc.wait(timeout=5)
