#pragma once
#include "StructureMapSnapshot.h"
namespace grove {
class NativeStructureRadar {
    static constexpr unsigned tag=0x47575354u;
    std::array<unsigned,64> handles{};
    std::array<StructureMapSnapshot::Point,64> shown{};
    ULONGLONG due{},stamp{};
    bool adopted{},lastMenu{},menuKnown{};
public:
    StructureMapSnapshot snapshot;
    unsigned visible{},created{},removed{},freeSlots{};
    NativeStructureRadar(){handles.fill(0xffffffffu);}
    void receive(const p::McEvent& e){if(snapshot.accept(e.formId,e.a,e.b,e.c,e.d,e.flags,e.weapon))stamp=GetTickCount64();}
    void tick(NativeDeliveryRadar& radar,const gta::Snapshot& s,bool connected){
        if(!radar.supported||!s.valid)return;
        // GTA hides short-range sprites in locked regions on its pause map, even
        // when CJ has already visited a generated structure there. Only our own
        // discovered landmarks ignore that filter while the map/menu is open.
        if(!menuKnown||s.menu!=lastMenu){
            for(auto h:handles)radar.shortRangeOwned(h,tag,!s.menu);
            lastMenu=s.menu;menuKnown=true;
        }
        // A paused server need not resend its unchanged landmarks. Keep their
        // lease frozen too, avoiding a blank radar on the first resumed frames.
        if(s.menu){if(stamp)stamp=GetTickCount64();return;}
        if(GetTickCount64()<due)return;
        due=GetTickCount64()+500;
        if(!adopted){radar.clearTag(tag);adopted=true;}
        radar.pruneTag(tag,handles);
        if(!connected){stamp=0;snapshot.staging=false;}
        bool fresh=connected&&stamp&&GetTickCount64()-stamp<10000;
        unsigned wanted=fresh?snapshot.count:0;visible=0;
        for(unsigned i=0;i<handles.size();++i){
            auto& h=handles[i];auto next=snapshot.points[i];
            bool same=shown[i].kind==next.kind&&shown[i].x==next.x&&shown[i].y==next.y&&shown[i].z==next.z;
            if(h!=0xffffffffu&&(!radar.owned(h,tag)||i>=wanted||!same)){
                if(radar.eraseOwned(h,tag))++removed;h=0xffffffffu;
            }
            if(i>=wanted)continue;
            if(h==0xffffffffu){
                // Reuse native textures. Short range keeps distant icons off the radar rim;
                // GTA itself still displays them on its full pause map.
                static constexpr BYTE sprites[]={0,31,53,51,52,17,37};
                h=radar.createOwned(tag,{next.x,next.y,next.z},sprites[next.kind]);
                if(h!=0xffffffffu){shown[i]=next;++created;}
            }
            if(h!=0xffffffffu)++visible;
        }
        freeSlots=radar.freeSlots();
    }
    void telemetry(FILE* f,const NativeDeliveryRadar& radar)const{
        std::fprintf(f,"{\"supported\":%s,\"published\":%u,\"visible\":%u,\"commits\":%u,\"created\":%u,\"removed\":%u,\"free_slots\":%u,\"markers\":[",
            radar.supported?"true":"false",snapshot.count,visible,snapshot.commits,created,removed,freeSlots);
        unsigned written=0;for(unsigned i=0;i<handles.size();++i){gta::Vec pos{};BYTE sprite{},flags{};
            if(!radar.inspectOwned(handles[i],tag,pos,sprite,flags))continue;
            std::fprintf(f,"%s{\"handle\":%u,\"kind\":%u,\"sprite\":%u,\"short_range\":%s,\"position\":[%.3f,%.3f,%.3f]}",
                written++?",":"",handles[i],shown[i].kind,unsigned(sprite),(flags&4)?"true":"false",pos.x,pos.y,pos.z);}
        std::fprintf(f,"]}\n");
    }
};
}
