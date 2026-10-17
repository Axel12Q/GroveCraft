#pragma once
#include <cmath>
#include <cstdint>
namespace grove {
// Only new distant sections fade. Never hide a nearby floor, a mined face,
// or a replacement mesh; draw/collision ownership remains unchanged.
inline unsigned streamOpacity(std::uint64_t born,std::uint64_t now,float distance){
    if(!std::isfinite(distance)||distance<48.f||now<born||now-born>=1200)return 255;
    float t=float(now-born)/1200.f;
    return unsigned(255.f*t*t*(3.f-2.f*t));
}
}
