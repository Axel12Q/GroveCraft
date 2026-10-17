#pragma once
#include <array>
#include <cmath>
#include <cstdint>
namespace grove {
// A dropped/reordered/partial ring snapshot must never erase the previous map.
struct StructureMapSnapshot {
    static constexpr unsigned limit=64;
    struct Point {float x{},y{},z{};unsigned kind{};};
    std::array<Point,limit> points{},pending{};
    std::array<bool,limit> seen{};
    unsigned count{},expected{},token{},received{},commits{};
    bool staging{};
    bool accept(unsigned slot,float x,float y,float z,float total,unsigned sequence,unsigned kind){
        if(slot==0){staging=sequence&&std::isfinite(total)&&total>=0&&total<=limit&&total==std::floor(total);
            token=sequence;expected=staging?unsigned(total):0;received=0;seen.fill(false);return false;}
        if(!staging||sequence!=token)return false;
        if(slot==limit+1){
            bool complete=std::isfinite(total)&&total==expected&&received==expected;
            staging=false;if(!complete)return false;
            count=expected;points=pending;++commits;return true;
        }
        if(slot>expected||kind<1||kind>6||!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)
            ||std::abs(x)>4000||std::abs(z)>4000||y< -60||y>1000){staging=false;return false;}
        auto i=slot-1;if(seen[i]){staging=false;return false;}
        seen[i]=true;++received;pending[i]={x,-z,y,kind};return false;
    }
};
}
