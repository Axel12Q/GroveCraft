#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
#include <map>
#include <array>
#include <set>
#include <algorithm>
#include <cassert>
#include "libertycraft_protocol.h"
namespace p=libertycraft::proto;
#include "GameBindings.h"
#include "NativeVoxelWorld.h"
#include "NativeExcavation.h"
#include "ExcavationRender.h"
using namespace grove;
int main(){
    std::vector<D3DVERTEXELEMENT9> decl{
        {0,0,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_POSITION,0},
        {0,12,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_NORMAL,0},
        {0,24,D3DDECLTYPE_FLOAT2,0,D3DDECLUSAGE_TEXCOORD,0}};
    std::vector<CutVertex> bark;
    gta::Vec corners[]={{-.3f,-.3f,0},{.3f,-.3f,0},{.3f,.3f,0},{-.3f,.3f,0}};
    auto vertex=[](gta::Vec point,float u,float v){CutVertex c;c.world=point;
        float data[]={point.x,point.y,point.z,1,0,0,u,v};std::memcpy(c.data.data(),data,sizeof data);return c;};
    for(int i=0;i<4;i++){auto a=corners[i],b=corners[(i+1)%4];
        auto c=b,d=a;c.z=d.z=6;
        for(auto v:{vertex(a,0,0),vertex(b,1,0),vertex(c,1,1),vertex(a,0,0),vertex(c,1,1),vertex(d,0,1)})bark.push_back(v);
    }
    NativeExcavation excavation;excavation.woodContext=true;excavation.woodRadius=.65f;
    excavation.renderMatrix.right.x=excavation.renderMatrix.forward.y=excavation.renderMatrix.up.z=1;
    auto ring=woodCap(bark,2,1,excavation,decl);assert(ring.size()==4);
    float area=0;for(unsigned i=0;i<ring.size();++i){auto a=ring[i].world,b=ring[(i+1)%ring.size()].world;area+=a.x*b.y-a.y*b.x;}
    assert(std::abs(area*.5f-.36f)<.00001f);
    assert(woodCap(bark,0,2,excavation,decl).empty());
    NativeBox left;left.min={-1,-1,0};left.max={0,1,1};
    NativeBox right=left;right.min.x=0;right.max.x=1;
    std::vector<BYTE> caps;appendWoodCaps(caps,bark,{left,right},excavation,decl,32);
    assert(!caps.empty()&&caps.size()%96==0);
    float topArea=0;
    for(unsigned i=0;i<caps.size();i+=96){float a[8],b[8],c[8];
        std::memcpy(a,caps.data()+i,32);std::memcpy(b,caps.data()+i+32,32);std::memcpy(c,caps.data()+i+64,32);
        for(auto v:{a,b,c}){assert(std::isfinite(v[0]+v[1]+v[2]));assert(v[6]>=-.001f&&v[6]<=1.001f&&v[7]>=-.001f&&v[7]<=1.001f);}
        if(std::abs(a[2]-1.005f)<.0001f&&std::abs(b[2]-1.005f)<.0001f&&std::abs(c[2]-1.005f)<.0001f)
            topArea+=std::abs((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]))*.5f;
    }
    assert(topArea>=.3599f&&topArea<.38f); // shared cut must leave no transparent strip
    excavation.woodContext=false;caps.clear();appendWoodCaps(caps,bark,{left},excavation,decl,32);assert(caps.empty());
    std::puts("Native wood caps: original UVs, convex section, adjacent-cut closure PASS");
}
