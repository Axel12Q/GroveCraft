#pragma once
namespace grove {
// Signature checked against the original executable and CJ's virtual Teleport.
// The function removes/re-adds the ped to CWorld; CVector is passed by value.
class NativeBedSpawn {
    using Teleport=void (__thiscall*)(void*,gta::Vec,bool);
    std::uintptr_t teleportCode{};
    unsigned accepted{};ULONGLONG lastTick{},elapsed{},failedUntil{};
    gta::Vec destination{},fallback{};
public:
    bool active{};unsigned successes{},failures{};
    bool init(){
        gta::OriginalImage image;if(!image.load())return false;
        auto found=image.scan("55 8B EC 83 EC 08 56 8B F1 8B 86 98 05 00 00 57 85 C0 74 1C 83 F8 01 74 17 8B 8E 7C 04 00 00 68 C0 02 00 00 8D 49 04 E8 ? ? ? ? 85 C0 74 0D 8B 8E 7C 04 00 00 6A 01 E8 ? ? ? ? 56 E8");
        if(found.size()!=1)return false;teleportCode=image.code(found[0]);return teleportCode!=0;
    }
    bool teleport(const gta::Snapshot& s,gta::Vec to){
        if(!teleportCode||!s.valid||s.health<=0||s.menu||s.mission||s.vehicle)return false;
        DWORD vt{},fn{};BYTE first{};
        if(!gta::read(s.ped,vt)||!gta::read(vt+14*4,fn)||fn!=teleportCode
            ||!gta::read(fn,first)||first!=0x55)return false;
        reinterpret_cast<Teleport>(fn)(reinterpret_cast<void*>(s.ped),to,false);return true;
    }
    bool request(const gta::Snapshot& s,unsigned nonce,unsigned current,bool death,gta::Vec to,unsigned area){
        if(active||!death||nonce!=current||nonce==accepted||area!=0||!std::isfinite(to.x+to.y+to.z)
            ||std::abs(to.x)>6000||std::abs(to.y)>6000||to.z< -59||to.z>1001)return false;
        fallback=s.position;
        if(!teleport(s,to)){failedUntil=GetTickCount64()+2500;++failures;return false;}
        destination=to;accepted=nonce;active=true;elapsed=0;lastTick=GetTickCount64();return true;
    }
    template<class Support> void tick(const gta::Snapshot& s,const gta::Bindings& b,bool running,Support support){
        auto now=GetTickCount64();auto delta=std::min<ULONGLONG>(now-lastTick,100);lastTick=now;
        if(!active)return;
        if(!s.valid||s.health<=0||s.mission||s.vehicle){active=false;failedUntil=now+2500;++failures;return;}
        b.correct(s,destination,7,true);
        if(!running)return;elapsed+=delta;
        if(elapsed>=1500&&support(destination)){active=false;++successes;return;}
        if(elapsed>=10000){teleport(s,fallback);active=false;failedUntil=now+2500;++failures;}
    }
    bool failed()const{return GetTickCount64()<failedUntil;}
};
}
