#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cmath>
#include <map>
#include <vector>
#include <array>
#include <string>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <cassert>
#include <cstdio>
#include "libertycraft_protocol.h"
#include "GameBindings.h"
#include "NativeVoxelWorld.h"
namespace p=libertycraft::proto;
#include "NativeWater.h"
int main(){
    grove::NativeWater water;water.enabled=true;
    struct Packet{p::RenLiquids h;p::RenLiquid cells[3];} packet{{-1,0,-1,3},{{15,4,15,31},{15,5,15,93},{14,5,15,45}}};
    assert(water.packet(&packet,sizeof packet));float height{};
    assert(!water.source(-1,4,-1)&&water.source(-1,5,-1));
    // Negative coordinates, connected depth, source fraction, lava exclusion.
    assert(water.surface(-.5f,.5f,4.2f,height)&&std::abs(height-(5+13/15.f))<.0001f);
    assert(!water.surface(-1.5f,.5f,5,height));
    assert(!water.surface(-.5f,.5f,1,height));
    auto original=water.cells;packet.cells[2]=packet.cells[0];assert(!water.packet(&packet,sizeof packet)&&water.cells==original);
    packet.h.sx=INT_MIN;assert(!water.packet(&packet,sizeof packet));
    packet.h={-1,0,-1,0};assert(water.packet(&packet.h,sizeof packet.h)&&water.cells==0);
    assert(!water.surface(-.5f,.5f,4.2f,height));
    puts("{\"passed\":true,\"negative_coords\":true,\"depth_and_height\":true,\"lava_excluded\":true,\"invalid_packet_atomic\":true,\"removal\":true}");
}
