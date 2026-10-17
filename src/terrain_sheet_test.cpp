#include "SATerrain.h"
#include <cassert>
#include <cstdio>
int main(){
    lc::col::Column c;c.yLo=-20;c.yHi=40;
    for(int s=0;s<lc::col::kSamples;++s){
        c.first[s]=static_cast<unsigned>(c.events.size());
        lc::col::Event top{0,{0,1,0},true,37},reverse{.001f,{0,-1,0},false,37};
        c.events.push_back(reverse);c.events.push_back(top);
    }
    c.first[lc::col::kSamples]=static_cast<unsigned>(c.events.size());
    lc::col::WallQuery ghost;ghost.sample=0;ghost.dir=0;ghost.kind=lc::col::kWallEvidence;ghost.y=1;ghost.done=true;
    c.walls.push_back(ghost);auto real=ghost;real.hit=true;c.walls.push_back(real);
    assert(c.SolidAt(1,1));grove::sanitizeTerrainColumn(c);
    assert(!c.SolidAt(1,1));assert(c.SolidAt(1,-1));
    assert(c.walls.size()==1&&c.walls[0].hit);
    assert(c.Count(1)==1);
    // A real elevated slab and its underside must survive.
    lc::col::Column building;building.first.fill(4);building.first[0]=0;
    building.events={{5,{0,1,0},true,141},{4,{0,-1,0},false,141},{.001f,{0,-1,0},false,37},{0,{0,1,0},true,37}};
    grove::sanitizeTerrainColumn(building);assert(building.Count(0)==3);
    assert(building.SolidAt(0,4.5f));assert(!building.SolidAt(0,2));
    std::puts("{\"passed\":true,\"above_ground_air\":true,\"underground_solid\":true,\"real_wall_preserved\":true,\"elevated_slab_preserved\":true}");
}
