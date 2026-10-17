#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define GROVE_CRIME_TEST
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>
#include <array>
#include <cassert>
#include <cstdio>
#include "GameBindings.h"
#include "NativeCrime.h"
static unsigned calls;
static void __thiscall raiseWanted(void* w,int level){assert(level==1);++calls;auto* value=reinterpret_cast<unsigned*>(static_cast<BYTE*>(w)+0x2c);*value=std::max(*value,unsigned(level));}
struct Actors{std::uintptr_t cop;template<class F> void eachPed(F f)const{f(1,cop);}};
template<class T> void put(BYTE* p,unsigned off,T value){std::memcpy(p+off,&value,sizeof value);}
int main(){
    BYTE cj[0x7c4]{},data[4]{},wanted[0x34]{},cop[0x7c4]{},matrix[64]{};
    put(cj,0x480,DWORD(std::uintptr_t(data)));put(data,0,DWORD(std::uintptr_t(wanted)));
    put(cop,0x14,DWORD(std::uintptr_t(matrix)));put(cop,0x598,DWORD(6));put(cop,0x530,DWORD(1));put(cop,0x540,100.f);cop[0x484]=1;
    put(matrix,0x30,gta::Vec{0,0,0});put(matrix,0x10,gta::Vec{1,0,0});
    gta::Snapshot s{};s.valid=true;s.menu=s.mission=false;s.health=100;s.ped=std::uintptr_t(cj);s.position={10,0,0};
    Actors actors{std::uintptr_t(cop)};grove::NativeCrime crime;crime.fixture(raiseWanted);
    auto visible=[](gta::Vec,gta::Vec){return true;};auto blocked=[](gta::Vec,gta::Vec){return false;};
    assert(!crime.vandalism(s,actors,s.position,blocked)&&calls==0);
    crime.fixture(raiseWanted);put(matrix,0x10,gta::Vec{-1,0,0});assert(!crime.vandalism(s,actors,s.position,visible)&&calls==0);
    crime.fixture(raiseWanted);put(matrix,0x10,gta::Vec{1,0,0});assert(crime.vandalism(s,actors,s.position,visible)&&calls==1);
    assert(*reinterpret_cast<unsigned*>(wanted+0x2c)==1);
    crime.fixture(raiseWanted);put(wanted,0x2c,unsigned(3));assert(crime.vandalism(s,actors,s.position,visible)&&calls==2);assert(*reinterpret_cast<unsigned*>(wanted+0x2c)==3);
    crime.fixture(raiseWanted);s.mission=true;assert(!crime.vandalism(s,actors,s.position,visible)&&calls==2);s.mission=false;
    crime.fixture(raiseWanted);s.position.x=41;assert(!crime.vandalism(s,actors,s.position,visible)&&calls==2);
    std::puts("{\"passed\":true,\"mock_wanted_call\":true,\"occlusion\":true,\"facing\":true,\"range\":true,\"mission_guard\":true,\"real_police_test\":false}");
}
