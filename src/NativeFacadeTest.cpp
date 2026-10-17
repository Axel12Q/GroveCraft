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
int main(){
    grove::NativeExcavation excavation;excavation.enabled=true;
    excavation.chamberReady=[](const grove::SectionKey&){return true;};
    auto send=[&](unsigned world,bool bounded){
        p::RenDug h{};h.count=1;h.worldId=world;h.pad=bounded?0x47434331:0;
        std::vector<BYTE> bytes(sizeof h+512+(bounded?24:0));std::memcpy(bytes.data(),&h,sizeof h);bytes[sizeof h]=1;
        float bounds[]={.2f,0,.7f,.6f,1,1};if(bounded)std::memcpy(bytes.data()+sizeof h+512,bounds,24);
        assert(excavation.frame(bytes.data(),unsigned(bytes.size())));
    };
    send(512,false);send(2304,true);
    auto boxes=excavation.boxes({-2,-2,-2},{2,2,2});assert(boxes.size()==1);
    assert(std::abs(boxes[0].min.x-.2f)<.0001f&&std::abs(boxes[0].max.x-.6f)<.0001f);
    assert(std::abs(boxes[0].min.y+1)<.0001f&&std::abs(boxes[0].max.y+.7f)<.0001f);
    assert(excavation.contains({.4f,-.8f,.5f}));
    assert(!excavation.contains({.9f,-.8f,.5f})); // Render facade outside lining remains solid.
    assert(!excavation.contains({.4f,-.2f,.5f}));
    send(0,false);assert(excavation.contains({.9f,-.2f,.5f})); // Terrain still uses full cells.
    puts("PASS: facade clipping and ray/contact removal use exact lined bounds; terrain unchanged");
}
