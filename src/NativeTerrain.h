#pragma once
#include <atomic>
#include <deque>
#include "../vendor/libertycraft/asi/src/collision/Geometry.h"
#include "SATerrain.h"
#include "NativeVegetation.h"
#include "NativeGroundProbe.h"
namespace grove {
// Reuse LibertyCraft's tested half-metre surface sampler and 1/8-block voxelizer.
// Only the GTA main thread queries CWorld; immutable samples are voxelized on a worker.
class NativeTerrain {
    // NewSteam packs the six surface bytes consecutively (B at 0x23).
    // Owned PE's contiguous setters 4104b0..410510 prove this; 1.0 SDK pads B to 0x24.
    struct ColPoint {gta::Vec point;float pad;gta::Vec normal;float pad2;BYTE a,pieceA,lightA,b,pieceB,lightB,padA,padB;float depth;};
    static_assert(sizeof(ColPoint)==0x2c&&offsetof(ColPoint,b)==0x23);
    using Line=bool (__cdecl*)(const gta::Vec&,const gta::Vec&,ColPoint&,void*&,bool,bool,bool,bool,bool,bool,bool,bool);
    Line line{};std::uintptr_t models{};
    struct Job {unsigned epoch{};std::shared_ptr<const lc::col::Column> col;std::array<int,2> key{};std::vector<lc::col::Tri> exact;};
    std::vector<lc::col::Tri> exactWalls;
    std::set<std::uintptr_t> wallSources;
    gta::Vec wallAnchor{};bool wallsAnchored{};
    void rememberWalls(void* entity){
        auto ptr=reinterpret_cast<std::uintptr_t>(entity);
        if(!entity||wallSources.count(ptr)||wallSources.size()>=128||exactWalls.size()>=16000)return;
        short model{};NativeCol col{};const NativeCol* identity{};NativeData data{};
        if(!nativeExcavation.model(entity,model,col,identity)||treeDigMaterial(model)||!col.data
            ||!gta::read(reinterpret_cast<std::uintptr_t>(col.data),data)||!data.triangles||data.triangles>4096)return;
        struct Triangle{unsigned short a,b,c;BYTE material,light;};struct Vertex{short x,y,z;};
        static_assert(sizeof(Triangle)==8&&sizeof(Vertex)==6);
        if(!gta::accessible(data.trianglePtr,data.triangles*sizeof(Triangle)))return;
        std::vector<Triangle> triangles(data.triangles);std::memcpy(triangles.data(),data.trianglePtr,triangles.size()*sizeof(Triangle));
        unsigned count=0;for(auto t:triangles)count=std::max(count,unsigned(std::max({t.a,t.b,t.c}))+1);
        if(!count||count>65536||!gta::accessible(data.vertices,count*sizeof(Vertex)))return;
        std::vector<Vertex> vertices(count);std::memcpy(vertices.data(),data.vertices,count*sizeof(Vertex));
        NativeExcavation::Matrix matrix{};DWORD address{};
        if(!(gta::read(ptr+0x14,address)&&address&&gta::read(address,matrix))){
            float heading{};if(!gta::read(ptr+0x10,heading)||!gta::read(ptr+4,matrix.pos)||!std::isfinite(heading))return;
            matrix.right={std::cos(heading),std::sin(heading),0};matrix.forward={-std::sin(heading),std::cos(heading),0};matrix.up.z=1;
        }
        auto begin=exactWalls.size();
        for(auto t:triangles){
            float v[9];unsigned i=0;
            for(auto index:{t.a,t.b,t.c}){auto a=vertices[index];
                auto p=NativeExcavation::transform(matrix,{a.x/128.f,a.y/128.f,a.z/128.f});
                v[i++]=p.x;v[i++]=p.z;v[i++]=-p.y;}
            lc::col::Tri wall{};if(exactWalls.size()<16000&&exactFacade(v,t.material,wall))exactWalls.push_back(wall);
        }
        wallSources.insert(ptr);
        // A ready sampled column may contain a gap below an overhang. Rebuild
        // it once with the actual COL faces; no live model pointer enters a job.
        float lowX=INFINITY,lowZ=INFINITY,highX=-INFINITY,highZ=-INFINITY;
        for(auto i=begin;i<exactWalls.size();i++){const auto& t=exactWalls[i];for(int v=0;v<3;v++){
            lowX=std::min(lowX,t.v[v*3]);highX=std::max(highX,t.v[v*3]);lowZ=std::min(lowZ,t.v[v*3+2]);highZ=std::max(highZ,t.v[v*3+2]);}}
        for(auto it=cache.begin();it!=cache.end();){
            bool touched=highX>=it->first[0]*8&&lowX<it->first[0]*8+8&&highZ>=it->first[1]*8&&lowZ<it->first[1]*8+8;
            if(touched)it=cache.erase(it);else ++it;
        }
    }
    struct Cached {ULONGLONG stamp{};float low{},high{};bool coarse{};};
    unsigned surveyPass{};
    std::map<std::array<int,2>,Cached> cache;
    std::unique_ptr<lc::col::ColumnProbe> probe;
    std::unique_ptr<lc::col::ColumnProbe> interactionProbe;
    std::array<int,3> interactionCell{};
    ULONGLONG interactionUntil{};
    std::unique_ptr<GroundProbe> groundProbe;
    std::map<std::array<int,2>,ULONGLONG> groundRetry;
    std::map<std::array<int,2>,ULONGLONG> parcelPriority;
    unsigned framePass{},groundChoices{};
    std::mutex mutex;std::deque<Job> jobs;
    unsigned char* mapping{};std::atomic<unsigned> currentEpoch{1};unsigned sentEpoch{};
    std::atomic<bool> digging{false};
    gta::Vec previous{};bool active{};ULONGLONG nextChoice{};
    bool enqueue(Job job){
        if(job.col)for(const auto& t:exactWalls){
            if(std::max({t.v[0],t.v[3],t.v[6]})<job.key[0]*8||std::min({t.v[0],t.v[3],t.v[6]})>job.key[0]*8+8
                ||std::max({t.v[2],t.v[5],t.v[8]})<job.key[1]*8||std::min({t.v[2],t.v[5],t.v[8]})>job.key[1]*8+8)continue;
            job.exact.push_back(t);
        }
        std::lock_guard<std::mutex> lock(mutex);if(jobs.size()>=4)return false;jobs.push_back(std::move(job));return true;
    }
    bool write(unsigned type,const void* payload,std::size_t size,unsigned epoch){
        auto* ring=mapping+p::kOffCollisionRing;constexpr auto capacity=p::kColRingDataBytes;
        const auto bytes=(8ull+size+7)&~7ull;
        while(currentEpoch==epoch){
            auto head=load64(ring),tail=load64(ring+p::kColRingTailOff),offset=head%capacity,remain=capacity-offset;
            auto pad=remain<bytes?remain:0;
            if(head<tail||head-tail>capacity)return false;
            if(head-tail+pad+bytes>capacity){Sleep(5);continue;}
            if(pad){if(remain>=8){p::ColMsgHeader h{};std::memcpy(ring+p::kColRingDataOff+offset,&h,sizeof h);}head+=pad;offset=0;}
            p::ColMsgHeader h{type,static_cast<unsigned>(size)};
            std::memcpy(ring+p::kColRingDataOff+offset,&h,sizeof h);
            if(size)std::memcpy(ring+p::kColRingDataOff+offset+8,payload,size);
            MemoryBarrier();store64(ring,head+bytes);return true;
        }return false;
    }
    template<class T> bool packet(unsigned type,p::ColRegion h,const std::vector<T>& values,unsigned epoch){
        h.count=static_cast<unsigned>(values.size());std::vector<unsigned char> bytes(sizeof h+sizeof(T)*values.size());
        std::memcpy(bytes.data(),&h,sizeof h);if(!values.empty())std::memcpy(bytes.data()+sizeof h,values.data(),values.size()*sizeof(T));
        return write(type,bytes.data(),bytes.size(),epoch);
    }
    static DWORD WINAPI workerEntry(void* context){static_cast<NativeTerrain*>(context)->worker();return 0;}
    void worker(){
        for(;;){
            Job job;{std::lock_guard<std::mutex> lock(mutex);if(!jobs.empty()){job=std::move(jobs.front());jobs.pop_front();}}
            if(!job.epoch){Sleep(10);continue;}if(job.epoch!=currentEpoch)continue;
            if(sentEpoch!=job.epoch){if(!write(p::kColClear,&job.epoch,sizeof job.epoch,job.epoch))continue;sentEpoch=job.epoch;}
            if(!job.col){p::ColRegion h{job.key[0]*8,0,job.key[1]*8,job.key[0]*8+7,0,job.key[1]*8+7,job.epoch,0};write(p::kColForget,&h,sizeof h,job.epoch);continue;}
            auto sanitized=*job.col;sanitizeTerrainColumn(sanitized);
            std::vector<lc::col::Tri> all,region;lc::col::BuildColumn(sanitized,all);
            all.insert(all.end(),job.exact.begin(),job.exact.end());
            if(all.size()>20000){++rejected;continue;}
            // SA rays flip a two-sided terrain sheet toward the ray, so the
            // sampler sees a coincident floor/ceiling and zero slab thickness.
            // Treat the lowest earth/road floor in each half-metre column as
            // land. Higher bridge/roof sheets retain their hollow geometry.
            std::map<std::array<int,2>,float> ground;
            auto tile=[](const lc::col::Tri& t){return std::array<int,2>{int(std::floor((t.v[0]+t.v[3]+t.v[6])*2/3)),int(std::floor((t.v[2]+t.v[5]+t.v[8])*2/3))};};
            for(const auto& t:all)if(t.kind==lc::col::kFloor){float y=(t.v[1]+t.v[4]+t.v[7])/3;auto k=tile(t);auto it=ground.find(k);if(it==ground.end()||y<it->second)ground[k]=y;}
            for(auto& t:all)if(t.kind==lc::col::kFloor){unsigned m=(t.flags>>p::kTriGtaMaterialShift)&255;
                unsigned dig=m==37?p::kDigGrass:m>=128&&m<128+p::kDigMaterialCount?m-128:0;
                bool earth=dig==p::kDigGrass||dig==p::kDigDirt||dig==p::kDigStone||dig==p::kDigCobble||dig==p::kDigSand||dig==p::kDigGravel||dig==p::kDigMud;
                if(earth&&(t.v[1]+t.v[4]+t.v[7])/3<=ground[tile(t)]+.75f){t.flags|=p::kTriTerrain;t.fill=std::max(t.fill,2.f);}
            }
            auto faceKey=[](const lc::col::Tri& t){std::array<std::array<int,3>,3> points;
                for(int i=0;i<3;++i)for(int a=0;a<3;++a)points[i][a]=int(std::lround(t.v[i*3+a]*512));
                std::sort(points.begin(),points.end());std::array<int,9> key{};for(int i=0;i<3;++i)for(int a=0;a<3;++a)key[i*3+a]=points[i][a];return key;};
            std::set<std::array<int,9>> landFaces;for(const auto& t:all)if(t.kind==lc::col::kFloor&&(t.flags&p::kTriTerrain))landFaces.insert(faceKey(t));
            // A flipped ray normal is not a real roof. Keeping its duplicate
            // downward face makes empty space above the ground look solid.
            std::map<std::array<int,2>,float> landHeights;
            for(const auto& t:all)if(t.kind==lc::col::kFloor&&(t.flags&p::kTriTerrain)){
                float y=(t.v[1]+t.v[4]+t.v[7])/3;auto k=tile(t);
                auto found=landHeights.find(k);if(found==landHeights.end()||y<found->second)landHeights[k]=y;
            }
            all.erase(std::remove_if(all.begin(),all.end(),[&](const auto& t){
                if(t.kind!=lc::col::kCeiling)return false;
                if(landFaces.count(faceKey(t)))return true;
                // The reverse ray can split the same quad along the other
                // diagonal. Exact triangle keys then differ despite describing
                // the same sheet. Match the half-metre land tile and height too.
                auto found=landHeights.find(tile(t));
                return found!=landHeights.end()&&std::abs((t.v[1]+t.v[4]+t.v[7])/3-found->second)<.05f;
            }),all.end());
            int low=int(std::floor((job.col->yLo+2)/8)),high=int(std::floor((job.col->yHi-2)/8))-1;
            for(int y=low;y<=high&&job.epoch==currentEpoch;++y){
                lc::col::RegionTris(all,job.col->rx,y,job.col->rz,region);
                std::vector<p::ColTri> triangles;triangles.reserve(region.size());
                for(const auto& t:region){p::ColTri v{};std::memcpy(v.v,t.v,sizeof v.v);
                    v.flags=t.flags&~p::kTriDiggable;
                    unsigned material=(t.flags>>p::kTriGtaMaterialShift)&255;
                    unsigned dig=material==37?p::kDigGrass:material>=128&&material<128+p::kDigMaterialCount?material-128:0;
                    if(!dig)v.flags&=~p::kTriTerrain;
                    if(material!=37)v.flags&=~(p::kTriGtaMaterial|(255u<<p::kTriGtaMaterialShift));
                    if(dig&&digging.load()){
                        v.flags=(v.flags&~(255u<<p::kTriMaterialShift))|p::kTriDiggable|(dig<<p::kTriMaterialShift);
                        // Only thick ground behaves as a height field. Walls,
                        // bridges and thin slabs retain their sampled geometry.
                    }
                    triangles.push_back(v);}
                std::vector<p::ColBlock> cells;lc::col::Voxelize(region,job.col->rx,y,job.col->rz,cells);
                p::ColRegion h{job.col->rx*8,y*8,job.col->rz*8,job.col->rx*8+7,y*8+7,job.col->rz*8+7,job.epoch,0};
                if(!packet(p::kColTris,h,triangles,job.epoch)||!packet(p::kColRegion,h,cells,job.epoch))break;
                ++regions;
            }
        }
    }
    bool cast(const float* from,const float* to,lc::col::Hit& hit){
        gta::Vec a{from[0],-from[2],from[1]},b{to[0],-to[2],to[1]};
        float dx=b.x-a.x,dy=b.y-a.y,dz=b.z-a.z,len=std::sqrt(dx*dx+dy*dy+dz*dz);
        if(len<.0001f)return false;
        for(unsigned skip=0;skip<16;++skip){
            ColPoint pnt{};void* entity{};++rays;
            bool oldSampling=nativeExcavation.sampling;nativeExcavation.sampling=true;
            bool found=line(a,b,pnt,entity,true,false,false,true,true,false,false,false);
            nativeExcavation.sampling=oldSampling;if(!found)return false;
            auto p=pnt.point,n=pnt.normal;
            if(!std::isfinite(p.x+p.y+p.z+n.x+n.y+n.z)){++rejected;return false;}
            float along=(p.x-a.x)*dx/len+(p.y-a.y)*dy/len+(p.z-a.z)*dz/len;
            if(along<-.03f||along>std::sqrt((b.x-a.x)*(b.x-a.x)+(b.y-a.y)*(b.y-a.y)+(b.z-a.z)*(b.z-a.z))+.03f){++rejected;return false;}
            short model{};DWORD info{},key{};bool own=entity&&gta::read(reinterpret_cast<std::uintptr_t>(entity)+0x22,model)
                &&model>=19000&&model<20000&&gta::read(models+model*4,info)&&gta::read(info+4,key)&&key==0x47525643;
            BYTE entityKind{};if(entity)gta::read(reinterpret_cast<std::uintptr_t>(entity)+0x36,entityKind);
            if(own||(entityKind&7)==2||(entityKind&7)==3){a={p.x+dx/len*.06f,p.y+dy/len*.06f,p.z+dz/len*.06f};continue;}
            hit.pos[0]=p.x;hit.pos[1]=p.z;hit.pos[2]=-p.y;
            hit.n[0]=n.x;hit.n[1]=n.z;hit.n[2]=-n.y;
            // Normalize SA grass surfaces to the protocol's documented GRASS class.
            // Other SA IDs are not passed off as GTA IV material IDs.
            BYTE kind{};bool staticBuilding=entity&&gta::read(reinterpret_cast<std::uintptr_t>(entity)+0x36,kind)&&(kind&7)==1;
            if(!staticBuilding){hit.mat=lc::col::kNoMaterial;++hits;return true;}
            unsigned m=pnt.b;bool grass=(m>=9&&m<=17)||(m>=80&&m<=82)||(m>=115&&m<=122)||m==125||(m>=146&&m<=153)||m==160;
            unsigned treeWood=treeDigMaterial(model);
            unsigned dig=landscapeMaterial(m);
            if((m>=19&&m<=27)||m==83||m==84||m==87||m==88)dig=p::kDigDirt;
            else if((m>=28&&m<=33)||(m>=74&&m<=79))dig=p::kDigSand;
            else if(m==6||m==101)dig=p::kDigGravel;
            else if(m==18||(m>=35&&m<=38)||m<=8||m==34||m==89||m==69)dig=p::kDigStone;
            else if((m>=42&&m<=44)||m==70||m==72||m==73||m==90||m==94||m==95)dig=p::kDigPlanks;
            else if(m>=45&&m<=47)dig=p::kDigGlass;
            else if(m>=50&&m<=59)dig=p::kDigMetal;
            hit.mat=treeWood?BYTE(128+treeWood):grass?37:dig?BYTE(128+dig):lc::col::kNoMaterial;++hits;return true;
        }++rejected;return false;
    }
public:
    unsigned targetReason{},targetType{};short targetModel{};
    gta::Vec targetFrom{},targetTo{},targetPoint{};
    // The reticle supplies a real CWorld hit, never an invented mining plane.
    // Keep background work resumable while this nearby column gets the budget.
    void requestSurface(int x,int y,int z){
        interactionCell={x,y,z};interactionUntil=GetTickCount64()+300;
    }
    void requestParcel(float x,float z,int radius){
        if(!std::isfinite(x+z)||radius<1||radius>13)return;auto expiry=GetTickCount64()+4000;
        for(int rx=int(std::floor((x-radius)/8));rx<=int(std::floor((x+radius)/8));rx++)
            for(int rz=int(std::floor((z-radius)/8));rz<=int(std::floor((z+radius)/8));rz++)parcelPriority[{rx,rz}]=expiry;
        while(parcelPriority.size()>128)parcelPriority.erase(parcelPriority.begin());
    }
    bool supported{};std::atomic<unsigned> regions{},rejected{};unsigned rays{},hits{};
    unsigned epoch()const{return currentEpoch;}
    void invalidate(){++currentEpoch;cache.clear();exactWalls.clear();wallSources.clear();probe.reset();interactionProbe.reset();interactionUntil=0;groundProbe.reset();groundRetry.clear();nextChoice=0;}
    std::size_t columns()const{return cache.size();}
    std::size_t exactFaces()const{return exactWalls.size();}
    bool target(gta::Vec from,gta::Vec to,gta::Vec& pos,gta::Vec& normal,unsigned* wood=nullptr,bool* boundary=nullptr){
        targetReason=1;targetType=0;targetModel=-1;targetFrom=from;targetTo=to;targetPoint={};
        if(wood)*wood=0;if(boundary)*boundary=false;
        if(!line||!nativeExcavation.supported)return false;ColPoint point{};void* entity{};
        targetReason=2;if(!line(from,to,point,entity,true,false,false,true,false,false,false,false))return false;
        targetPoint=point.point;targetReason=3;
        short model{};bool privateModel=entity&&gta::read(reinterpret_cast<std::uintptr_t>(entity)+0x22,model)&&model>=19000&&model<20000;
        targetModel=model;
        if(privateModel){
            // Excavation lining is a private native collider with an air block
            // behind it in Minecraft. Keep its real hit for ceiling/side mining;
            // the server still checks adjacency, original material and reach.
            if(!boundary||!std::isfinite(point.point.x+point.point.y+point.point.z+point.normal.x+point.normal.y+point.normal.z)
                ||!nativeExcavation.terrainBoundary(int(std::floor(point.point.x-point.normal.x*.02f)),
                    int(std::floor(point.point.z-point.normal.z*.02f)),int(std::floor(-point.point.y+point.normal.y*.02f))))return false;
            *boundary=true;
        }
        targetReason=4;BYTE kind{};if(!entity||!gta::read(reinterpret_cast<std::uintptr_t>(entity)+0x36,kind))return false;
        targetType=kind&7;if(targetType!=1)return false;
        targetReason=5;if(!std::isfinite(point.point.x+point.point.y+point.point.z))return false;
        if(!privateModel)rememberWalls(entity);
        if(wood)*wood=treeDigMaterial(model);
        pos=point.point;normal=point.normal;targetReason=6;return true;
    }
    bool aim(gta::Vec from,gta::Vec to,const gta::Snapshot& player,gta::Vec& pos){
        if(!line)return false;gta::Vec delta{to.x-from.x,to.y-from.y,to.z-from.z};
        float length=std::sqrt(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z);if(length<.01f)return false;
        for(unsigned skip=0;skip<8;++skip){ColPoint point{};void* entity{};
            if(!line(from,to,point,entity,true,true,true,true,true,false,false,false))return false;
            if(!std::isfinite(point.point.x+point.point.y+point.point.z))return false;
            if(reinterpret_cast<std::uintptr_t>(entity)!=player.ped){
                // Continue just inside the visible surface. Projectile direction follows the
                // reticle, including vehicles and peds, rather than an unrelated terrain cell.
                pos={point.point.x+delta.x/length*.1f,point.point.y+delta.y/length*.1f,point.point.z+delta.z/length*.1f};return true;
            }
            from={point.point.x+delta.x/length*.15f,point.point.y+delta.y/length*.15f,point.point.z+delta.z/length*.15f};
        }return false;
    }
    bool visible(gta::Vec from,gta::Vec to){
        if(!line)return false;ColPoint point{};void* entity{};
        return !line(from,to,point,entity,true,true,false,true,false,false,false,false);
    }
    bool plantVisible(gta::Vec from,gta::Vec root){
        if(!line)return true;ColPoint point{};void* entity{};
        // End above the root, not on its ground polygon. Ignore traffic/peds
        // and dynamic objects; they must not make a grass patch blink.
        root.z+=.12f;
        return !line(from,root,point,entity,true,false,false,false,true,false,false,false);
    }
    bool init(unsigned char* mem){
        gta::OriginalImage image;if(!image.load())return false;
        auto candidates=image.scan("55 8B EC B8 FF FF 00 00 83 EC 40 66 39 05 ? ? ? ? 73 09 66 FF 05 ? ? ? ? EB 11 E8 ? ? ? ? B9 01 00 00 00 66 89 0D ? ? ? ? 53 56 8B 75 08 C7 05 ? ? ? ? 00 00 00 00 D9 06");
        auto infos=image.scan("0F BF 41 22 8B 0C 85 ? ? ? ? 8B 51 14 D9 42 24 C3");
        auto packed=image.scan("55 8B EC 8B 45 08 66 8B 10 66 89 51 23 8A 40 02 88 41 25 5D C2 04 00");
        if(candidates.size()!=1||infos.size()!=1||packed.size()!=1)return false;DWORD table{};if(!gta::read(infos[0]+7,table))return false;
        line=reinterpret_cast<Line>(image.code(candidates[0]));models=image.global(table);mapping=mem;
        HANDLE thread=CreateThread(nullptr,0,workerEntry,this,0,nullptr);if(!thread)return false;CloseHandle(thread);supported=true;return true;
    }
    void tick(const gta::Snapshot& s,bool connected){
        if(!supported)return;
        digging=nativeExcavation.supported;
        if(!connected||!s.valid){if(active){invalidate();active=false;}return;}
        if(s.menu)return; // Terrain streaming must continue through driving/tutorial missions.
        if(!active||std::hypot(previous.x-s.position.x,previous.y-s.position.y)>96||std::abs(previous.z-s.position.z)>32){
            invalidate();active=true;
        }previous=s.position;
        if(!wallsAnchored||std::hypot(s.position.x-wallAnchor.x,s.position.y-wallAnchor.y)>96){
            exactWalls.clear();wallSources.clear();wallAnchor=s.position;wallsAnchored=true;
        }
        auto now=GetTickCount64();int rx=int(std::floor(s.position.x/8)),rz=int(std::floor(-s.position.y/8));
        if(interactionProbe&&(std::abs(interactionProbe->Rx()-rx)>3||std::abs(interactionProbe->Rz()-rz)>3))interactionProbe.reset();
        if(!interactionProbe&&now<interactionUntil){
            for(auto key:surfaceColumns(interactionCell[0],interactionCell[2])){
            auto old=cache.find(key);float y=float(interactionCell[1]);
            bool ready=old!=cache.end()&&!old->second.coarse&&now-old->second.stamp<120000&&y>old->second.low+2&&y<old->second.high-2;
            if(!ready&&std::abs(key[0]-rx)<=2&&std::abs(key[1]-rz)<=2){
                if(probe&&probe->Rx()==key[0]&&probe->Rz()==key[1])interactionProbe=std::move(probe);
                else {int base=int(std::floor((s.position.z-1)/8));
                    interactionProbe=std::make_unique<lc::col::ColumnProbe>(key[0],key[1],float((base-3)*8-2),float((base+7)*8+2));}
                break;
            }
            }
        }
        if(interactionProbe){
            LARGE_INTEGER start{},freq{};QueryPerformanceCounter(&start);QueryPerformanceFrequency(&freq);unsigned begin=rays;
            auto exhausted=[&](){LARGE_INTEGER t{};QueryPerformanceCounter(&t);return rays-begin>=96||(t.QuadPart-start.QuadPart)*1000.0/freq.QuadPart>=2.0;};
            if(interactionProbe->Run([&](const float* a,const float* b,lc::col::Hit& h){return cast(a,b,h);},exhausted)){
                auto column=interactionProbe->Result();if(enqueue({currentEpoch,column,{column->rx,column->rz},{}})){
                    cache[{column->rx,column->rz}]={now,column->yLo,column->yHi};interactionProbe.reset();
                }
            }
            return; // Same frame budget, not an extra 2ms on top of surveying.
        }
        for(auto i=parcelPriority.begin();i!=parcelPriority.end();)if(i->second<now)i=parcelPriority.erase(i);else ++i;
        // Vehicle: two thirds of frames survey pasture; on foot: one third.
        // Detailed wall/underground sampling retains the same 2ms/96-ray cap.
        ++framePass;
        if(s.vehicle?framePass%3!=0:framePass%3==0){
            if(!groundProbe){
                gta::Vec velocity{};gta::read(s.ped+0x44,velocity);
                if(s.vehicle){DWORD car{};if(gta::read(s.ped+0x58c,car)&&car)gta::read(car+0x44,velocity);}
                float lx=std::clamp(velocity.x*150.f,-112.f,112.f)/8,lz=std::clamp(-velocity.y*150.f,-112.f,112.f)/8;
                if(!std::isfinite(lx+lz))lx=lz=0;
                bool distant=++groundChoices%2==0&&std::hypot(lx,lz)<4;
                double best=1e9;std::array<int,2> choice{};bool found=false;
                for(int dz=-20;dz<=20;++dz)for(int dx=-20;dx<=20;++dx){
                    int distance=dx*dx+dz*dz;if(distance<4||distance>400)continue;
                    std::array<int,2> key{rx+dx,rz+dz};auto cached=cache.find(key);
                    if(cached!=cache.end()&&now-cached->second.stamp<120000)continue;
                    auto retry=groundRetry.find(key);if(retry!=groundRetry.end()&&now<retry->second)continue;
                    double rank=(dx-lx)*(dx-lx)+(dz-lz)*(dz-lz);
                    if(distant)rank=std::abs(std::sqrt(double(distance))-13)*20;
                    if(parcelPriority.count(key))rank-=20000;
                    if(rank<best){best=rank;choice=key;found=true;}
                }
                if(found)groundProbe=std::make_unique<GroundProbe>(choice[0],choice[1],s.position.z-1);
            }
            if(groundProbe){
                LARGE_INTEGER start{},frequency{};QueryPerformanceCounter(&start);QueryPerformanceFrequency(&frequency);unsigned begin=rays;
                auto exhausted=[&](){LARGE_INTEGER t{};QueryPerformanceCounter(&t);return rays-begin>=96||(t.QuadPart-start.QuadPart)*1000.0/frequency.QuadPart>=2;};
                if(groundProbe->run([&](const float* a,const float* b,lc::col::Hit& hit){return cast(a,b,hit);},exhausted)){
                    std::array<int,2> key{groundProbe->rx(),groundProbe->rz()};
                    auto newer=cache.find(key);
                    if(newer!=cache.end()&&!newer->second.coarse&&now-newer->second.stamp<120000){groundProbe.reset();return;}
                    if(!groundProbe->valid()){groundRetry[key]=now+5000;groundProbe.reset();}
                    else {auto column=groundProbe->result();if(enqueue({currentEpoch,column,key,{}})){
                        cache[key]={now,column->yLo,column->yHi,true};groundProbe.reset();
                    }}
                }
            }
            if(groundRetry.size()>512)groundRetry.erase(groundRetry.begin());
            return;
        }
        if(!probe&&now>=nextChoice){
            for(auto it=cache.begin();it!=cache.end();)if(std::abs(it->first[0]-rx)>28||std::abs(it->first[1]-rz)>28){
                if(enqueue({currentEpoch,nullptr,it->first,{}}))it=cache.erase(it);else break;
            }else ++it;
            double nearest=1e9;std::array<int,2> choice{};bool found=false;
            gta::Vec speed{};gta::read(s.ped+0x44,speed);
            if(s.vehicle){DWORD vehicle{};BYTE kind{};if(gta::read(s.ped+0x58c,vehicle)&&vehicle&&gta::read(vehicle+0x36,kind)&&(kind&7)==2)gta::read(vehicle+0x44,speed);}
            float leadX=std::clamp(speed.x*300.f,-144.f,144.f)/8,leadZ=std::clamp(-speed.y*300.f,-144.f,144.f)/8;
            if(!std::isfinite(leadX+leadZ))leadX=leadZ=0;
            bool farPass=++surveyPass%3==0;
            if(farPass&&std::hypot(leadX,leadZ)<4){gta::Vec forward{};if(s.matrix&&gta::read(s.matrix+0x10,forward)){leadX=forward.x*16;leadZ=-forward.y*16;}}
            for(int dz=-22;dz<=22;++dz)for(int dx=-22;dx<=22;++dx){
                if(dx*dx+dz*dz>529)continue;std::array<int,2> key{rx+dx,rz+dz};auto it=cache.find(key);
                bool fresh=it!=cache.end()&&now-it->second.stamp<120000
                    && !(it->second.coarse&&(dx*dx+dz*dz<=9||parcelPriority.count(key)))
                    && (it->second.coarse||(s.position.z>it->second.low+10&&s.position.z<it->second.high-18));
                if(fresh)continue;double rank=dx*dx+dz*dz<=9?dx*dx+dz*dz-10000:(dx-leadX)*(dx-leadX)+(dz-leadZ)*(dz-leadZ);
                if(farPass&&dx*dx+dz*dz>144)rank-=500;
                rank+=(it==cache.end()?0:it->second.coarse?-500:20000);
                if(parcelPriority.count(key))rank-=40000;
                if(dx*dx+dz*dz<=4)rank-=100000; // nearby interaction data cannot starve behind far parcels
                if(rank<nearest){nearest=rank;choice=key;found=true;}
            }
            if(!found){nextChoice=now+1000;return;}
            int y=int(std::floor((s.position.z-1)/8));probe=std::make_unique<lc::col::ColumnProbe>(choice[0],choice[1],float((y-3)*8-2),float((y+7)*8+2));
        }
        if(!probe)return;
        LARGE_INTEGER start{},freq{};QueryPerformanceCounter(&start);QueryPerformanceFrequency(&freq);unsigned begin=rays;
        auto exhausted=[&](){LARGE_INTEGER t{};QueryPerformanceCounter(&t);return rays-begin>=96||(t.QuadPart-start.QuadPart)*1000.0/freq.QuadPart>=2.0;};
        if(probe->Run([&](const float* a,const float* b,lc::col::Hit& h){return cast(a,b,h);},exhausted)){
            auto column=probe->Result();if(enqueue({currentEpoch,column,{column->rx,column->rz},{}})){
                cache[{column->rx,column->rz}]={now,column->yLo,column->yHi};probe.reset();
            }
        }
    }
};
}
