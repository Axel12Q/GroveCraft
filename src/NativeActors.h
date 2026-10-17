#pragma once
#include "NativeVehicles.h"
namespace grove {
// Export only validated ambient peds. Wire IDs are monotonic leases, never pointers
// or wrapping pool handles; recycled pool slots cannot receive an old mob's damage.
class NativeActors {
    struct Lease {unsigned id{};BYTE generation{};std::uintptr_t address{};ULONGLONG stamp{};};
    std::map<unsigned,Lease> slots;
    unsigned nextId=1,seq{};ULONGLONG due{};
public:
    NativeVehicles vehicles;
    unsigned count{},damageHits{},rejected{};
    template<class Fn> void eachPed(Fn fn)const{for(const auto& [i,l]:slots)if(GetTickCount64()-l.stamp<500)fn(l.id,l.address);}
    int identity(std::uintptr_t ped)const{for(const auto& [i,l]:slots)if(l.address==ped&&GetTickCount64()-l.stamp<500)return int(l.id);return 0;}
    void publish(unsigned char* mem,const gta::Snapshot& s,NativeMelee& melee,bool live){
        auto now=GetTickCount64();if(now<due)return;due=now+100;
        std::vector<p::ActorRecord> records;std::set<unsigned> seen;
        DWORD pool{},base{},flags{};int size{};
        if(live&&s.valid&&!s.menu&&!s.mission&&melee.supported
            &&gta::read(melee.pedPool(),pool)&&gta::read(pool,base)&&gta::read(pool+4,flags)
            &&gta::read(pool+8,size)&&size>0&&size<=256){
            for(int i=0;i<size;++i){
                BYTE generation{},type{},created{};DWORD state{},bits{},matrix{};gta::Vec pos{};float health{},maximum{},heading{};
                auto address=std::uintptr_t(base)+i*0x7c4;
                if(address==s.ped||!gta::read(flags+i,generation)||(generation&128)
                    ||!gta::read(address+0x36,type)||(type&7)!=3||!gta::read(address+0x484,created)
                    ||!gta::read(address+0x46c,bits)||(bits&256)||!gta::read(address+0x530,state)
                    ||!gta::read(address+0x540,health)||!gta::read(address+0x544,maximum)
                    ||!gta::read(address+0x558,heading)||!gta::read(address+0x14,matrix)||!gta::read(matrix+0x30,pos))continue;
                if(!std::isfinite(pos.x)||!std::isfinite(pos.y)||!std::isfinite(pos.z)||!std::isfinite(health)
                    ||!std::isfinite(maximum)||maximum<=0||!std::isfinite(heading)
                    ||std::hypot(pos.x-s.position.x,pos.y-s.position.y)>80||std::abs(pos.z-s.position.z)>24)continue;
                auto& lease=slots[i];if(!lease.id||lease.generation!=generation||lease.address!=address){
                    if(nextId>=0x50000000)continue;lease={nextId++,generation,address,now};
                }lease.stamp=now;seen.insert(i);
                p::ActorRecord r{};r.formId=lease.id;r.x=pos.x;r.y=pos.z-1;r.z=-pos.y;
                r.yaw=-heading*180.f/3.14159265f;r.width=.6f;r.height=1.8f;r.healthFrac=std::clamp(health/maximum,0.f,1.f);
                r.level=1;if(health<=0||grove::dyingState(state))r.flags|=p::kActorDead;
                if(created!=1)r.flags|=p::kActorMission;
                std::strcpy(r.name,"San Andreas pedestrian");records.push_back(r);
            }
        }
        for(auto it=slots.begin();it!=slots.end();)if(!seen.count(it->first))it=slots.erase(it);else ++it;
        vehicles.append(records,s,live&&s.valid&&!s.menu&&!s.mission);
        auto* out=reinterpret_cast<p::ActorTable*>(mem+p::kOffActorTable);
        LARGE_INTEGER q{},f{};QueryPerformanceCounter(&q);QueryPerformanceFrequency(&f);
        InterlockedExchange(reinterpret_cast<volatile LONG*>(&out->seq),++seq);
        count=static_cast<unsigned>(records.size());out->count=count;
        out->stamp=f.QuadPart?static_cast<std::uint64_t>((q.QuadPart/f.QuadPart)*10000000ull
            +(q.QuadPart%f.QuadPart)*10000000ull/f.QuadPart):0;
        if(count)std::memcpy(out->actors,records.data(),records.size()*sizeof(p::ActorRecord));
        MemoryBarrier();InterlockedExchange(reinterpret_cast<volatile LONG*>(&out->seq),++seq);
    }
    bool hit(const p::McEvent& e,const gta::Snapshot& s,NativeMelee& melee){
        // Event 3 already creates GTA's own explosion. Proxy blast hits must not
        // apply a second damage event to the same native victim.
        if(e.flags&p::kHitExplosion)return true;
        if(!s.valid||s.menu||s.mission||s.health<=0){++rejected;return false;}
        if((e.formId&0xff000000u)==p::kActorVehicleTag)return vehicles.hit(e,s);
        std::uintptr_t victim{};
        if(e.type==p::kEvMobHitPlayer){if(s.vehicle)return false;victim=s.ped;}
        else for(const auto& [index,l]:slots)if(l.id==e.formId&&GetTickCount64()-l.stamp<=500){
            DWORD pool{},base{},flags{};BYTE generation{},created{};
            if(gta::read(melee.pedPool(),pool)&&gta::read(pool,base)&&gta::read(pool+4,flags)
                &&std::uintptr_t(base)+index*0x7c4==l.address&&gta::read(flags+index,generation)
                &&generation==l.generation&&!(generation&128)&&gta::read(l.address+0x484,created)&&created==1)victim=l.address;
            break;
        }
        if(!victim){++rejected;return false;}
        // Mob blows have no native CJ creator, so CJ receives no crime credit.
        auto creator=e.type==p::kEvMobHitPlayer||(e.flags&p::kHitByMob)?0:s.ped;
        if(melee.apply(victim,creator,e.a,e.weapon)){++damageHits;return true;}++rejected;return false;
    }
};
}
