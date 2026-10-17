#pragma once
#include "../vendor/libertycraft/asi/src/collision/Geometry.h"
namespace grove {
// Distant open grass needs a surface/headroom survey, not underground chains
// and thousands of wall probes. Reject the whole tile on roofs, foliage, gaps,
// steep faces or malformed hits; nearby detailed sampling still owns walls/mines.
class GroundProbe {
    std::shared_ptr<lc::col::Column> column;unsigned cursor{};bool rejected{};
    float low{INFINITY},high{-INFINITY};
public:
    GroundProbe(int x,int z,float y){column=std::make_shared<lc::col::Column>();column->rx=x;column->rz=z;column->yLo=y-24;column->yHi=y+40;}
    int rx()const{return column->rx;}int rz()const{return column->rz;}
    bool valid()const{return !rejected&&cursor==lc::col::kSamples;}
    std::shared_ptr<const lc::col::Column> result()const{return column;}
    template<class Ray,class Budget> bool run(Ray ray,Budget exhausted){
        if(rejected)return true;
        while(cursor<lc::col::kSamples){
            unsigned index=cursor++;float x=column->X(index%lc::col::kGrid),z=column->Z(index/lc::col::kGrid);
            float a[3]={x,column->yHi,z},b[3]={x,column->yLo,z};lc::col::Hit h{};
            if(!ray(a,b,h)||(h.mat!=37&&h.mat!=128+libertycraft::proto::kDigDirt)
                ||!std::isfinite(h.pos[0]+h.pos[1]+h.pos[2]+h.n[0]+h.n[1]+h.n[2])
                ||std::abs(h.pos[0]-x)>.05f||std::abs(h.pos[2]-z)>.05f||h.pos[1]<b[1]||h.pos[1]>a[1]
                ||std::abs(h.n[1])<.65f){rejected=true;return true;}
            lc::col::Event e{};e.y=h.pos[1];e.top=true;e.mat=h.mat;e.n[0]=h.n[0];e.n[1]=std::abs(h.n[1]);e.n[2]=h.n[2];
            column->events.push_back(e);column->first[cursor]=unsigned(column->events.size());
            low=std::min(low,e.y);high=std::max(high,e.y);
            if(exhausted())break;
        }
        if(cursor==lc::col::kSamples){column->yLo=std::floor((low-2)/8)*8-2;column->yHi=std::ceil((high+12)/8)*8+2;return true;}
        return false;
    }
};
}
