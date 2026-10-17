#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <map>
#include <array>
#include <algorithm>
#include <cstdio>
#include <cassert>
#include "libertycraft_protocol.h"
#include "GameBindings.h"
#include "NativeVoxelWorld.h"
namespace p=libertycraft::proto;
#include "NativeExcavation.h"
#include "ExcavationRender.h"
#include "NativeFrameGate.h"
#include "PlayerLifeRules.h"
#include "FellingRules.h"
#include "NativeWeather.h"
#include "NativeGroundProbe.h"
#include "RenderVisibility.h"
using E=grove::NativeExcavation;
static int __cdecl fixture(const E::Matrix&,grove::NativeCol&,const E::Matrix&,grove::NativeCol&,E::Point* points,E::Point*,float*,bool){points[0].pos={.25f,.25f,2};points[1].pos={-.25f,-.25f,2};return 2;}
struct V {float x,y,z;DWORD color;};
int main(){
    {
        grove::GroundProbe ground(-2,3,20);unsigned rays=0,frameRays=0;
        auto pasture=[&](const float* a,const float*,lc::col::Hit& h){++rays;++frameRays;h.pos[0]=a[0];h.pos[1]=18+a[0]*.05f;h.pos[2]=a[2];h.n[1]=1;h.mat=37;return true;};
        while(!ground.run(pasture,[&]{return frameRays>=16;})){assert(frameRays==16);frameRays=0;}
        assert(ground.valid()&&rays==lc::col::kSamples);
        std::vector<lc::col::Tri> surface;lc::col::BuildColumn(*ground.result(),surface);assert(surface.size()>=512);
        for(auto& t:surface)assert(t.kind==lc::col::kFloor&&std::isfinite(t.v[1]));
        grove::GroundProbe roof(0,0,20);assert(roof.run([](const float*,const float*,lc::col::Hit& h){h.mat=128+libertycraft::proto::kDigStone;return true;},[]{return false;}));assert(!roof.valid());
        grove::GroundProbe unknown(0,0,20);assert(unknown.run([](const float*,const float*,lc::col::Hit&){return false;},[]{return false;}));assert(!unknown.valid());
    }
    D3DMATRIX clipIdentity{};clipIdentity._11=clipIdentity._22=clipIdentity._33=clipIdentity._44=1;
    assert(grove::visibleSection(clipIdentity,{-.5f,-.5f,.1f},{.5f,.5f,.9f}));
    assert(!grove::visibleSection(clipIdentity,{2,-.5f,.1f},{3,.5f,.9f}));
    assert(grove::visibleSection(clipIdentity,{-.5f,-.5f,-1},{.5f,.5f,.5f}));
    auto fallen=grove::fallenPoint({10,20,15},{10,20,5},1,0,1.5707963f);
    assert(std::abs(fallen.x-20)<.001&&std::abs(fallen.y-20)<.001&&std::abs(fallen.z-5)<.001);
    auto intact=grove::fallenPoint({11,22,15},{10,20,5},1,0,0);
    assert(std::abs(intact.x-11)<.001&&std::abs(intact.y-22)<.001&&std::abs(intact.z-15)<.001);
    assert(grove::severedBase({.5f,.5f,0},[](gta::Vec p){return p.z<1;}));
    assert(grove::severedBase({.5f,.5f,0},[](gta::Vec p){return p.x<.5;})); // Grid-edge base cut, no unselectable sliver.
    assert(!grove::severedBase({.5f,.5f,0},[](gta::Vec p){return p.x<.2;}));
    assert(!grove::severedBase({.5f,.5f,0},[](gta::Vec p){return p.z>2;}));
    // Real broad trunk705: the front bark is a full cell away from its origin.
    assert(grove::severedBase({728.992f,249.469f,26.219f},[](gta::Vec p){return int(std::floor(p.x))==728&&int(std::floor(-p.y))==-248&&int(std::floor(p.z))==26;},1.6f));
    assert(!grove::severedBase({728.992f,249.469f,25.219f},[](gta::Vec p){return p.z>=27;},1.6f));
    assert(grove::dyingState(54)&&grove::dyingState(55)&&grove::dyingState(56));
    assert(!grove::dyingState(50)&&!grove::dyingState(51)&&!grove::dyingState(53)&&!grove::dyingState(59)&&!grove::dyingState(61)&&!grove::dyingState(63));
    E e;e.enabled=true;e.supported=true;e.world=0;e.rendering=true;
    e.renderMatrix.right.x=e.renderMatrix.forward.y=e.renderMatrix.up.z=1;
    std::array<BYTE,536> packet{};p::RenDug h{0,0,-1,1,0,0};std::memcpy(packet.data(),&h,24);unsigned bit=256*2+16*15;packet[24+bit/8]|=1<<(bit%8);
    assert(e.frame(packet.data(),packet.size()));assert(e.contains({.25f,.25f,2}));assert(!e.contains({-.25f,.25f,2}));assert(!e.contains({.25f,1.25f,2}));
    assert(e.terrainBoundary(0,3,-1)); // Ceiling directly above a real removed terrain cell.
    assert(e.terrainBoundary(1,2,-1)); // Side wall of that same excavation.
    assert(!e.terrainBoundary(0,2,-1)&&!e.terrainBoundary(1,3,-1));
    E woodOnly;woodOnly.enabled=true;auto woodPacket=packet;reinterpret_cast<p::RenDug*>(woodPacket.data())->worldId=256;
    assert(woodOnly.frame(woodPacket.data(),woodPacket.size()));
    assert(!woodOnly.contains({.25f,.25f,2})&&woodOnly.contains({.25f,.25f,2},true));
    assert(!woodOnly.terrainBoundary(0,3,-1)); // Bark masks cannot create a terrain mining target.
    E architecture;architecture.enabled=true;auto architecturalPacket=packet;
    reinterpret_cast<p::RenDug*>(architecturalPacket.data())->worldId=512;
    assert(architecture.frame(architecturalPacket.data(),architecturalPacket.size()));
    assert(!architecture.contains({.25f,.25f,2})); // Original surface remains until both mesh and collision exist.
    auto readyPacket=architecturalPacket;reinterpret_cast<p::RenDug*>(readyPacket.data())->worldId=2304;
    assert(architecture.frame(readyPacket.data(),readyPacket.size()));
    assert(!architecture.contains({.25f,.25f,2}));
    auto entryPacket=architecturalPacket;reinterpret_cast<p::RenDug*>(entryPacket.data())->worldId=1280;
    assert(architecture.frame(entryPacket.data(),entryPacket.size()));
    auto chambers=architecture.chambers();assert(chambers.size()==1&&chambers.begin()->second.size()==5);
    const auto& floor=chambers.begin()->second[0];assert(std::abs(floor.max.z-floor.min.z-.04f)<.001f);
    std::vector<BYTE> boundedReady(readyPacket.begin(),readyPacket.end());
    reinterpret_cast<p::RenDug*>(boundedReady.data())->pad=0x47434331;
    float cutBounds[]={0,0,.5f,1,1,1};auto rawBounds=reinterpret_cast<const BYTE*>(cutBounds);
    boundedReady.insert(boundedReady.end(),rawBounds,rawBounds+sizeof cutBounds);
    assert(architecture.frame(boundedReady.data(),boundedReady.size()));
    chambers=architecture.chambers();assert(chambers.size()==1&&chambers.begin()->second.size()==5);
    assert(std::abs(chambers.begin()->second[0].max.y-(-7.5f))<.001f); // No plates protrude in front of the native facade.
    auto malformedReady=boundedReady;float invalid=2;std::memcpy(malformedReady.data()+536,&invalid,4);
    assert(!architecture.frame(malformedReady.data(),malformedReady.size()));
    grove::NativeBox left,right;left.min={0,0,.5f};left.max={1,1,1};right=left;
    assert(E::connectedBounds(left,right,5));
    right.min.x=.2f;assert(!E::connectedBounds(left,right,5));
    right=left;right.min.z=.7f;assert(!E::connectedBounds(left,right,5));
    right=left;assert(!E::connectedBounds(left,right,2)); // Shallow facade cavities do not meet in depth.
    architecture.chamberReady=[](const grove::SectionKey&){return true;};
    assert(architecture.contains({.25f,.25f,2})&&!architecture.contains({.25f,.25f,2},true));
    architecture.sampling=true;
    assert(!architecture.contains({.25f,.25f,2})&&!architecture.contains({.25f,.25f,2},true));
    architecture.sampling=false;
    assert(architecture.boxes({-1,-1,0},{2,2,4}).size()==1);
    architecture.woodContext=true;assert(architecture.boxes({-1,-1,0},{2,2,4}).empty());
    architecture.world=1;assert(!architecture.contains({.25f,.25f,2}));
    assert(woodOnly.boxes({-1,-1,0},{2,2,4}).empty());
    woodOnly.woodContext=true;assert(!woodOnly.boxes({-1,-1,0},{2,2,4}).empty());
    woodOnly.world=1;assert(!woodOnly.contains({.25f,.25f,2},true));
    e.world=1;assert(!e.contains({.25f,.25f,2}));e.world=0;
    auto malformed=packet;reinterpret_cast<p::RenDug*>(malformed.data())->count=2;assert(!e.frame(malformed.data(),malformed.size()));
    E::Point points[32]{};grove::NativeCol a,b;grove::NativeData data;b.data=&data;data.triangles=1;e.physics=fixture;
    e.registerSurfaceForTest(b);
    assert(e.contacts(e.renderMatrix,a,e.renderMatrix,b,points,nullptr,nullptr,false)==1);assert(points[0].pos.x<0);
    grove::NativeCol privateBlock;privateBlock.data=&data;assert(e.contacts(e.renderMatrix,a,e.renderMatrix,privateBlock,points,nullptr,nullptr,false)==2);
    E::Line line{{.25f,.25f,4},0,{.25f,.25f,0},0};E::Point hit{};float distance=1;unsigned casts=0;
    bool found=e.sight(line,hit,distance,[&](const E::Line& l,E::Point& cp,float& d){++casts;float z=l.from.z>2?2:1;d=(l.from.z-z)/(l.from.z-l.to.z);cp.pos={.25f,.25f,z};return true;});
    assert(found&&casts==2&&hit.pos.z==1&&std::abs(distance-.75f)<.001);
    WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"GroveExcavationTest";RegisterClassW(&wc);
    HWND window=CreateWindowW(wc.lpszClassName,L"",WS_OVERLAPPED,0,0,128,128,nullptr,nullptr,wc.hInstance,nullptr);
    auto* d3d=Direct3DCreate9(D3D_SDK_VERSION);IDirect3DDevice9* dev{};D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.BackBufferWidth=pp.BackBufferHeight=128;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window;
    assert(SUCCEEDED(d3d->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&dev)));
    {
        grove::NativeWeather weather;
        std::array<D3DRENDERSTATETYPE,9> names{D3DRS_FOGENABLE,D3DRS_FOGCOLOR,D3DRS_FOGSTART,D3DRS_FOGEND,
            D3DRS_FOGDENSITY,D3DRS_FOGTABLEMODE,D3DRS_FOGVERTEXMODE,D3DRS_RANGEFOGENABLE,D3DRS_LIGHTING};
        auto bits=[](float f){DWORD value{};std::memcpy(&value,&f,4);return value;};
        std::array<DWORD,9> values{TRUE,0xff8c9eaa,bits(12),bits(90),bits(.02f),D3DFOG_LINEAR,D3DFOG_NONE,FALSE,FALSE};
        for(unsigned i=0;i<names.size();++i)assert(SUCCEEDED(dev->SetRenderState(names[i],values[i])));
        weather.capture(dev);
        // A native HUD/alpha pass disables fog; it must not erase the world pass.
        dev->SetRenderState(D3DRS_FOGENABLE,FALSE);dev->SetRenderState(D3DRS_FOGCOLOR,0xff000000);weather.capture(dev);
        IDirect3DStateBlock9* hud{};assert(SUCCEEDED(dev->CreateStateBlock(D3DSBT_ALL,&hud)));
        weather.apply(dev);
        for(unsigned i=0;i<names.size()-1;++i){DWORD actual{};assert(SUCCEEDED(dev->GetRenderState(names[i],&actual))&&actual==values[i]);}
        assert(SUCCEEDED(hud->Apply()));hud->Release();DWORD actual{};dev->GetRenderState(D3DRS_FOGENABLE,&actual);assert(!actual);
    }
    assert(grove::FrameGate::mainTarget(dev));
    IDirect3DSurface9 *backbuffer{},*radar{};dev->GetRenderTarget(0,&backbuffer);
    assert(SUCCEEDED(dev->CreateRenderTarget(128,128,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,FALSE,&radar,nullptr)));
    assert(SUCCEEDED(dev->SetRenderTarget(0,radar)));assert(!grove::FrameGate::mainTarget(dev));
    dev->SetRenderTarget(0,backbuffer);assert(grove::FrameGate::mainTarget(dev));radar->Release();backbuffer->Release();
    V vertices[]={{-2,-2,2,0xff00ff00},{2,-2,2,0xff00ff00},{-2,2,2,0xff00ff00},{2,2,2,0xff00ff00}};WORD indices[]={0,1,2,2,1,3};
    IDirect3DVertexBuffer9* vb{};IDirect3DIndexBuffer9* ib{};assert(SUCCEEDED(dev->CreateVertexBuffer(sizeof vertices,0,D3DFVF_XYZ|D3DFVF_DIFFUSE,D3DPOOL_MANAGED,&vb,nullptr)));assert(SUCCEEDED(dev->CreateIndexBuffer(sizeof indices,0,D3DFMT_INDEX16,D3DPOOL_MANAGED,&ib,nullptr)));
    void* ptr{};vb->Lock(0,0,&ptr,0);std::memcpy(ptr,vertices,sizeof vertices);vb->Unlock();ib->Lock(0,0,&ptr,0);std::memcpy(ptr,indices,sizeof indices);ib->Unlock();
    D3DMATRIX identity{};identity._11=identity._22=identity._33=identity._44=1;D3DMATRIX projection{};projection._11=projection._22=1;projection._33=20.f/19;projection._34=1;projection._43=-20.f/19;
    dev->SetTransform(D3DTS_WORLD,&identity);dev->SetTransform(D3DTS_VIEW,&identity);dev->SetTransform(D3DTS_PROJECTION,&projection);dev->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE);dev->SetStreamSource(0,vb,0,sizeof(V));dev->SetIndices(ib);dev->SetRenderState(D3DRS_LIGHTING,FALSE);dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    dev->Clear(0,nullptr,D3DCLEAR_TARGET,0xff0000ff,1,0);dev->BeginScene();HRESULT result{};assert(grove::cutNativeDraw(e,dev,D3DPT_TRIANGLELIST,0,0,4,0,2,result));assert(SUCCEEDED(result));dev->EndScene();
    IDirect3DVertexBuffer9* restored{};UINT offset{},stride{};dev->GetStreamSource(0,&restored,&offset,&stride);assert(restored==vb&&stride==sizeof(V));restored->Release();
    IDirect3DSurface9 *rt{},*copy{};dev->GetRenderTarget(0,&rt);dev->CreateOffscreenPlainSurface(128,128,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&copy,nullptr);assert(SUCCEEDED(dev->GetRenderTargetData(rt,copy)));D3DLOCKED_RECT lr{};copy->LockRect(&lr,nullptr,D3DLOCK_READONLY);
    auto pixel=[&](int x,int y){return *reinterpret_cast<DWORD*>(static_cast<BYTE*>(lr.pBits)+y*lr.Pitch+x*4);};DWORD hole=pixel(72,56),ground=pixel(56,72);assert((hole&0xffffff)==0xff);assert((ground&0xffffff)==0xff00);
    copy->UnlockRect();
    // Same input really reuses the cut and preserves the GPU image/state.
    auto misses=e.cutCacheMisses,hits=e.cutCacheHits;
    dev->Clear(0,nullptr,D3DCLEAR_TARGET,0xff0000ff,1,0);dev->BeginScene();
    assert(grove::cutNativeDraw(e,dev,D3DPT_TRIANGLELIST,0,0,4,0,2,result));assert(SUCCEEDED(result));dev->EndScene();
    assert(e.cutCacheHits==hits+1&&e.cutCacheMisses==misses);
    assert(SUCCEEDED(dev->GetRenderTargetData(rt,copy)));copy->LockRect(&lr,nullptr,D3DLOCK_READONLY);
    assert(pixel(72,56)==hole&&pixel(56,72)==ground);copy->UnlockRect();
    // The native grass plane can be a millimetre above the dug cell's top.
    for(auto& v:vertices)v.z=3.001f;
    vb->Lock(0,0,&ptr,0);std::memcpy(ptr,vertices,sizeof vertices);vb->Unlock();
    dev->Clear(0,nullptr,D3DCLEAR_TARGET,0xff0000ff,1,0);dev->BeginScene();
    assert(grove::cutNativeDraw(e,dev,D3DPT_TRIANGLELIST,0,0,4,0,2,result));assert(SUCCEEDED(result));dev->EndScene();
    assert(e.cutCacheMisses==misses+1); // A rewritten native VB is not reused.
    assert(SUCCEEDED(dev->GetRenderTargetData(rt,copy)));copy->LockRect(&lr,nullptr,D3DLOCK_READONLY);
    assert((pixel(69,59)&0xffffff)==0xff);assert((pixel(59,69)&0xffffff)==0xff00);copy->UnlockRect();
    for(auto& v:vertices)v.z=2;
    IDirect3DVertexBuffer9 *positions{},*colours{};dev->CreateVertexBuffer(48,0,0,D3DPOOL_MANAGED,&positions,nullptr);dev->CreateVertexBuffer(16,0,0,D3DPOOL_MANAGED,&colours,nullptr);
    positions->Lock(0,0,&ptr,0);for(int i=0;i<4;++i)std::memcpy(static_cast<BYTE*>(ptr)+i*12,&vertices[i],12);positions->Unlock();colours->Lock(0,0,&ptr,0);for(int i=0;i<4;++i)static_cast<DWORD*>(ptr)[i]=0xff00ff00;colours->Unlock();
    WORD strip[]={0,1,2,3};ib->Lock(0,0,&ptr,0);std::memcpy(ptr,strip,sizeof strip);ib->Unlock();
    D3DVERTEXELEMENT9 elements[]={{0,0,D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_COLOR,0},{1,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},D3DDECL_END()};IDirect3DVertexDeclaration9* vd{};dev->CreateVertexDeclaration(elements,&vd);
    dev->SetVertexDeclaration(vd);dev->SetStreamSource(0,colours,0,4);dev->SetStreamSource(1,positions,0,12);
    dev->Clear(0,nullptr,D3DCLEAR_TARGET,0xff0000ff,1,0);dev->BeginScene();assert(grove::cutNativeDraw(e,dev,D3DPT_TRIANGLESTRIP,0,0,4,0,2,result));assert(SUCCEEDED(result));dev->EndScene();
    dev->GetStreamSource(0,&restored,&offset,&stride);assert(restored==colours&&stride==4);restored->Release();dev->GetStreamSource(1,&restored,&offset,&stride);assert(restored==positions&&stride==12);restored->Release();
    IDirect3DVertexDeclaration9* restoredDecl{};dev->GetVertexDeclaration(&restoredDecl);assert(restoredDecl==vd);restoredDecl->Release();
    assert(SUCCEEDED(dev->GetRenderTargetData(rt,copy)));copy->LockRect(&lr,nullptr,D3DLOCK_READONLY);assert((pixel(72,56)&0xffffff)==0xff);assert((pixel(56,72)&0xffffff)==0xff00);copy->UnlockRect();
    vd->Release();positions->Release();colours->Release();copy->Release();rt->Release();vb->Release();ib->Release();dev->Release();d3d->Release();DestroyWindow(window);
    std::printf("{\"passed\":true,\"gpu_hole\":%lu,\"gpu_ground\":%lu,\"contacts\":true,\"private_blocks_preserved\":true,\"world_isolation\":true,\"negative_coordinates\":true,\"ray_continuation\":true,\"streams_restored\":true,\"split_colour_position_streams\":true,\"triangle_strips\":true,\"terrain_boundary_targets\":true,\"native_fog_state_and_hud_restore\":true}\n",hole,ground);return 0;
}
