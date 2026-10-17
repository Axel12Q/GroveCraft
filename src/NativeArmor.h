#pragma once
#include "ArmorRules.h"
namespace grove {
class NativeArmor {
public:
    using Calculate=void (__thiscall*)(void*,void*,void*,bool);
    Calculate original{};
    std::uintptr_t code{};bool supported{};
    float value{},toughness{},meleeProtection{},bulletProtection{};ULONGLONG stamp{};
    unsigned protectedHits{},wearReports{};float lastRaw{},lastReduced{};
    unsigned calls{},fixtureCalls{};std::uintptr_t fixtureSource{};bool fixtureApplied{};
    bool init(){
        gta::OriginalImage image;if(!image.load())return false;
        auto hits=image.scan("55 8B EC 64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 83 EC 44 53 8B 5D 0C 80 7B 0A 00 56 8B F1");
        if(hits.size()!=1)return false;code=image.code(hits[0]);return true;
    }
};
}
