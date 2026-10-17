#pragma once
// GroveCraft-owned collision adapter. Public SA layouts: Plugin-SDK (GTA Community).
// Native entry points/globals come from validated instructions, not 1.0 addresses.
#include <memory>
#include <array>
#include <map>
namespace grove {
struct NativeBox { gta::Vec min,max; unsigned char material{},flags{},lighting{0xff},light{}; };
struct NativeData { unsigned short spheres{},boxes{},triangles{}; unsigned char lines{},flags{2};
    void* spherePtr{}; NativeBox* boxPtr{}; void* linePtr{};void* vertices{};void* trianglePtr{};void* planes{};
    unsigned shadowTriangles{},shadowVertices{};void* shadowVerts{};void* shadowTris{}; };
struct NativeCol {gta::Vec min{-8,-8,-8},max{8,8,8},centre{};float radius{13.857f};unsigned char slot{},flags{1};unsigned short pad{};NativeData* data{};};
struct NativeModel {void** vt{};unsigned key{};unsigned short refs{};short txd{-1};unsigned char alpha{255},effects{};short effectIndex{-1},objectInfo{-1};unsigned short flags{};NativeCol* col{};float distance{};void* rw{};};
static_assert(sizeof(NativeBox)==0x1c&&sizeof(NativeData)==0x30&&sizeof(NativeCol)==0x30&&sizeof(NativeModel)==0x20);
using SectionBits=std::array<unsigned char,512>;
using SectionKey=std::array<int,3>;
inline std::vector<NativeBox> collisionBoxes(const SectionBits& bits){
    std::vector<NativeBox> boxes;
    auto occupied=[&](int x,int y,int z){unsigned bit=x+16*z+256*y;return (bits[bit/8]&(1u<<(bit%8)))!=0;};
    // Merge consecutive X cells; plants/water do not appear in the solid bitmap.
    for(int y=0;y<16;++y)for(int z=0;z<16;++z)for(int x=0;x<16;){
        if(!occupied(x,y,z)){++x;continue;}int start=x;while(x<16&&occupied(x,y,z))++x;
        NativeBox box;box.min={float(start-8),float(7-z),float(y-8)};box.max={float(x-8),float(8-z),float(y-7)};boxes.push_back(box);
    }
    return boxes;
}
class NativeWorld {
    using Ctor=void* (__thiscall*)(void*);using Alloc=void* (__cdecl*)(unsigned);
    using Method=void (__thiscall*)(void*);using Delete=void (__thiscall*)(void*,unsigned);
    struct Node {NativeModel model;NativeCol col;NativeData data;std::vector<NativeBox> boxes;SectionBits bits{};std::vector<NativeBox> caps;void* entity{};unsigned char generation{};int id{-1};};
    std::map<SectionKey,std::unique_ptr<Node>> nodes;
    // GTA can retain a building contact for a frame after teleport/respawn.
    // A retired private model must stay valid (with no collision), never null.
    struct Retired {NativeModel model;NativeCol col;ULONGLONG stamp{};};
    std::map<int,Retired> retired;
    std::uintptr_t models{},poolGlobal{};Ctor ctor{};Alloc alloc{};void** buildingVt{};
    ULONGLONG nextSync{};bool worldReady{};ULONGLONG readyAfter{};
    bool live(const Node& n)const{
        if(!n.entity)return false;DWORD pool{},base{},flags{};int size{};
        if(!gta::read(poolGlobal,pool)||!gta::read(pool,base)||!gta::read(pool+4,flags)||!gta::read(pool+8,size)||size<=0||size>100000)return false;
        auto ptr=reinterpret_cast<std::uintptr_t>(n.entity);if(ptr<base||(ptr-base)%0x38)return false;auto index=(ptr-base)/0x38;if(index>=static_cast<unsigned>(size))return false;
        BYTE flag{};short id{};void** vt{};
        return gta::read(flags+index,flag)&&!(flag&0x80)&&flag==n.generation&&gta::read(ptr+0x22,id)&&id==n.id&&gta::read(ptr,vt)&&vt==buildingVt;
    }
    void discard(Node& n){
        if(live(n)){reinterpret_cast<Method>(buildingVt[3])(n.entity);reinterpret_cast<Delete>(buildingVt[0])(n.entity,1);}
        n.entity=nullptr;void* current{};
        if(n.id>=0&&gta::read(models+n.id*4,current)&&current==&n.model&&gta::accessible(reinterpret_cast<void*>(models+n.id*4),4,true)){
            auto& r=retired[n.id];r.model=n.model;r.col=NativeCol{};r.col.flags=0;r.col.radius=0;
            r.model.col=&r.col;r.model.refs=0;r.stamp=GetTickCount64();
            void* empty=&r.model;std::memcpy(reinterpret_cast<void*>(models+n.id*4),&empty,4);
        }n.id=-1;
    }
    bool spawn(const SectionKey& key,Node& n){
        if(n.boxes.empty())return false;
        // Only occupy a genuinely unused model slot; never modify a stock model.
        int id=-1;for(int i=19999;i>=19000;--i){void* p{};auto old=retired.find(i);
            if(gta::read(models+i*4,p)&&(!p||(old!=retired.end()&&p==&old->second.model&&GetTickCount64()-old->second.stamp>=2000))){id=i;break;}}
        if(id<0)return false;
        void* source{};if(!gta::read(models+1337*4,source)||!gta::accessible(source,0x20))return false;
        std::memcpy(&n.model,source,sizeof n.model);n.model.key=0x47525643;n.model.refs=0;n.model.txd=-1;n.model.effects=0;n.model.effectIndex=-1;n.model.objectInfo=-1;n.model.flags=0;n.model.distance=0;n.model.rw=nullptr;
        n.data.boxes=static_cast<unsigned short>(n.boxes.size());n.data.boxPtr=n.boxes.data();n.col.data=&n.data;n.model.col=&n.col;
        if(!gta::accessible(n.model.vt,16*4)||!gta::accessible(reinterpret_cast<void*>(models+id*4),4,true))return false;
        auto entity=alloc(0x38);if(!entity)return false;ctor(entity);n.entity=entity;n.id=id;
        void* info=&n.model;std::memcpy(reinterpret_cast<void*>(models+id*4),&info,4);
        auto p=reinterpret_cast<unsigned char*>(entity);short model=static_cast<short>(id);std::memcpy(p+0x22,&model,2);
        gta::Vec position{float(key[0]*16+8),float(-key[2]*16-8),float(key[1]*16+8)};std::memcpy(p+4,&position,sizeof position);
        // Invisible collision-only building, excluded from streaming and deletion by streaming.
        DWORD flags=1u|4u|(1u<<10)|(1u<<19);std::memcpy(p+0x1c,&flags,4);p[0x2f]=0;
        DWORD pool{},base{},entries{};gta::read(poolGlobal,pool);gta::read(pool,base);gta::read(pool+4,entries);gta::read(entries+(reinterpret_cast<std::uintptr_t>(entity)-base)/0x38,n.generation);
        reinterpret_cast<Method>(buildingVt[2])(entity);return true;
    }
public:
    bool supported{};unsigned failures{};std::size_t boxCount{};
    bool init(){
        gta::OriginalImage image;if(!image.load())return false;
        auto constructors=image.scan("56 8B F1 E8 ? ? ? ? 8A 46 36 83 4E 1C 01 24 F9 0C 01 88 46 36 C7 06 ? ? ? ? 8B C6 5E C3");
        auto pools=image.scan("8B 0D ? ? ? ? E9 ? ? ? ? CC CC CC CC CC 55 8B EC 8B 4D 08 56 8B 35 ? ? ? ? 2B 0E B8 93 24 49 92");
        auto infos=image.scan("0F BF 41 22 8B 0C 85 ? ? ? ? 8B 51 14 D9 42 24 C3");
        if(constructors.size()!=1||infos.size()!=1)return false;
        // Building and Dummy pools share stride 0x38. Only the pool factory adjacent
        // to the independently identified Building constructor belongs to Building.
        auto constructorAddress=image.code(constructors[0]);
        pools.erase(std::remove_if(pools.begin(),pools.end(),[&](auto p){auto code=image.code(p);return code<constructorAddress||code-constructorAddress>0x1000;}),pools.end());
        if(pools.size()!=1)return false;
        DWORD vt{},pool{},pool2{},table{};if(!gta::read(constructors[0]+24,vt)||!gta::read(pools[0]+2,pool)||!gta::read(pools[0]+25,pool2)||pool!=pool2||!gta::read(infos[0]+7,table))return false;
        vt=image.global(vt);pool=image.global(pool);table=image.global(table);
        buildingVt=reinterpret_cast<void**>(vt);if(!gta::accessible(buildingVt,22*4)||!gta::accessible(reinterpret_cast<void*>(table),4))return false;
        auto constructor=image.code(constructors[0]),allocator=image.code(pools[0]);
        BYTE first{};if(!gta::read(constructor,first)||first!=0x56||!gta::read(allocator,first)||first!=0x8b)return false;
        ctor=reinterpret_cast<Ctor>(constructor);alloc=reinterpret_cast<Alloc>(allocator);poolGlobal=pool;models=table;supported=true;return true;
    }
    std::size_t entities()const{return nodes.size();}
    bool ready()const{return worldReady;}
    bool hasSection(const SectionKey& key)const{auto i=nodes.find(key);return i!=nodes.end()&&live(*i->second);}
    bool supports(gta::Vec origin)const{
        for(const auto& entry:nodes){if(!live(*entry.second))continue;const auto& k=entry.first;
            gta::Vec c{float(k[0]*16+8),float(-k[2]*16-8),float(k[1]*16+8)};
            for(const auto& b:entry.second->boxes)if(origin.x>=c.x+b.min.x&&origin.x<=c.x+b.max.x
                &&origin.y>=c.y+b.min.y&&origin.y<=c.y+b.max.y&&std::abs(origin.z-1-c.z-b.max.z)<.16)return true;
        }return false;
    }
    void clear(){for(auto& pair:nodes)discard(*pair.second);nodes.clear();boxCount=0;}
    void sync(const std::map<SectionKey,SectionBits>& solids,const gta::Snapshot& player,bool connected,const std::map<SectionKey,std::vector<NativeBox>>& caps={}){
        if(!supported)return;
        if(!connected||!player.valid){clear();worldReady=false;readyAfter=0;return;}if(player.menu)return;
        // New Game publishes a temporary CJ before its world/model initialization ends.
        // Wait for the first free-roam frame; subsequent missions keep their collisions.
        if(!worldReady){if(player.mission){readyAfter=0;return;}auto t=GetTickCount64();if(!readyAfter){readyAfter=t+3000;return;}if(t<readyAfter)return;worldReady=true;}
        auto now=GetTickCount64();if(now<nextSync)return;nextSync=now+100;
        auto sections=solids;for(const auto& c:caps)sections.try_emplace(c.first);
        std::vector<std::pair<double,SectionKey>> nearby;
        for(const auto& pair:sections){auto k=pair.first;double dx=k[0]*16+8-player.position.x,dy=-k[2]*16-8-player.position.y,dz=k[1]*16+8-player.position.z;
            if(dx*dx+dy*dy<=96*96&&std::abs(dz)<=48&&(caps.count(k)||std::any_of(pair.second.begin(),pair.second.end(),[](auto a){return a!=0;})))nearby.push_back({dx*dx+dy*dy+dz*dz,k});}
        std::sort(nearby.begin(),nearby.end());if(nearby.size()>128)nearby.resize(128);
        std::set<SectionKey> wanted;for(const auto& p:nearby)wanted.insert(p.second);
        for(auto it=nodes.begin();it!=nodes.end();){if(!wanted.count(it->first)||!live(*it->second)){discard(*it->second);it=nodes.erase(it);}else ++it;}
        unsigned changes=0;for(const auto& entry:nearby){auto key=entry.second;auto it=nodes.find(key);const auto& bits=sections.at(key);
            const std::vector<NativeBox> empty;auto c=caps.find(key);const auto& extra=c==caps.end()?empty:c->second;
            if(it!=nodes.end()&&it->second->bits==bits&&it->second->caps.size()==extra.size()&&
                (extra.empty()||!std::memcmp(it->second->caps.data(),extra.data(),extra.size()*sizeof(NativeBox))))continue;
            if(changes++>=8)break;
            auto n=std::make_unique<Node>();n->bits=bits;n->caps=extra;n->boxes=collisionBoxes(bits);n->boxes.insert(n->boxes.end(),extra.begin(),extra.end());
            if(n->boxes.size()>65535){++failures;continue;}
            if(spawn(key,*n)){
                if(it!=nodes.end()){discard(*it->second);nodes.erase(it);}
                nodes.emplace(key,std::move(n));
            }else ++failures; // Keep the previous collider when allocation fails.
        }
        boxCount=0;for(const auto& pair:nodes)boxCount+=pair.second->boxes.size();
    }
};
}
