#include <cassert>
#include <cstdio>
#include <limits>
#include "VoxelCollision.h"
#include "BuildTarget.h"
int main(){
    auto block=[](int x,int y,int z){return x==5&&y==1&&z==0;};
    auto hit=grove::aim({.5,1.5,.5},{1,0,0},{2,0,.5},block);
    assert(hit.hit&&hit.x==5&&std::abs(hit.distance-4.5)<1e-8);
    assert(!grove::nativePrecedes(4.8,hit)); // Behind the front face, before the centre.
    assert(grove::nativePrecedes(4.4,hit));
    assert(!grove::nativePrecedes(4.495,hit)); // Shared boundary favours the real block.
    assert(!grove::nativePrecedes(std::numeric_limits<double>::quiet_NaN(),hit));
    auto down=grove::aim({-1.5,1,-3.5},{0,-1,0},{-1.5,-1,-3.5},
        [](int x,int y,int z){return x==-2&&y==-3&&z==-4;});
    assert(down.hit&&std::abs(down.distance-3)<1e-8);
    assert(!grove::nativePrecedes(3.25,down)&&grove::nativePrecedes(2.9,down));
    auto inside=grove::aim({.3,.4,.5},{1,0,0},{0,0,0},[](int x,int y,int z){return !x&&!y&&!z;});
    assert(inside.hit&&inside.distance==0&&!grove::nativePrecedes(0,inside));
    std::puts("{\"passed\":true,\"front_surface_priority\":true,\"negative_coordinates\":true,\"inside_voxel\":true}");
}
