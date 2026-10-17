#pragma once
#include "../vendor/libertycraft/asi/src/collision/Geometry.h"
namespace grove {
// DigWalls/HostDig look 2.5 metres around each neighbouring face. Sample
// these columns before spending the interaction budget on distant parcels.
inline std::vector<std::array<int,2>> surfaceColumns(int x,int z){
    std::array<int,2> center{int(std::floor(x/8.0)),int(std::floor(z/8.0))};
    std::vector<std::array<int,2>> out{center};
    for(int rz=int(std::floor((z-4)/8.0));rz<=int(std::floor((z+4)/8.0));++rz)
        for(int rx=int(std::floor((x-4)/8.0));rx<=int(std::floor((x+4)/8.0));++rx)
            if(std::array<int,2>{rx,rz}!=center)out.push_back({rx,rz});
    return out;
}
// SA procedural landscape materials are still ground, not unknown objects.
// Confirmed against the installation's surfinfo/surfaud and COL: cunteground13
// uses P_BUSHY (111), including the underside encountered from an excavation.
inline unsigned landscapeMaterial(unsigned material) {
    namespace p=libertycraft::proto;
    if(material==109)return p::kDigStone; // P_MOUNTAIN
    if(material==110)return p::kDigMud;   // P_MARSH
    if(material>=111&&material<=114)return p::kDigDirt; // P_BUSHY variants
    return 0;
}
inline unsigned facadeMaterial(unsigned m){
    namespace p=libertycraft::proto;
    if(m<=8||m==89||m==69)return p::kDigStone;
    if((m>=42&&m<=44)||m==70||m==72||m==73||m==90||m==94||m==95)return p::kDigPlanks;
    if(m>=45&&m<=47)return p::kDigGlass;
    if(m>=50&&m<=59)return p::kDigMetal;
    return 0;
}
inline bool exactFacade(const float* v,unsigned material,lc::col::Tri& out){
    unsigned dig=facadeMaterial(material);if(!dig)return false;
    float a[3],b[3],n[3];for(int i=0;i<9;i++)if(!std::isfinite(v[i]))return false;
    for(int i=0;i<3;i++){a[i]=v[i+3]-v[i];b[i]=v[i+6]-v[i];}
    lc::col::Cross(a,b,n);float len=std::sqrt(lc::col::Dot(n,n));
    if(len<.0001f||std::abs(n[1])/len>=.7f)return false;
    std::memcpy(out.v,v,sizeof out.v);out.kind=lc::col::kWall;out.fill=0;
    out.flags=lc::col::MaterialFlags(static_cast<std::uint8_t>(128+dig));return true;
}
inline bool earthSample(unsigned material) {
    namespace p=libertycraft::proto;
    unsigned dig=material==37?p::kDigGrass:material>=128&&material<128+p::kDigMaterialCount?material-128:0;
    return dig==p::kDigGrass||dig==p::kDigDirt||dig==p::kDigStone||dig==p::kDigCobble||dig==p::kDigSand||dig==p::kDigGravel||dig==p::kDigMud;
}
// SA raycasts see both faces of a ground sheet. Remove the reverse face before
// GapSolid and wall evidence interpret the space above it as a sealed building.
inline void sanitizeTerrainColumn(lc::col::Column& column) {
    std::vector<lc::col::Event> cleaned;
    auto originalFirst=column.first;
    for(int sample=0;sample<lc::col::kSamples;++sample) {
        auto begin=originalFirst[sample],end=originalFirst[sample+1];
        float ground=INFINITY;
        for(auto i=begin;i<end;++i)if(column.events[i].top&&earthSample(column.events[i].mat))ground=std::min(ground,column.events[i].y);
        column.first[sample]=static_cast<unsigned>(cleaned.size());
        for(auto i=begin;i<end;++i){const auto& e=column.events[i];
            if(!e.top&&std::abs(e.y-ground)<.05f)continue;
            cleaned.push_back(e);
        }
    }
    column.first[lc::col::kSamples]=static_cast<unsigned>(cleaned.size());
    column.events=std::move(cleaned);
    column.walls.erase(std::remove_if(column.walls.begin(),column.walls.end(),[&](const auto& w){
        if(w.hit||w.kind!=lc::col::kWallEvidence)return false;
        int i=w.sample%lc::col::kGrid+lc::col::kDirI[w.dir];
        int j=w.sample/lc::col::kGrid+lc::col::kDirJ[w.dir];
        return i>=0&&j>=0&&i<lc::col::kGrid&&j<lc::col::kGrid&&!column.SolidAt(lc::col::Column::Index(i,j),w.y);
    }),column.walls.end());
}
}
