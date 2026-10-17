#pragma once
#include <algorithm>
#include <cmath>
namespace grove {
struct Point { double x{}, y{}, z{}; };
struct Motion { Point position; unsigned blocked{}; bool grounded{}; };
template<class Solid> bool overlaps(Point p, Solid solid) {
    constexpr double radius=0.28,height=1.65,epsilon=0.0001;
    for(int x=static_cast<int>(std::floor(p.x-radius+epsilon)); x<=std::floor(p.x+radius-epsilon); ++x)
    for(int y=static_cast<int>(std::floor(p.y+epsilon)); y<=std::floor(p.y+height-epsilon); ++y)
    for(int z=static_cast<int>(std::floor(p.z-radius+epsilon)); z<=std::floor(p.z+radius-epsilon); ++z)
        if(solid(x,y,z))return true;
    return false;
}
/** Short swept steps with axis sliding. Full unit cubes only; not a GTA collision-world replacement. */
template<class Solid> Motion sweep(Point from,Point to,Solid solid) {
    Motion out{to};
    Point delta{to.x-from.x,to.y-from.y,to.z-from.z};
    if(!std::isfinite(delta.x)||!std::isfinite(delta.y)||!std::isfinite(delta.z)
       || std::max({std::abs(delta.x),std::abs(delta.y),std::abs(delta.z)})>8 || overlaps(from,solid))return out;
    int steps=std::max(1,static_cast<int>(std::ceil(std::max({std::abs(delta.x),std::abs(delta.y),std::abs(delta.z)})/0.1)));
    out.position=from;
    for(int n=0;n<steps;++n){
        Point next=out.position;next.x+=delta.x/steps;
        if(overlaps(next,solid))out.blocked|=1;else out.position=next;
        next=out.position;next.z+=delta.z/steps;
        if(overlaps(next,solid))out.blocked|=4;else out.position=next;
        next=out.position;next.y+=delta.y/steps;
        if(overlaps(next,solid)){out.blocked|=2;if(delta.y<0)out.grounded=true;}else out.position=next;
    }
    return out;
}
}
