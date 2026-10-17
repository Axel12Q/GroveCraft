#pragma once
#include <algorithm>
#include <cmath>
namespace grove {
inline float armorDamage(float nativeDamage,float armor,float toughness,float protection){
    if(!std::isfinite(nativeDamage+armor+toughness+protection)||nativeDamage<=0||armor<0||armor>30||toughness<0||toughness>30)return nativeDamage;
    float mc=nativeDamage/5;
    float absorb=std::clamp(armor-mc/(2+toughness/4),armor*.2f,20.f);
    return nativeDamage*(1-absorb/25)*(1-std::clamp(protection,0.f,20.f)/25);
}
inline bool armorWeapon(int weapon){return (weapon>=0&&weapon<=15)||(weapon>=22&&weapon<=34);}
}
