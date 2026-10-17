#pragma once
#include "CargoRules.h"
namespace grove {
class NativeVehicles {
    using Damage=void (__thiscall*)(void*,void*,int,float,gta::Vec);
    Damage damage{};
    using Door=void (__thiscall*)(void*,void*,int,int,float,bool);
    Door openDoor{};

    struct Lease {unsigned id{};BYTE generation{};DWORD address{};};
    std::map<unsigned,Lease> leases;unsigned next=1;std::uintptr_t poolGlobal{},models{};
    Lease trunk{};unsigned trunkIndex{};float trunkRatio{};bool trunkWanted{};ULONGLONG trunkTick{};
    bool validLease(const Lease& l,unsigned index)const{
        DWORD pool{},base{},flags{};int size{};BYTE gen{},kind{};
        return l.id&&gta::read(poolGlobal,pool)&&gta::read(pool,base)&&gta::read(pool+4,flags)&&gta::read(pool+8,size)
            &&size>0&&size<=1024&&index<unsigned(size)&&base+index*0xa18==l.address
            &&gta::read(flags+index,gen)&&gen==l.generation&&!(gen&128)
            &&gta::read(l.address+0x36,kind)&&(kind&7)==2;
    }
public:
    void invalidateLeases(){leases.clear();cargoCandidate=0;trunk={};trunkRatio=0;trunkWanted=false;}
    // Roadside work targets an empty parked lease at CJ's rear access point.
    bool repairTarget(const gta::Snapshot& s,unsigned id,std::uintptr_t& target,BYTE& generation,float& health)const{
        if(!s.valid||s.menu||s.mission||s.vehicle||s.health<=0||id!=cargoCandidate||GetTickCount64()-cargoStamp>300)return false;
        for(const auto& [index,l]:leases){
            if(l.id!=id||!validLease(l,index))continue;
            DWORD driver{},matrix{};BYTE created{};gta::Vec pos{},speed{};
            if(!gta::read(l.address+0x460,driver)||driver||!gta::read(l.address+0x4a4,created)||created==2
                ||!gta::read(l.address+0x4c0,health)||!std::isfinite(health)||health<=250
                ||!gta::read(l.address+0x14,matrix)||!gta::read(matrix+0x30,pos)||!gta::read(l.address+0x44,speed)
                ||!std::isfinite(pos.x+pos.y+pos.z+speed.x+speed.y+speed.z)
                ||std::hypot(pos.x-s.position.x,pos.y-s.position.y)>6||std::abs(pos.z-s.position.z)>2
                ||std::sqrt(speed.x*speed.x+speed.y*speed.y+speed.z*speed.z)>=.02f)return false;
            target=l.address+0x4c0;generation=l.generation;return gta::accessible(reinterpret_cast<void*>(target),4,true);
        }
        return false;
    }
    unsigned cargoContents{};ULONGLONG contentsStamp{};unsigned trunkOpens{},trunkCloses{},cargoProps{};
    bool trunkSupported()const{return openDoor!=nullptr;}
    unsigned trunkId()const{return trunk.id;}
    float trunkOpening()const{return trunkRatio;}
    bool toggleTrunk(const gta::Snapshot& s,bool toggle){
        if(!openDoor||!s.valid||s.menu||s.mission||s.vehicle||s.health<=0||!cargoCandidate||GetTickCount64()-cargoStamp>300)return false;
        if(trunk.id&&validLease(trunk,trunkIndex)){
            for(const auto& [index,l]:leases)if(l.id==cargoCandidate&&validLease(l,index)
                &&l.address==trunk.address&&l.generation==trunk.generation){
                trunk=l;trunkIndex=index;trunkWanted=toggle?!trunkWanted:true;return true;
            }
            return false; // Close this car before opening another.
        }
        for(const auto& [index,l]:leases)if(l.id==cargoCandidate&&validLease(l,index)){
            DWORD frame{};BYTE status{};float angle{},closed{};
            // Original executable confirms boot node 17 and CDamageManager door 1.
            if(!gta::read(l.address+0x68c,frame)||!frame||!gta::accessible(reinterpret_cast<void*>(frame),80)
                ||!gta::read(l.address+0x5aa,status)||status!=0
                ||!gta::read(l.address+0x5dc,angle)||!gta::read(l.address+0x5d4,closed)
                ||!std::isfinite(angle+closed)||std::abs(angle-closed)>.05f)return false;
            trunk=l;trunkIndex=index;trunkRatio=0;trunkWanted=true;trunkTick=GetTickCount64();++trunkOpens;return true;
        }
        return false;
    }
    void tickTrunk(const gta::Snapshot& s,bool connected,bool active){
        if(!trunk.id)return;
        auto now=GetTickCount64();float dt=std::min(.05f,float(now-trunkTick)/1000);trunkTick=now;
        if(!validLease(trunk,trunkIndex)){trunk={};trunkRatio=0;return;}
        // Native pause/minimize freezes the animation too; never advance a paused game.
        if(s.menu||!active)return;
        DWORD matrix{},frame{},driver{};BYTE status{};gta::Vec pos{},speed{};float health{};
        if(!s.valid||!gta::read(trunk.address+0x68c,frame)||!frame
            ||!gta::read(trunk.address+0x5aa,status)||status>=3
            ||!gta::read(trunk.address+0x14,matrix)||!gta::read(matrix+0x30,pos)
            ||!gta::read(trunk.address+0x460,driver)||!gta::read(trunk.address+0x44,speed)
            ||!gta::read(trunk.address+0x4c0,health)||!std::isfinite(pos.x+pos.y+pos.z+speed.x+speed.y+speed.z+health)){
            trunk={};trunkRatio=0;return;
        }
        if(!connected||s.mission||s.vehicle||s.health<=0||driver||health<=250
            ||std::hypot(pos.x-s.position.x,pos.y-s.position.y)>5
            ||std::abs(pos.z-s.position.z)>3||std::hypot(speed.x,speed.y)>.02f)trunkWanted=false;
        float target=trunkWanted?1.f:0.f;
        trunkRatio=std::clamp(trunkRatio+(trunkWanted?1.f:-1.f)*dt*2,0.f,1.f);
        openDoor(reinterpret_cast<void*>(trunk.address),nullptr,17,1,trunkRatio,false);
        if(trunkRatio==target&&!trunkWanted){trunk={};++trunkCloses;}
    }
    bool cargoPoint(unsigned kind,gta::Vec& origin,float& yaw)const{
        if(kind<1||kind>8||!trunk.id||!trunkWanted||trunkRatio<.8f||GetTickCount64()-contentsStamp>1500
            ||!validLease(trunk,trunkIndex)||(kind==1?!(cargoContents&1):!(cargoContents&2)))return false;
        DWORD matrix{},info{},col{};short model{};gta::Vec pos{},right{},forward{},up{},lo{},hi{};
        if(!gta::read(trunk.address+0x14,matrix)||!gta::read(matrix,right)
            ||!gta::read(matrix+0x10,forward)||!gta::read(matrix+0x20,up)||!gta::read(matrix+0x30,pos)
            ||!gta::read(trunk.address+0x22,model)||model<0||model>=20000||!gta::read(models+model*4,info)
            ||!gta::read(info+0x14,col)||!gta::read(col,lo)||!gta::read(col+12,hi))return false;
        float x=kind==1?-.24f:.24f,y=lo.y+.65f,z=lo.z+(hi.z-lo.z)*.42f;
        if(kind>=3){unsigned slot=kind-3;x=(int(slot%3)-1)*.27f;y=lo.y+.45f+(slot/3)*.28f;}
        origin={pos.x+right.x*x+forward.x*y+up.x*z,pos.y+right.y*x+forward.y*y+up.y*z,pos.z+right.z*x+forward.z*y+up.z*z};
        yaw=std::atan2(-right.y,right.x)*180.f/3.14159265f;
        return std::isfinite(origin.x+origin.y+origin.z+yaw);
    }
    bool cargoMarker(gta::Vec& point)const{
        if(GetTickCount64()-cargoStamp>300)return false;
        for(const auto& [index,l]:leases)if(l.id==cargoCandidate&&validLease(l,index)){
            DWORD matrix{},info{},col{};short model{};gta::Vec pos{},forward{},lo{},hi{};
            if(!gta::read(l.address+0x14,matrix)||!gta::read(matrix+0x30,pos)||!gta::read(matrix+0x10,forward)
                ||!gta::read(l.address+0x22,model)||model<0||model>=20000||!gta::read(models+model*4,info)
                ||!gta::read(info+0x14,col)||!gta::read(col,lo)||!gta::read(col+12,hi))return false;
            float n=std::hypot(forward.x,forward.y);if(n<.5f)return false;
            point={pos.x+forward.x/n*(lo.y+.2f),pos.y+forward.y/n*(lo.y+.2f),pos.z+hi.z+.4f};return std::isfinite(point.x+point.y+point.z);
        }
        return false;
    }
    bool trip(const gta::Snapshot& s,int& kmh,int& condition,float* exactSpeed=nullptr)const{
        if(!s.valid||!s.vehicle||s.health<=0)return false;
        DWORD pool{},base{},flags{};int size{};
        if(!gta::read(poolGlobal,pool)||!gta::read(pool,base)||!gta::read(pool+4,flags)||!gta::read(pool+8,size)||size<1||size>1024)return false;
        for(int i=0;i<size;i++){BYTE gen{},kind{};DWORD driver{};gta::Vec velocity{};float health{};auto a=base+i*0xa18;
            if(!gta::read(flags+i,gen)||(gen&128)||!gta::read(a+0x36,kind)||(kind&7)!=2
                ||!gta::read(a+0x460,driver)||driver!=s.ped||!gta::read(a+0x44,velocity)
                ||!gta::read(a+0x4c0,health)||!std::isfinite(velocity.x+velocity.y+velocity.z+health))continue;
            float realSpeed=std::clamp(float(std::hypot(velocity.x,velocity.y)*180),0.f,500.f);
            if(exactSpeed)*exactSpeed=realSpeed;
            kmh=std::clamp(int(std::lround(realSpeed/5))*5,0,500);
            condition=std::clamp(int(std::lround(health/10)),0,100);return true;
        }return false;
    }
    static constexpr unsigned cargoReadyFlag=1u<<20;
    unsigned cargoCandidate{};ULONGLONG cargoStamp{};
    bool supported{};unsigned count{},damageHits{};
    bool init(){
        gta::OriginalImage image;if(!image.load())return false;
        auto pools=image.scan("55 8B EC 8B 4D 08 8B 15 ? ? ? ? 56 8B 72 04 8B C1 C1 F8 08 38 0C 30 5E 75 0A 69 C0 18 0A 00 00 03 02 5D C3 33 C0 5D C3");
        auto infos=image.scan("0F BF 41 22 8B 0C 85 ? ? ? ? 8B 51 14 D9 42 24 C3");
        auto hurt=image.scan("55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 70 53 8B 5D 0C 56 57 8B 7D 08 8D 45 F3 50 53 57 8B F1 C6 45 F3 00 E8 ? ? ? ? 84 C0");
        DWORD p{},m{};if(pools.size()!=1||infos.size()!=1||!gta::read(pools[0]+8,p)||!gta::read(infos[0]+7,m))return false;
        poolGlobal=image.global(p);models=image.global(m);
        if(hurt.size()==1)damage=reinterpret_cast<Damage>(image.code(hurt[0]));
        auto doors=image.scan("55 8B EC 64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 8B 45 0C 64 89 25 00 00 00 00 81 EC 9C 00 00 00 57 8B F9 8B 84 87 48 06 00 00 85 C0");
        if(doors.size()==1)openDoor=reinterpret_cast<Door>(image.code(doors[0]));
        supported=true;return true;
    }
    bool hit(const p::McEvent& e,const gta::Snapshot& s){
        if(!damage||!s.valid||s.menu||s.mission||s.health<=0||!std::isfinite(e.a)||e.a<=0)return false;
        unsigned id=(e.formId&0x00fffff0u)>>4;
        for(const auto& [index,l]:leases)if(l.id==id){
            DWORD pool{},base{},flags{},matrix{},driver{};BYTE gen{},kind{},created{};float before{};gta::Vec pos{};
            if(!gta::read(poolGlobal,pool)||!gta::read(pool,base)||!gta::read(pool+4,flags)
                ||base+index*0xa18!=l.address||!gta::read(flags+index,gen)||gen!=l.generation||(gen&128)
                ||!gta::read(l.address+0x36,kind)||(kind&7)!=2||!gta::read(l.address+0x4a4,created)||!gta::read(l.address+0x460,driver)
                ||(created==2&&(!s.vehicle||driver!=s.ped))
                ||!gta::read(l.address+0x4c0,before)||!std::isfinite(before)||before<=0
                ||!gta::read(l.address+0x14,matrix)||!gta::read(matrix+0x30,pos)
                ||!std::isfinite(pos.x+pos.y+pos.z)||std::hypot(pos.x-s.position.x,pos.y-s.position.y)>80)return false;
            int weapon=(e.flags&p::kHitProjectile)?22:(e.weapon==1?8:e.weapon==3?5:0);
            // Native damage/proofs/fire processing; never write the health field.
            auto creator=(e.flags&p::kHitByMob)?nullptr:reinterpret_cast<void*>(s.ped);
            damage(reinterpret_cast<void*>(l.address),creator,weapon,std::clamp(e.a*5.f,0.f,200.f),pos);
            float after{};if(gta::read(l.address+0x4c0,after)&&after<before)++damageHits;
            return true;
        }
        return false;
    }
    void append(std::vector<p::ActorRecord>& records,const gta::Snapshot& s,bool enabled){
        std::set<unsigned> seen;count=0;cargoCandidate=0;cargoStamp=GetTickCount64();float nearestCargo=1e9f;DWORD pool{},base{},flags{};int size{};
        // The published table is empty during pause, but the actual car pool has
        // not changed. Preserve checked IDs so a paused repair can resume.
        if(!enabled&&s.valid&&s.menu)return;
        if(enabled&&supported&&gta::read(poolGlobal,pool)&&gta::read(pool,base)&&gta::read(pool+4,flags)&&gta::read(pool+8,size)&&size>0&&size<=1024){
            for(int i=0;i<size&&records.size()+8<=p::kMaxActors;++i){
                BYTE gen{},kind{},created{};short model{};DWORD matrix{},driver{},info{},col{};float health{};gta::Vec pos{},forward{},lo{},hi{};
                auto address=base+i*0xa18;
                if(!gta::read(flags+i,gen)||(gen&128)||!gta::read(address+0x36,kind)||(kind&7)!=2
                    ||!gta::read(address+0x22,model)||model<0||model>=20000||!gta::read(models+model*4,info)||!gta::read(info+0x14,col)
                    ||!gta::read(col,lo)||!gta::read(col+12,hi)||!gta::read(address+0x14,matrix)||!gta::read(matrix+0x30,pos)
                    ||!gta::read(matrix+0x10,forward)||!gta::read(address+0x460,driver)||!gta::read(address+0x4a4,created)||!gta::read(address+0x4c0,health))continue;
                float width=hi.x-lo.x,length=hi.y-lo.y,height=hi.z-lo.z,n=std::hypot(forward.x,forward.y);
                if(!std::isfinite(pos.x+pos.y+pos.z+width+length+height+health+n)||n<.5f||width<.3f||width>8||length<.5f||length>16||height<.3f||height>8
                    ||std::hypot(pos.x-s.position.x,pos.y-s.position.y)>80||std::abs(pos.z-s.position.z)>24)continue;
                auto& lease=leases[i];if(!lease.id||lease.generation!=gen||lease.address!=address){if(next>=0xfffff)continue;lease={next++,gen,address};}seen.insert(i);
                unsigned pieces=std::clamp(unsigned(std::ceil(length/width)),1u,8u);float step=length/pieces;
                unsigned actorFlags=p::kActorVehicle;
                if(driver)actorFlags|=p::kActorOccupied;if(driver==s.ped)actorFlags|=p::kActorPlayerVehicle;
                if(created==2&&driver!=s.ped)actorFlags|=p::kActorMission;if(health<=0)actorFlags|=p::kActorDead;
                gta::Vec velocity{};
                float rearX=pos.x+forward.x/n*(lo.y-.4f),rearY=pos.y+forward.y/n*(lo.y-.4f);
                float rearDistance=std::hypot(rearX-s.position.x,rearY-s.position.y);
                if(gta::read(address+0x44,velocity)&&cargoAccessible(model,driver!=0,created==2,health,
                    std::sqrt(velocity.x*velocity.x+velocity.y*velocity.y+velocity.z*velocity.z),rearDistance,pos.z-s.position.z)){
                    actorFlags|=cargoReadyFlag;
                    if(rearDistance<nearestCargo){nearestCargo=rearDistance;cargoCandidate=lease.id;}
                }
                for(unsigned piece=0;piece<pieces;++piece){float offset=hi.y-step*(piece+.5f);p::ActorRecord r{};
                    r.formId=p::kActorVehicleTag|(lease.id<<4)|piece;r.flags=actorFlags;
                    r.x=pos.x+forward.x/n*offset;r.y=pos.z+lo.z;r.z=-pos.y-forward.y/n*offset;
                    r.width=std::max(width,step);r.height=height;r.healthFrac=std::clamp(health/1000,0.f,1.f);r.level=1;
                    r.yaw=std::atan2(-forward.x,-forward.y)*180.f/3.14159265f;
                    std::strcpy(r.name,"San Andreas vehicle");records.push_back(r);
                }++count;
            }
        }
        for(auto it=leases.begin();it!=leases.end();)if(!seen.count(it->first))it=leases.erase(it);else ++it;
    }
};
}
