#pragma once
#include <array>
#include <cmath>
#include <map>

namespace grove {
// Used only while looking out of an excavated volume. GTA can cull a terrain
// sheet from below while still drawing the procedural grass above that sheet.
// Bound both work and storage; unresolved underground grass waits for a ray.
class PlantVisibility {
public:
    using Point=std::array<float,3>;
    static constexpr unsigned rayLimit=16,entryLimit=4096;
    unsigned queries{},hits{},pending{},hidden{};
private:
    std::map<std::array<int,3>,bool> cache;
    Point origin{};
    unsigned world{},revision{},remaining{};
    bool active{};
public:
    void begin(Point camera,unsigned area,unsigned cuts,bool underground){
        float distance=0;for(unsigned i=0;i<3;++i)distance+=(camera[i]-origin[i])*(camera[i]-origin[i]);
        if(!underground||!active||area!=world||cuts!=revision||distance>.0625f){cache.clear();origin=camera;}
        active=underground;world=area;revision=cuts;remaining=rayLimit;
    }
    template<class Visible,class Exhausted> bool visible(Point root,Visible ray,Exhausted exhausted){
        if(!active)return true;
        for(float value:root)if(!std::isfinite(value)||std::abs(value)>10000)return true;
        std::array<int,3> key{};for(unsigned i=0;i<3;++i)key[i]=int(std::floor(root[i]*32));
        auto found=cache.find(key);if(found!=cache.end()){++hits;return found->second;}
        if(!remaining||exhausted()){++pending;return false;}
        --remaining;++queries;bool result=ray(origin,root);if(!result)++hidden;
        // Do not discard early entries every frame when a large patch is seen.
        // Extra roots are queried within the same budget without growing RAM.
        if(cache.size()<entryLimit)cache.emplace(key,result);
        return result;
    }
    unsigned size()const{return unsigned(cache.size());}
};
}
