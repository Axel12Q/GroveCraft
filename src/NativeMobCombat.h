#pragma once
namespace grove {
struct NativeHitPoint {gta::Vec point;float pad{};gta::Vec normal;float pad2{};BYTE surfaceA{},pieceA{},lightA{},surfaceB{},pieceB{},lightB{},padA{},padB{};float depth{};};
static_assert(sizeof(NativeHitPoint)==0x2c&&offsetof(NativeHitPoint,surfaceB)==0x23);
class NativeMobCombat {
    struct Mob {unsigned id{},target{},flags{};gta::Vec feet{};float width{},height{};ULONGLONG stamp{};};
    std::map<unsigned,Mob> mobs;
    using Info=void* (__cdecl*)(int,int);
    Info info{};
public:
    using Impact=void (__thiscall*)(void*,void*,void*,const gta::Vec&,const gta::Vec&,const NativeHitPoint&,int);
    using Explosion=bool (__cdecl*)(void*,void*,int,gta::Vec,unsigned,unsigned char,float,unsigned char);
    Impact impact{};Explosion explosion{};std::uintptr_t impactCode{},explosionCode{};
    unsigned bulletHits{},blasts{},hostBlasts{},bulletCalls{};int lastWeapon{},bulletStage{};gta::Vec lastFrom{},lastTo{};bool mirroring{};
    bool init(){
        gta::OriginalImage image;if(!image.load())return false;
        auto f=image.scan("55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 68 53 56 8B F1 8B 06 57 6A 01 50 89 75 F0 E8 ? ? ? ? 8B 5D 08 83 C4 08 89 45 E8 85 DB 74 24 8A 4B 36 80 E1 07 80 F9 03");
        auto x=image.scan("55 8B EC 83 EC 20 53 56 57 6A FF C6 45 FD 00 E8 ? ? ? ? 8B 5D 0C 83 C4 04 3B D8 75 26 0F B6 05 ? ? ? ? 69 C0 90 01 00 00 83 80 ? ? ? ? 05");
        if(f.size()!=1||x.size()!=1)return false;
        impactCode=image.code(f[0]);explosionCode=image.code(x[0]);
        std::int32_t relative{};if(!gta::read(f[0]+41,relative))return false;
        info=reinterpret_cast<Info>(impactCode+45+relative);return true;
    }
    void report(const p::McEvent& e,const gta::Snapshot& s){
        float width=(e.weapon&65535)*.01f;
        if(!e.flags||!std::isfinite(e.a+e.b+e.c+e.d)||e.d<.1f||e.d>12||width<.1f||width>12
            ||std::hypot(e.a-s.position.x,-e.c-s.position.y)>100)return;
        auto now=GetTickCount64();for(auto it=mobs.begin();it!=mobs.end();)if(now-it->second.stamp>1000)it=mobs.erase(it);else ++it;
        if(mobs.size()>=128&&!mobs.count(e.flags))return;
        mobs[e.flags]={e.flags,e.formId,e.weapon,{e.a,-e.c,e.b},width,e.d,now};
    }
    std::size_t count()const{return mobs.size();}
    template<class Fn> void each(Fn fn)const{for(const auto& [id,m]:mobs)if(GetTickCount64()-m.stamp<500)fn(id,m.target,m.flags,m.feet,m.height);}
    template<class Send> bool bullet(void* weapon,void* shooter,void* victim,const gta::Vec& start,const gta::Vec& end,
        const NativeHitPoint& cp,const gta::Snapshot& s,int shooterId,Send send,gta::Vec& stopped){
        ++bulletCalls;bulletStage=1;lastFrom=start;lastTo=victim?cp.point:end;
        gta::read(reinterpret_cast<std::uintptr_t>(weapon),lastWeapon);
        if(!s.valid||s.menu||s.mission||!info)return false;
        bulletStage=2;
        auto limit=victim?cp.point:end;float dir[3]={limit.x-start.x,limit.y-start.y,limit.z-start.z};
        float origin[3]={start.x,start.y,start.z};unsigned target{};float closest=1;
        for(const auto& [id,m]:mobs){
            if(GetTickCount64()-m.stamp>750)continue;
            float low[3]={m.feet.x-m.width/2,m.feet.y-m.width/2,m.feet.z},high[3]={m.feet.x+m.width/2,m.feet.y+m.width/2,m.feet.z+m.height};
            float enter=0,leave=closest;bool hit=true;
            for(unsigned axis=0;axis<3;++axis){
                if(!std::isfinite(dir[axis]+origin[axis]))return false;
                if(std::abs(dir[axis])<.00001f){if(origin[axis]<low[axis]||origin[axis]>high[axis])hit=false;}
                else{float a=(low[axis]-origin[axis])/dir[axis],b=(high[axis]-origin[axis])/dir[axis];
                    if(a>b)std::swap(a,b);enter=std::max(enter,a);leave=std::min(leave,b);if(enter>leave)hit=false;}
            }
            if(hit&&enter<closest){closest=enter;target=id;}
        }
        if(!target)return false;bulletStage=3;int type{};unsigned short damage{};
        if(!gta::read(reinterpret_cast<std::uintptr_t>(weapon),type)||type<22||type>34)return false;
        auto details=reinterpret_cast<std::uintptr_t>(info(type,1));if(!gta::read(details+0x22,damage)||!damage||damage>1000)return false;
        bulletStage=4;
        if(!send(p::kInMobHit,unsigned(type),int(target),shooter==reinterpret_cast<void*>(s.ped)?p::kMobHitByPlayer:shooterId,int(damage)*20))return false;
        bulletStage=5;
        stopped={start.x+dir[0]*closest,start.y+dir[1]*closest,start.z+dir[2]*closest};++bulletHits;return true;
    }
    bool blast(const p::McEvent& e,const gta::Snapshot& s){
        if(!explosion||!s.valid||s.menu||s.mission||!std::isfinite(e.a+e.b+e.c+e.d)||e.d<=0||e.d>16
            ||std::hypot(e.a-s.position.x,-e.c-s.position.y)>128)return false;
        mirroring=true;bool made=explosion(nullptr,nullptr,0,{e.a,-e.c,e.b},0,1,.25f,0);mirroring=false;
        if(made)++blasts;return made;
    }
};
}
