#include "VoxelCollision.h"
#include <cassert>
#include <cstdio>
int main(){
    auto cube=[](int x,int y,int z){return x==0&&y==0&&z==0;};
    auto a=grove::sweep({-1,0,0.5},{2,0,0.5},cube);
    assert((a.blocked&1)&&a.position.x < -0.27); // tunneling across a whole cube prevented
    auto b=grove::sweep({-1,0,0.3},{0.3,0,0.8},cube);
    assert(b.blocked&1);assert(b.position.z>0.79); // slide parallel to the wall
    auto c=grove::sweep({0.5,2,0.5},{0.5,-0.5,0.5},cube);
    assert(c.grounded&&c.position.y>=0.999); // land on top, not inside the cube
    auto d=grove::sweep({-10,0,0},{10,0,0},cube);
    assert(!d.blocked&&d.position.x==10); // a teleport must not sweep the city
    auto negative=[](int x,int y,int z){return x==-1&&y==0&&z==-1;};
    assert(grove::overlaps({-0.5,0,-0.5},negative)); // floor conversion at negative coordinates
    assert(!grove::overlaps({2,0,2},cube));
    std::puts("Swept cube collision: tunneling, sliding, landing, teleports and negative coordinates passed.");
}
