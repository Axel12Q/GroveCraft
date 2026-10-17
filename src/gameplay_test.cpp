#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cassert>
#include <cstdio>
#include "GameBindings.h"
#include "NativeVoxelWorld.h"
#include "VoxelCollision.h"
#include "BuildTarget.h"
int main(){
    grove::SectionBits bits{};bits[0]=3;auto boxes=grove::collisionBoxes(bits);assert(boxes.size()==1);assert(boxes[0].min.x==-8&&boxes[0].max.x==-6&&boxes[0].min.y==7&&boxes[0].max.y==8);
    bits.fill(255);boxes=grove::collisionBoxes(bits);assert(boxes.size()==256);
    auto cube=[](int x,int y,int z){return x==3&&y==1&&z==0;};
    auto a=grove::aim({-3,1.5,.5},{1,0,0},{0,0,.5},cube);
    assert(a.valid&&a.hit&&a.face==4&&a.x==3&&a.px==2);
    a=grove::aim({3.5,5,.5},{0,-1,0},{3.5,0,.5},cube);
    assert(a.valid&&a.hit&&a.face==1&&a.py==2);
    a=grove::aim({-3,3,.5},{1,-.5,0},{0,0,.5},[](int,int,int){return false;});
    assert(a.valid&&!a.hit&&a.face==1);
    a=grove::aim({-6,1.5,.5},{1,0,0},{-8,0,.5},cube);assert(!a.valid);
    // Own allocated fake ped: verify health caps, ownership, dead/menu/vehicle rejection.
    auto* ped=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x1000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(ped);
    DWORD playerInfo[100]{};BYTE focus=0;playerInfo[0]=reinterpret_cast<std::uintptr_t>(ped);
    float initial=99.5f,maximum=100;std::memcpy(ped+0x540,&initial,4);std::memcpy(ped+0x544,&maximum,4);
    gta::Bindings b;b.players=reinterpret_cast<std::uintptr_t>(playerInfo);b.focus=reinterpret_cast<std::uintptr_t>(&focus);
    b.foodSupported=true;
    gta::Snapshot s;s.valid=true;s.menu=false;s.mission=false;s.state=1;s.ped=reinterpret_cast<std::uintptr_t>(ped);s.health=initial;s.maxHealth=maximum;
    WORD hours=50;std::memcpy(reinterpret_cast<char*>(playerInfo)+0x144,&hours,2);
    assert(b.feed(s));std::memcpy(&hours,reinterpret_cast<char*>(playerInfo)+0x144,2);assert(hours==0);
    b.foodSupported=false;assert(!b.feed(s));b.foodSupported=true;
    assert(b.heal(s,1)==.5f);assert(b.heal(s,1)==0);
    std::memcpy(ped+0x540,&initial,4);s.menu=true;assert(b.heal(s,1)==0);s.menu=false;s.vehicle=true;assert(b.heal(s,1)==0);s.vehicle=false;
    playerInfo[0]=0;assert(b.heal(s,1)==0);playerInfo[0]=s.ped;
    float dead=0;std::memcpy(ped+0x540,&dead,4);assert(b.heal(s,1)==0);
    VirtualFree(ped,0,MEM_RELEASE);std::puts("Camera ray faces/reach and guarded CJ regeneration passed (synthetic).");
}
