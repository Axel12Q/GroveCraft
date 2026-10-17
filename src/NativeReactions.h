#pragma once
namespace grove {
// Native tasks own aiming, gun animations, ammunition and flight. We never
// fabricate bullet damage or replace an existing combat task / mission ped.
class NativeReactions {
    using Allocate=void* (__cdecl*)(unsigned);
    using GunCtor=void* (__thiscall*)(void*,void*,gta::Vec,signed char,unsigned short,bool);
    using FleeCtor=void* (__thiscall*)(void*,const gta::Vec&,bool,float,int);
    using SetSecondary=void (__thiscall*)(void*,void*,int);
    using SetPrimary=void (__thiscall*)(void*,void*,int,bool);
    using Control=bool (__thiscall*)(void*,void*,void*,signed char);
    Allocate allocate{};GunCtor gunCtor{};FleeCtor fleeCtor{};
    SetSecondary secondary{};SetPrimary primary{};Control control{};
    struct Owned {std::uintptr_t ped{},task{};bool gun{};};
    std::map<unsigned,Owned> owned;ULONGLONG due{};
    static std::uintptr_t call(gta::OriginalImage& image,std::uintptr_t instruction){
        BYTE op{};int rel{};if(!gta::read(instruction,op)||op!=0xe8||!gta::read(instruction+1,rel))return 0;
        return image.code(instruction)+5+rel;
    }
public:
    bool supported{};unsigned shotsAssigned{},flightsAssigned{};
    bool init(){
        gta::OriginalImage image;if(!image.load())return false;
        auto calls=image.scan("6A 3C E8 ? ? ? ? 83 C4 04 89 45 E0 89 5D FC 3B C3 74 2F 0F B6 4E 3A 0F B7 56 2C 51 8B 4E 10 52 6A 01 83 EC 0C 8B D4 89 0A 8B 4E 14 89 4A 04 8B 4E 18 89 4A 08 8B 56 0C 52 8B C8 E8 ? ? ? ? EB 02 33 C0 8B 8F 7C 04 00 00 53 50 83 C1 04 C7 45 FC FF FF FF FF E8 ? ? ? ?");
        auto c=image.scan("55 8B EC 53 8B 5D 0C 56 8B F1 8B 4E 1C 57 8D 7E 1C C6 46 09 01 3B D9 74 29 85 C9");
        auto f=image.scan("55 8B EC 56 8B F1 E8 ? ? ? ? D9 45 10 8B 45 08 C7 06 ? ? ? ? 8B 08 89 4E 18 8B 50 04 8A 4D 0C 89 56 1C 8B 40 08 D9 5E 2C 8B 55 14 89 46 20 33 C0 89 56 28 88 4E 24 C7 46 30 07 00 00 00");
        auto p=image.scan("55 8B EC 53 8B 5D 08 56 57 8B 7D 0C 8B F1 85 DB 75 1D 8B 0C BE 85 C9 74 16 8B 01 8B 10 6A 01 FF D2 C7 04 BE 00 00 00 00");
        if(calls.size()!=1||c.size()!=1||f.size()!=1||p.size()!=1)return false;
        allocate=reinterpret_cast<Allocate>(call(image,calls[0]+2));
        gunCtor=reinterpret_cast<GunCtor>(call(image,calls[0]+60));
        secondary=reinterpret_cast<SetSecondary>(call(image,calls[0]+87));
        control=reinterpret_cast<Control>(image.code(c[0]));
        fleeCtor=reinterpret_cast<FleeCtor>(image.code(f[0]));primary=reinterpret_cast<SetPrimary>(image.code(p[0]));
        supported=allocate&&gunCtor&&secondary&&control&&fleeCtor&&primary;return supported;
    }
    template<class Clear> void tick(const gta::Snapshot& s,const NativeActors& actors,const NativeMobCombat& mobs,bool live,Clear clear){
        if(!supported)return;auto now=GetTickCount64();if(now<due)return;due=now+100;
        if(!live||!s.valid||s.menu||s.mission)return;
        std::set<unsigned> seen;
        actors.eachPed([&](unsigned id,std::uintptr_t ped){
            BYTE created{},slot{};DWORD bits{},state{},matrix{},brain{},pedType{};float health{};gta::Vec pos{};int weapon{};
            if(!gta::read(ped+0x484,created)||created!=1||!gta::read(ped+0x46c,bits)||(bits&256)
                ||!gta::read(ped+0x530,state)||state>=50||!gta::read(ped+0x540,health)||health<=0
                ||!gta::read(ped+0x14,matrix)||!gta::read(matrix+0x30,pos)||!gta::read(ped+0x47c,brain)
                ||!gta::read(ped+0x718,slot)||slot>=13||!gta::read(ped+0x5a0+slot*28,weapon)||!gta::read(ped+0x598,pedType))return;
            DWORD attack{},temp{};if(!gta::read(brain+4+0x14,attack)||!gta::read(brain+4+4,temp))return;
            bool gun=weapon>=22&&weapon<=34;auto it=owned.find(id);
            bool ours=it!=owned.end()&&it->second.ped==ped&&(it->second.gun?attack:temp)==it->second.task;
            if(ours)seen.insert(id);else if(it!=owned.end())owned.erase(it);
            float best=gun?18.f:6.f;gta::Vec target{};bool found=false;
            mobs.each([&](unsigned,unsigned victim,unsigned flags,gta::Vec feet,float height){
                if((flags&p::kMobPassive)&&victim!=id)return;
                float distance=std::hypot(feet.x-pos.x,feet.y-pos.y);
                if(distance>=best||std::abs(feet.z-pos.z)>4)return;
                if(gun&&victim!=id&&pedType!=6&&(pedType<7||pedType>16))return;
                gta::Vec point{feet.x,feet.y,feet.z+height*.65f};gta::Vec eye{pos.x,pos.y,pos.z+.6f};
                if(!clear(eye,point))return;target=point;best=distance;found=true;
            });
            if(ours){
                if(it->second.gun){
                    if(found&&gun&&gta::accessible(reinterpret_cast<void*>(it->second.task+0x20),12,true)){
                        std::memcpy(reinterpret_cast<void*>(it->second.task+0x20),&target,12);
                        control(reinterpret_cast<void*>(it->second.task),reinterpret_cast<void*>(ped),nullptr,2);
                    }else control(reinterpret_cast<void*>(it->second.task),reinterpret_cast<void*>(ped),nullptr,6);
                }return;
            }
            if(!found||(gun?attack:temp))return;
            void* task=allocate(gun?0x3c:0x44);if(!task)return;
            if(gun){gunCtor(task,nullptr,target,2,1,false);secondary(reinterpret_cast<void*>(brain+4),task,0);++shotsAssigned;}
            else{fleeCtor(task,target,true,18.f,4000);primary(reinterpret_cast<void*>(brain+4),task,1,false);++flightsAssigned;}
            owned[id]={ped,reinterpret_cast<std::uintptr_t>(task),gun};seen.insert(id);
        });
        for(auto it=owned.begin();it!=owned.end();)if(!seen.count(it->first))it=owned.erase(it);else ++it;
    }
};
}
