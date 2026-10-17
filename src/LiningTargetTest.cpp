#include <cassert>
#include <cstdio>
#include "LiningTarget.h"
struct Vertex{float x,y,z;unsigned flags;};
int main(){
    grove::LiningTarget target;
    auto always=[](auto){return true;};
    unsigned down=256|(1<<4)|1;
    // GTA coordinates: a ceiling at MC y=20,z=-200..-199, viewed from below.
    std::vector<Vertex> ceiling{{610,199,20,down},{611,199,20,down},{611,200,20,down},
                                {610,199,20,down},{611,200,20,down},{610,200,20,down}};
    assert(target.replace({38,1,-13},ceiling)&&target.size()==2);
    auto h=target.ray({610.5,18,-199.5},{0,1,0},12,always);
    assert(h.valid&&h.cell==grove::LiningTarget::Key({610,20,-200})&&h.face==0&&std::abs(h.distance-2)<.0001);
    assert(!target.ray({610.5,22,-199.5},{0,-1,0},12,always).valid); // Back of lining is not the dug opening.
    assert(!target.ray({610.5,18,-199.5},{0,1,0},12,[](auto){return false;}).valid);
    assert(!target.ray({610.5,18,-199.5},{0,1,0},1,always).valid);
    assert(!target.ray({610.5,18,-199.5},{0,0,0},12,always).valid);
    auto ordinary=ceiling;for(auto& v:ordinary)v.flags&=~256u;
    assert(target.replace({38,1,-13},ordinary)&&target.size()==0); // A stale lining cannot survive a normal block replacement.
    assert(!target.ray({610.5,18,-199.5},{0,1,0},12,always).valid);
    assert(target.replace({38,1,-13},ceiling));target.erase({38,1,-13});assert(!target.size());
    // Negative chunk boundary, including a face registered by the adjacent section.
    for(auto& v:ceiling){v.x-=626;v.z-=20;v.y-=200;}
    assert(target.replace({-2,-1,0},ceiling));
    h=target.ray({-15.5,-2,.5},{0,1,0},12,always);
    assert(h.valid&&h.cell==grove::LiningTarget::Key({-16,0,0}));
    target.clear();assert(!target.size());
    puts("PASS: visible lining is picked from below; normal meshes, stale sections, backfaces and unknown cuts are excluded");
}
