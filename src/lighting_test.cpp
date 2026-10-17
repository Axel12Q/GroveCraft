#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cassert>
#include <cstdio>
#include "GameBindings.h"
#include "NativeLighting.h"
int main(){
    grove::NativeLighting l;assert(l.factors({},0,0)[0]==1); // Safe synthetic/unknown host fallback.
    l.supported=true;l.ambient={.2f,.25f,.3f};l.daylight=grove::NativeLighting::daylightAt(20);
    assert(l.daylight>.49f&&l.daylight<.51f);
    auto dusk=l.factors({},0xf00,0),cave=l.factors({},0,0),torch=l.factors({},15,0);
    assert(dusk[0]>.5f&&dusk[2]>dusk[0]);assert(cave[0]<dusk[0]);assert(torch[0]>.9f);
    l.daylight=0;auto night=l.factors({},0xf00,0);assert(night[0]<dusk[0]&&night[0]>.25f);
    assert((l.shade(0x80104080,{})&0xff000000)==0x80000000);
    assert(grove::NativeLighting::lampFactor({0,0,0},{},8,0)==1);
    assert(grove::NativeLighting::lampFactor({8,0,0},{},8,0)==0);
    assert(grove::NativeLighting::lampFactor({2,0,0},{1,0,0},8,1)>.5f);
    assert(grove::NativeLighting::lampFactor({-2,0,0},{1,0,0},8,1)==0);
    assert(grove::NativeLighting::lampFactor({0,0,0},{},8,3)==0); // Fog is not a light.
    for(int i=0;i<2400;++i){float h=i*.01f,d=grove::NativeLighting::daylightAt(h);assert(std::isfinite(d)&&d>=0&&d<=1);
        assert(std::abs(d-grove::NativeLighting::daylightAt(h+.01f))<.006f);}
    std::puts("PASS twilight continuity, night readability, host RGB tint, cave darkness, Minecraft emission and alpha preservation");
}
