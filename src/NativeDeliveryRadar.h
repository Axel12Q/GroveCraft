#pragma once
namespace grove {
// Owned coordinate trace only. Native API and global are resolved in this exact original PE.
class NativeDeliveryRadar {
    using Set=unsigned (__cdecl*)(int,gta::Vec,int,int);using Clear=void (__cdecl*)(unsigned);
    Set set{};Clear clear{};ULONGLONG due{};std::uintptr_t traces{};unsigned handle=0xffffffffu;
    static constexpr unsigned tag=0x4757444cu;
    bool own(unsigned index)const{unsigned mark{};BYTE flags{},type{};return index<175&&gta::read(traces+index*40+4,mark)&&mark==tag
        &&gta::read(traces+index*40+37,flags)&&(flags&2)&&gta::read(traces+index*40+38,type)&&((type>>2)&15)==4;}
    bool current()const{unsigned i=handle&65535;unsigned short counter{};return handle!=0xffffffffu&&own(i)&&gta::read(traces+i*40+20,counter)&&counter==(handle>>16);}
    void remove(){if(current())clear(handle);handle=0xffffffffu;}
public:
    bool supported{};unsigned updates{},created{},removed{};
    bool owned(unsigned h,unsigned marker)const{
        if(!supported||h==0xffffffffu||(h&65535)>=175)return false;
        auto a=traces+(h&65535)*40;unsigned m{};unsigned short n{};BYTE f{},t{};
        return gta::read(a+4,m)&&m==marker&&gta::read(a+20,n)&&n==(h>>16)
            &&gta::read(a+37,f)&&(f&2)&&gta::read(a+38,t)&&((t>>2)&15)==4;
    }
    bool eraseOwned(unsigned h,unsigned marker){if(!owned(h,marker))return false;clear(h);return true;}
    void clearTag(unsigned marker){if(!supported)return;for(unsigned i=0;i<175;++i){unsigned short n{};
        if(gta::read(traces+i*40+20,n))eraseOwned(i|(unsigned(n)<<16),marker);}}
    template<class Handles> void pruneTag(unsigned marker,const Handles& keep){if(!supported)return;
        for(unsigned i=0;i<175;++i){unsigned short n{};if(!gta::read(traces+i*40+20,n))continue;
            unsigned h=i|(unsigned(n)<<16);if(std::find(keep.begin(),keep.end(),h)==keep.end())eraseOwned(h,marker);}}
    bool inspectOwned(unsigned h,unsigned marker,gta::Vec& pos,BYTE& sprite,BYTE& flags)const{
        auto a=traces+(h&65535)*40;return owned(h,marker)&&gta::read(a+8,pos)&&gta::read(a+36,sprite)&&gta::read(a+37,flags);
    }
    void shortRangeOwned(unsigned h,unsigned marker,bool enabled){
        if(!owned(h,marker))return;auto a=traces+(h&65535)*40+37;BYTE f{};
        if(gta::read(a,f)&&gta::accessible(reinterpret_cast<void*>(a),1,true)){
            f=enabled?BYTE(f|4):BYTE(f&~4);std::memcpy(reinterpret_cast<void*>(a),&f,1);}
    }
    unsigned freeSlots()const{
        unsigned free=0;if(supported){for(unsigned i=0;i<175;++i){BYTE f{};
            if(gta::read(traces+i*40+37,f)&&!(f&2))++free;}}
        return free;
    }
    unsigned createOwned(unsigned marker,gta::Vec pos,BYTE sprite){
        // Never compete with script mission blips for the final slots.
        if(!supported||freeSlots()<=24)return 0xffffffffu;
        unsigned h=set(4,pos,4,2);if(h==0xffffffffu||(h&65535)>=175)return 0xffffffffu;
        auto a=traces+(h&65535)*40;unsigned short n{};BYTE f{},t{};
        if(!gta::read(a+20,n)||n!=(h>>16)||!gta::read(a+37,f)||!(f&2)||!gta::read(a+38,t)
            ||((t>>2)&15)!=4||!gta::accessible(reinterpret_cast<void*>(a),40,true))return 0xffffffffu;
        // Layout checked against this executable and tRadarTrace: +36 sprite, +37 short-range bit.
        f|=4;std::memcpy(reinterpret_cast<void*>(a+4),&marker,4);
        std::memcpy(reinterpret_cast<void*>(a+36),&sprite,1);std::memcpy(reinterpret_cast<void*>(a+37),&f,1);
        return h;
    }
    unsigned id()const{return current()?handle:0xffffffffu;}
    bool init(){gta::OriginalImage image;if(!image.load())return false;
        auto a=image.scan("55 8B EC 56 B1 02 33 D2 33 F6 84 0D ? ? ? ? 74 25 33 C0 3D 58 1B 00 00");
        auto b=image.scan("55 8B EC 8B 55 08 83 FA FF 74 66 8B CA 81 E1 FF FF 00 00 8D 04 89 03 C0 03 C0 03 C0 C1 EA 10 66 3B 90 ? ? ? ? 75 49 F6 80 ? ? ? ? 02");
        DWORD flags{};if(a.size()!=1||b.size()!=1||!gta::read(a[0]+12,flags))return false;
        traces=image.global(flags-37);set=reinterpret_cast<Set>(image.code(a[0]));clear=reinterpret_cast<Clear>(image.code(b[0]));supported=true;return true;
    }
    void tick(const gta::Snapshot& s,bool live,unsigned kind,gta::Vec point,ULONGLONG stamp){
        if(!supported||s.menu||!s.valid||GetTickCount64()<due)return;due=GetTickCount64()+250;
        bool wanted=live&&!s.mission&&s.health>0&&kind&&GetTickCount64()-stamp<1500&&std::isfinite(point.x+point.y+point.z);
        // Save/load can restore our trace. Adopt tagged records; never touch native/script blips.
        if(!current()){handle=0xffffffffu;for(unsigned i=0;i<175;i++)if(own(i)){unsigned short n{};
            if(!gta::read(traces+i*40+20,n))continue;unsigned h=i|(unsigned(n)<<16);
            if(handle==0xffffffffu)handle=h;else{clear(h);++removed;}}}
        if(!wanted){if(current())++removed;remove();return;}
        if(current()){gta::Vec old{};if(gta::read(traces+(handle&65535)*40+8,old)
            &&std::hypot(old.x-point.x,old.y-point.y)<.25f&&std::abs(old.z-point.z)<.25f)return;
            remove();}
        unsigned h=set(4,point,4,2);if(h==0xffffffffu||(h&65535)>=175)return;
        auto address=traces+(h&65535)*40;unsigned short n{};BYTE flags{},type{};
        if(!gta::read(address+20,n)||n!=(h>>16)||!gta::read(address+37,flags)||!(flags&2)
            ||!gta::read(address+38,type)||((type>>2)&15)!=4||!gta::accessible(reinterpret_cast<void*>(address),40,true))return;
        unsigned short size=2;std::memcpy(reinterpret_cast<void*>(address+28),&size,2);
        unsigned yellow=4;std::memcpy(reinterpret_cast<void*>(address),&yellow,4);std::memcpy(reinterpret_cast<void*>(address+4),&tag,4);
        handle=h;++created;++updates;
    }
};
}
