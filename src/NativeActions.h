#pragma once
// Existing GTA animation, streaming and clothing APIs; resolved in the owned original PE.
namespace grove {
class NativeActions {
    using Blend=void* (__cdecl*)(void*,int,int,float);
    using BlendHier=void* (__cdecl*)(void*,void*,int,float);
    using Request=void (__cdecl*)(int,int);
    using Rebuild=void (__cdecl*)(void*);
    Blend blend{};BlendHier blendHier{};Request request{};Rebuild rebuild{};
    std::uintptr_t groups{},blocks{},blockCount{},hierarchies{},fatStat{},clumpDataOffset{};
    ULONGLONG nextAnimation{},nextWeight{},lastRebuild{},nextStream{},placeUntil{};
    void* ownAssociation{};void* lastClump{};
    DWORD ownHierarchy{};
    static unsigned hash(const char* s){unsigned h=0xffffffff;while(*s){auto c=*s++;if(c>='a'&&c<='z')c-=32;h^=static_cast<unsigned char>(c);for(int i=0;i<8;++i)h=(h>>1)^((h&1)?0xedb88320u:0);}return h;}
    bool attached(void* clump,void* association)const{
        int offset{};DWORD data{},link{};if(!clump||!association||!gta::read(clumpDataOffset,offset)||offset<0||offset>4096
            ||!gta::read(reinterpret_cast<std::uintptr_t>(clump)+offset,data)||!gta::read(data,link))return false;
        for(int n=0;link&&n<128;++n){if(link==reinterpret_cast<std::uintptr_t>(association)+4)return true;if(!gta::read(link,link))break;}return false;
    }
    void finish(void* clump){
        if(attached(clump,ownAssociation)){
            auto a=reinterpret_cast<std::uintptr_t>(ownAssociation);float delta=-8;unsigned short flags{};DWORD hierarchy{};
            if(ownHierarchy&&gta::read(a+0x14,hierarchy)&&hierarchy==ownHierarchy&&gta::read(a+0x2e,flags)&&(flags&16)
                &&gta::accessible(reinterpret_cast<void*>(a+0x1c),4,true)&&gta::accessible(reinterpret_cast<void*>(a+0x2e),2,true)){
                flags=(flags|4|8)&~2;std::memcpy(reinterpret_cast<void*>(a+0x1c),&delta,4);std::memcpy(reinterpret_cast<void*>(a+0x2e),&flags,2);
            }
        }ownAssociation=nullptr;ownHierarchy=0;
    }
    void* actionHierarchy(const char* block,const char* animation){
        int count{};if(!gta::read(blockCount,count)||count<1||count>180)return nullptr;
        for(int i=0;i<count;++i){auto b=blocks+i*32;char name[16]{};if(!gta::read(b,name)||name[15]!=0||_stricmp(name,block)!=0)continue;
            BYTE loaded{};int first{},total{};if(!gta::read(b+16,loaded)||!gta::read(b+20,first)||!gta::read(b+24,total))return nullptr;
            if(!loaded){auto now=GetTickCount64();if(request&&now>=nextStream){request(25575+i,8);nextStream=now+1000;}return nullptr;}
            if(first<0||total<1||first+total>2500)return nullptr;
            unsigned key=hash(animation);
            for(int n=0;n<total;++n){unsigned actual{};auto p=hierarchies+(first+n)*24;if(gta::read(p,actual)&&actual==key)return reinterpret_cast<void*>(p);}return nullptr;
        }return nullptr;
    }
public:
    void reset(){ownAssociation=nullptr;ownHierarchy=0;lastClump=nullptr;nextAnimation=placeUntil=0;}
    bool supported{},weightSupported{},foodLoaded{},repairLoaded{};unsigned animations{},repairAnimations{};float fat{-1};
    bool init(){
        gta::OriginalImage image;if(!image.load())return false;
        auto blends=image.scan("55 8B EC 83 EC 10 8B 4D 08 8B 15 ? ? ? ? 53 56 8B 75 0C 33 C0 8D 34 B6 57 8B 7D 10 89 45 F4 89 45 F8 89 45 F0 A1 ? ? ? ? 8B 1C 08");
        auto hi=image.scan("83 EC 18 A1 ? ? ? ? D9 45 14 8B 4D 08 D9 EE 8B 55 10 53 8B 1C 08 56 33 F6 C1 EA 04 57 8B 3B 80 E2 01");
        auto ab=image.scan("55 8B EC 53 56 33 F6 57 39 35 ? ? ? ? 7E 22 8B 5D 08 BF ? ? ? ? 53 57 E8 ? ? ? ? 83 C4 08 85 C0 74 13 46 83 C7 20");
        auto ah=image.scan("55 8B EC 8B 55 0C 8B 42 14 8B 52 18 8D 04 40 33 C9 56 8D 04 C5 ? ? ? ? 85 D2 7E 0F");
        auto rq=image.scan("55 8B EC 53 8B 5D 08 56 57 8D 3C 9B 03 FF 8A 84 3F ? ? ? ? 03 FF 3C 02 75 1E F6 45 0C 10");
        if(blends.size()==1&&hi.size()==1&&ab.size()==1&&ah.size()==1&&rq.size()==1){
            DWORD g{},c{},bs{},hs{},off{};gta::read(blends[0]+11,g);gta::read(blends[0]+39,off);gta::read(ab[0]+10,c);gta::read(ab[0]+20,bs);gta::read(ah[0]+21,hs);
            groups=image.global(g);clumpDataOffset=image.global(off);blocks=image.global(bs);blockCount=image.global(c);hierarchies=image.global(hs);
            blend=reinterpret_cast<Blend>(image.code(blends[0]));blendHier=reinterpret_cast<BlendHier>(image.code(hi[0]-24));request=reinterpret_cast<Request>(image.code(rq[0]));supported=true;
        }
        auto st=image.scan("55 8B EC 66 8B 45 08 66 83 F8 52 73 0C 0F B7 C0 D9 04 85 ? ? ? ? 5D C3");
        auto rb=image.scan("55 8B EC 83 EC 08 56 8B 75 08 8B 86 80 04 00 00 8B 48 04 D9 41 70 6A 15 D9 5D 08 D9 45 08 DD 5D F8 E8");
        if(st.size()==1&&rb.size()==1){DWORD stats{};gta::read(st[0]+19,stats);fatStat=image.global(stats)+21*4;rebuild=reinterpret_cast<Rebuild>(image.code(rb[0]));weightSupported=true;}
        return supported;
    }
    void tick(const gta::Snapshot& s,bool linked,bool mining,bool eating,bool place,float& weight,unsigned kind=0,bool attacking=false,bool heldUse=false,bool repairing=false){
        bool safe=linked&&s.valid&&!s.menu&&!s.vehicle&&!s.mission&&s.health>0&&s.state>0&&s.state<20;
        DWORD clump{};gta::read(s.ped+0x18,clump);void* current=reinterpret_cast<void*>(clump);
        // Death/reload can recycle the same clump/association addresses. Never
        // fade a pointer inherited from the old life: it may now be GTA's idle.
        if(!safe){if(s.valid&&s.health>0&&current==lastClump&&s.state>0&&s.state<20)finish(current);
            reset();if(!linked||!s.valid||s.health<=0)weight=0;return;}
        auto now=GetTickCount64();
        if(weightSupported&&now>=nextWeight&&std::isfinite(weight)){
            nextWeight=now+1000;float before{};
            if(gta::read(fatStat,before)&&std::isfinite(before)&&before>=0&&before<=1000&&gta::accessible(reinterpret_cast<void*>(fatStat),4,true)){
                float delta=std::clamp(weight,-.05f,.5f);fat=std::clamp(before+delta,0.0f,1000.0f);weight-=delta;
                std::memcpy(reinterpret_cast<void*>(fatStat),&fat,4);
            }
            // Rebuild only while standing still, and in coarse steps; use GTA's own clothing API.
            DWORD data{},desc{};float oldFat{};gta::Vec velocity{};
            if(fat>=0&&now-lastRebuild>30000&&s.grounded&&!mining&&!eating&&gta::read(s.ped+0x480,data)&&gta::read(data+4,desc)
                &&gta::read(desc+0x70,oldFat)&&std::isfinite(oldFat)&&std::abs(fat-oldFat)>=10
                &&gta::read(s.ped+0x44,velocity)&&std::abs(velocity.x)+std::abs(velocity.y)+std::abs(velocity.z)<.005f){
                finish(current);rebuild(reinterpret_cast<void*>(s.ped));lastRebuild=now;gta::read(s.ped+0x18,clump);current=reinterpret_cast<void*>(clump);
            }
        }
        if(!supported||!clump||!gta::accessible(current,0x30))return;
        if(current!=lastClump){ownAssociation=nullptr;lastClump=current;nextAnimation=0;}
        if(place)placeUntil=now+600;
        gta::Vec motion{};if(repairing&&(!gta::read(s.ped+0x44,motion)||std::hypot(motion.x,motion.y)>.005f))repairing=false;
        if(!mining&&!eating&&!place&&!attacking&&!heldUse&&!repairing){if(now<placeUntil)return;finish(current);nextAnimation=0;return;}
        if(now<nextAnimation&&!place)return;
        if(repairing){
            auto hierarchy=actionHierarchy("CAR","Fixn_Car_Loop");repairLoaded=hierarchy!=nullptr;if(!hierarchy)return;
            ownAssociation=blendHier(current,hierarchy,0x18,6.f);nextAnimation=now+900;if(ownAssociation)++repairAnimations;
        }else if(heldUse&&!mining&&!eating&&(kind==20||kind==21||kind==22)){
            // Existing partial upper-body poses; feet retain GTA locomotion.
            // Gun_stand reaches with both arms; FIGHTIDLE keeps the shield grip
            // at chest height rather than FightA_Block's overhead unarmed guard.
            auto hierarchy=kind==21?actionHierarchy("PED","FIGHTIDLE"):actionHierarchy("PED","Gun_stand");
            if(!hierarchy)return;
            ownAssociation=blendHier(current,hierarchy,0x18,8.0f);nextAnimation=now+1000;
        }else if(eating){auto hierarchy=actionHierarchy("FOOD","EAT_Burger");foodLoaded=hierarchy!=nullptr;if(!hierarchy)return;
            ownAssociation=blendHier(current,hierarchy,0x18,8.0f);nextAnimation=now+1500;
        }else if(attacking&&!mining&&kind!=15){
            auto hierarchy=actionHierarchy("PED","FightA_1");
            if(hierarchy){ownAssociation=blendHier(current,hierarchy,0x18,8.0f);nextAnimation=now+600;}
            else {ownAssociation=blend(current,0,164,8.0f);nextAnimation=now+450;}
        }else if((mining||attacking)&&(kind==10||kind==15||kind==6)){
            auto hierarchy=actionHierarchy("SWORD","sword_1");
            if(hierarchy){ownAssociation=blendHier(current,hierarchy,0x18,8.0f);nextAnimation=now+600;}
            else {ownAssociation=blend(current,0,164,8.0f);nextAnimation=now+450;}
        }else{ownAssociation=blend(current,0,164,8.0f);nextAnimation=now+450;}
        if(ownAssociation){++animations;unsigned short flags{};auto a=reinterpret_cast<std::uintptr_t>(ownAssociation);
            ownHierarchy=0;gta::read(a+0x14,ownHierarchy);
            if(gta::read(a+0x2e,flags)&&gta::accessible(reinterpret_cast<void*>(a+0x2e),2,true)){flags=(flags|8|16)&~2;if(heldUse||repairing)flags|=2;std::memcpy(reinterpret_cast<void*>(a+0x2e),&flags,2);}}
    }
};
}
