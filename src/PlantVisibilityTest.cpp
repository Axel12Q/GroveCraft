#include <cassert>
#include <cstdio>
#include "PlantVisibility.h"
int main(){
    grove::PlantVisibility sight;unsigned calls=0;
    auto ray=[&](auto,auto root){++calls;return root[0]<1;};
    auto timeLeft=[](){return false;};
    sight.begin({0,0,-2},0,1,false);
    assert(sight.visible({8,0,0},ray,timeLeft)&&calls==0); // Normal surface grass is untouched.
    sight.begin({0,0,-2},0,1,true);
    assert(sight.visible({0,0,0},ray,timeLeft));
    assert(!sight.visible({8,0,0},ray,timeLeft)); // The solid hillside blocks a distant root.
    assert(!sight.visible({8,0,0},ray,timeLeft)&&calls==2);
    for(int i=1;i<100;++i)sight.visible({float(i),0,0},ray,timeLeft);
    assert(calls==16&&sight.pending>0); // Never run one CWorld ray per grass draw.
    sight.begin({0,0,-2},0,1,true);
    assert(!sight.visible({99,0,0},ray,[](){return true;})&&calls==16);
    assert(sight.visible({0,0,0},ray,timeLeft)&&calls==16); // Cached after budget/time exhaustion.
    sight.begin({1,0,-2},0,1,true);sight.visible({0,0,0},ray,timeLeft);assert(calls==17);
    sight.begin({1,0,-2},0,2,true);sight.visible({0,0,0},ray,timeLeft);assert(calls==18);
    sight.begin({1,0,-2},1,2,true);sight.visible({0,0,0},ray,timeLeft);assert(calls==19);
    for(int frame=0;frame<400;++frame){
        sight.begin({1,0,-2},1,2,true);
        for(int i=0;i<16;++i)sight.visible({float(frame*16+i),0,0},ray,timeLeft);
    }
    assert(sight.size()<=sight.entryLimit);
    sight.begin({1,0,-2},1,2,false);
    assert(sight.size()==0&&sight.visible({99,0,0},ray,timeLeft));
    puts("PASS: underground grass occlusion respects ray/time/memory bounds and camera/world/cut invalidation");
}
