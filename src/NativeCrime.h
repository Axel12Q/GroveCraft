#pragma once
namespace grove {
// Raise at least one star through CWanted, never lower an existing pursuit.
// Exact owned NewSteam PE signature verified offline; no 1.0 address fallback.
class NativeCrime {
    using Wanted=void (__thiscall*)(void*,int);
    Wanted raise{};ULONGLONG last{};
public:
    bool supported{};unsigned witnessed{},unseen{},rejected{};
#ifdef GROVE_CRIME_TEST
    void fixture(Wanted fn){raise=fn;supported=true;last=0;}
#endif
    bool init(){
        gta::OriginalImage image;if(!image.load())return false;
        auto hits=image.scan("55 8B EC 56 8B F1 8B 46 30 39 46 2C 7D 06 50 E8 ? ? ? ? 8B 45 08 3B 46 2C 7E 08 50 8B CE E8 ? ? ? ? 5E 5D C2 04 00");
        if(hits.size()!=1)return false;raise=reinterpret_cast<Wanted>(image.code(hits[0]));return supported=raise!=nullptr;
    }
    template<class Actors,class Clear> bool vandalism(const gta::Snapshot& s,const Actors& actors,gta::Vec target,Clear clear){
        auto now=GetTickCount64();if(!supported||!s.valid||s.menu||s.mission||s.vehicle||s.health<=0
            ||!std::isfinite(target.x+target.y+target.z)||std::hypot(target.x-s.position.x,target.y-s.position.y)>9
            ||std::abs(target.z-s.position.z)>9){++rejected;return false;}
        if(now-last<500)return false;last=now;bool seen=false;
        actors.eachPed([&](unsigned,std::uintptr_t cop){
            DWORD type{},state{},matrix{};float hp{};BYTE created{};gta::Vec pos{},forward{};
            if(seen||!gta::read(cop+0x598,type)||type!=6||!gta::read(cop+0x484,created)||created!=1
                ||!gta::read(cop+0x540,hp)||hp<=0||!gta::read(cop+0x530,state)||state>=50
                ||!gta::read(cop+0x14,matrix)||!gta::read(matrix+0x30,pos)||!gta::read(matrix+0x10,forward))return;
            float dx=s.position.x-pos.x,dy=s.position.y-pos.y,distance=std::hypot(dx,dy);
            if(!std::isfinite(distance+forward.x+forward.y)||distance>40||std::abs(pos.z-s.position.z)>10)return;
            float len=std::hypot(forward.x,forward.y);
            if(distance>3&&(len<.1f||(dx*forward.x+dy*forward.y)<.2f*distance*len))return;
            seen=clear({pos.x,pos.y,pos.z+.7f},{s.position.x,s.position.y,s.position.z+.6f});
        });
        if(!seen){++unseen;return false;}
        DWORD data{},wanted{};unsigned level{},parole{};
        if(!gta::read(s.ped+0x480,data)||!data||!gta::read(data,wanted)||!wanted
            ||!gta::read(wanted+0x2c,level)||level>6||!gta::read(wanted+0x30,parole)||parole>6
            ||!gta::accessible(reinterpret_cast<void*>(wanted),0x34,true)){++rejected;return false;}
        raise(reinterpret_cast<void*>(wanted),1);++witnessed;return true;
    }
};
}
