#pragma once
#include <functional>
#include "NativeVegetation.h"
#include "ExcavationCache.h"
// Persistent masks belong to Minecraft's chunk attachments. This adapter only
// consumes the bounded nearby render snapshot; no stock COL/model is modified.
namespace grove {
class NativeExcavation {
    using Key=std::array<int,4>;
    std::map<Key,SectionBits> dug;
    std::map<Key,std::map<unsigned,NativeBox>> chamberBounds; // Minecraft cell-local bounds, GCC1 readiness payload
    mutable std::map<SectionKey,std::vector<NativeBox>> cachedChambers;
    mutable unsigned chamberRevision=~0u,chamberWorld=~0u;
    std::set<const NativeCol*> staticCols;
    std::set<const NativeCol*> woodCols;
    std::uintptr_t modelTable{};
public:
    ExcavationCache cutCache;
    unsigned cutCacheHits{},cutCacheMisses{};
    double cutMicros{};
    struct Matrix {gta::Vec right;DWORD a;gta::Vec forward;DWORD b;gta::Vec up;DWORD c;gta::Vec pos;DWORD d;};
    struct Point {gta::Vec pos;float pad;gta::Vec normal;float pad2;BYTE surface[8];float depth;};
    struct Line {gta::Vec from;float pad;gta::Vec to;float pad2;};
    static_assert(sizeof(Point)==44&&sizeof(Line)==32&&sizeof(Matrix)==64);
    using Physics=int (__cdecl*)(const Matrix&,NativeCol&,const Matrix&,NativeCol&,Point*,Point*,float*,bool);
    using Ray=bool (__cdecl*)(const Line&,const Matrix&,NativeCol&,Point&,float&,bool,bool);
    using Vertical=bool (__cdecl*)(const Line&,const Matrix&,NativeCol&,Point&,float&,bool,bool,void*);
    using Test=bool (__cdecl*)(const Line&,const Matrix&,NativeCol&,bool,bool);
    using Render=void (__thiscall*)(void*);
    Physics physics{};Ray ray{};Vertical vertical{};Test test{};Render render{};
    void (__cdecl* plants)(){};
    std::uintptr_t plantsCode{};
    bool drawingPlants{};
    unsigned plantDraws{},plantsRemoved{};
    std::uintptr_t physicsCode{},rayCode{},verticalCode{},testCode{},renderCode{};
    bool supported{},enabled{},sampling{},rendering{};
    unsigned world{},revision{},cells{},packets{},removedContacts{},removedRays{},clippedDraws{},rejectedDraws{};
    Matrix renderMatrix{};
    bool woodContext{};
    gta::Vec woodRoot{};float woodRadius{.65f};
    std::function<bool(const SectionKey&)> chamberReady;
    bool bit(int scope,int x,int y,int z)const{
        auto i=dug.find({scope,x>>4,y>>4,z>>4});unsigned b=(x&15)+16*(z&15)+256*(y&15);
        return i!=dug.end()&&(i->second[b/8]&(1u<<(b%8)));
    }
    bool architectureReady(int x,int y,int z)const{
        return bit(int(world)+2304,x,y,z)&&chamberReady&&chamberReady({x>>4,y>>4,z>>4});
    }
    const std::map<SectionKey,std::vector<NativeBox>>& chambers()const{
        if(chamberRevision==revision&&chamberWorld==world)return cachedChambers;
        auto& out=cachedChambers;out.clear();chamberRevision=revision;chamberWorld=world;
        for(const auto& pair:dug){auto k=pair.first;if(k[0]!=int(world)+512)continue;
            for(int b=0;b<4096;++b)if(pair.second[b/8]&(1u<<(b%8))){
                int x=k[1]*16+(b&15),y=k[2]*16+(b>>8),z=k[3]*16+((b>>4)&15);
                if(!bit(int(world)+2304,x,y,z))continue;
                int entry=-1;for(int f=0;f<6;++f)if(bit(int(world)+768+f*256,x,y,z)){entry=f;break;}
                if(entry<0)continue;
                const int dx[]={0,0,0,0,-1,1},dy[]={-1,1,0,0,0,0},dz[]={0,0,-1,1,0,0};
                NativeBox bounds;bounds.min={0,0,0};bounds.max={1,1,1};
                auto section=chamberBounds.find({int(world)+2304,k[1],k[2],k[3]});
                if(section!=chamberBounds.end()){auto cell=section->second.find(b);if(cell==section->second.end())continue;bounds=cell->second;}
                for(int f=0;f<6;++f){if(f==entry)continue;
                    int nx=x+dx[f],ny=y+dy[f],nz=z+dz[f];
                    // A neighbouring bit alone is insufficient: shallow cuts may
                    // not touch. Cross-section openings wait for a shared collider.
                    if(section!=chamberBounds.end()&&(nx>>4)==k[1]&&(ny>>4)==k[2]&&(nz>>4)==k[3]
                        &&bit(int(world)+512,nx,ny,nz)&&bit(int(world)+2304,nx,ny,nz)){
                        unsigned nb=(nx&15)+16*(nz&15)+256*(ny&15);auto neighbour=section->second.find(nb);
                        if(neighbour!=section->second.end()&&connectedBounds(bounds,neighbour->second,f))continue;
                    }
                    NativeBox box;box.min={float((x&15)-8)+bounds.min.x,float(8-(z&15))-bounds.max.z,float((y&15)-8)+bounds.min.y};
                    box.max={float((x&15)-8)+bounds.max.x,float(8-(z&15))-bounds.min.z,float((y&15)-8)+bounds.max.y};
                    if(f==0)box.max.z=box.min.z+.04f;if(f==1)box.min.z=box.max.z-.04f;
                    if(f==2)box.min.y=box.max.y-.04f;if(f==3)box.max.y=box.min.y+.04f;
                    if(f==4)box.max.x=box.min.x+.04f;if(f==5)box.min.x=box.max.x-.04f;
                    out[{k[1],k[2],k[3]}].push_back(box);
                }
            }
        }return out;
    }
    static bool connectedBounds(const NativeBox& a,const NativeBox& b,int face){
        float lo[]={a.min.x,a.min.y,a.min.z},hi[]={a.max.x,a.max.y,a.max.z};
        float otherLo[]={b.min.x,b.min.y,b.min.z},otherHi[]={b.max.x,b.max.y,b.max.z};
        int axis=face<2?1:face<4?2:0;bool positive=(face&1)!=0;
        if(positive?(std::abs(hi[axis]-1)>.001f||std::abs(otherLo[axis])>.001f)
                   :(std::abs(lo[axis])>.001f||std::abs(otherHi[axis]-1)>.001f))return false;
        for(int j=0;j<3;++j)if(j!=axis&&(otherLo[j]>lo[j]+.001f||otherHi[j]<hi[j]-.001f))return false;
        return true;
    }
    std::function<void(IDirect3DDevice9*,const std::vector<BYTE>&,unsigned,const std::vector<D3DVERTEXELEMENT9>&)> capture;
    bool model(void* entity,short& id,NativeCol& bounds,const NativeCol*& identity)const{
        DWORD info{},col{};auto ptr=reinterpret_cast<std::uintptr_t>(entity);
        if(!modelTable||!gta::read(ptr+0x22,id)||id<0||id>=19000||!gta::read(modelTable+id*4,info)||!info
            ||!gta::read(info+0x14,col)||!col||!gta::read(col,bounds))return false;
        identity=reinterpret_cast<const NativeCol*>(col);return true;
    }
    unsigned indexedCalls{},worldDraws{},affectedEntities{},noBoxDraws{},lastType{},stage{};
    gta::Vec lastDrawMin{},lastDrawMax{},lastCell{};
    static gta::Vec transform(const Matrix& m,gta::Vec p){return {m.pos.x+p.x*m.right.x+p.y*m.forward.x+p.z*m.up.x,m.pos.y+p.x*m.right.y+p.y*m.forward.y+p.z*m.up.y,m.pos.z+p.x*m.right.z+p.y*m.forward.z+p.z*m.up.z};}
    void clear(){dug.clear();chamberBounds.clear();staticCols.clear();woodCols.clear();cutCache.clear();cells=0;++revision;}
    bool affected(void* entity,const Matrix& matrix){
        short model{};DWORD info{},col{};auto ptr=reinterpret_cast<std::uintptr_t>(entity);
        if(!modelTable||!gta::read(ptr+0x22,model)||model<0||model>=19000||!gta::read(modelTable+model*4,info)||!info||!gta::read(info+0x14,col)||!col)return false;
        staticCols.insert(reinterpret_cast<const NativeCol*>(col));
        woodContext=treeDigMaterial(model)!=0;
        if(woodContext)woodCols.insert(reinterpret_cast<const NativeCol*>(col));
        if(staticCols.size()>10000)staticCols.clear();
        NativeCol bounds;if(!gta::read(col,bounds))return false;
        if(woodContext){woodRoot=transform(matrix,{0,0,std::max(0.f,bounds.min.z)});
            woodRadius=std::clamp(std::min(bounds.max.x-bounds.min.x,bounds.max.y-bounds.min.y)*.25f,.35f,1.6f);}
        gta::Vec lo{1e9f,1e9f,1e9f},hi{-1e9f,-1e9f,-1e9f};
        for(int i=0;i<8;++i){auto v=transform(matrix,{i&1?bounds.max.x:bounds.min.x,i&2?bounds.max.y:bounds.min.y,i&4?bounds.max.z:bounds.min.z});
            lo={std::min(lo.x,v.x),std::min(lo.y,v.y),std::min(lo.z,v.z)};hi={std::max(hi.x,v.x),std::max(hi.y,v.y),std::max(hi.z,v.z)};}
        bool yes=!boxes({lo.x-.005f,lo.y-.005f,lo.z-.005f},{hi.x+.005f,hi.y+.005f,hi.z+.005f}).empty();if(yes)++affectedEntities;return yes;
    }
    bool nativeSurface(const NativeCol& col)const{return staticCols.count(&col)!=0;}
    bool frame(const void* payload,unsigned bytes){
        if(bytes<sizeof(p::RenDug))return false;p::RenDug h{};std::memcpy(&h,payload,sizeof h);
        if(h.count>4096||h.worldId>2559||std::abs(h.sx)>1000||std::abs(h.sz)>1000||h.sy<-256||h.sy>256)return false;
        Key key{int(h.worldId),h.sx,h.sy,h.sz};
        if(!h.count&&bytes==sizeof h){chamberBounds.erase(key);if(dug.erase(key))++revision;return true;}
        bool bounded=h.worldId>=2304&&h.pad==0x47434331;
        if(bytes!=sizeof h+512+(bounded?h.count*24:0)||(!dug.count(key)&&dug.size()>=512))return false;
        SectionBits bits{};std::memcpy(bits.data(),static_cast<const BYTE*>(payload)+sizeof h,512);
        unsigned count=0;for(auto b:bits)for(;b;b&=b-1)++count;if(count!=h.count)return false;
        if(bounded){std::map<unsigned,NativeBox> bounds;unsigned offset=sizeof h+512;
            for(unsigned b=0;b<4096;++b)if(bits[b/8]&(1u<<(b%8))){float v[6];std::memcpy(v,static_cast<const BYTE*>(payload)+offset,24);offset+=24;
                for(unsigned j=0;j<3;++j)if(!std::isfinite(v[j])||!std::isfinite(v[j+3])||v[j]<0||v[j+3]>1||v[j+3]-v[j]<.08f)return false;
                NativeBox box;box.min={v[0],v[1],v[2]};box.max={v[3],v[4],v[5]};bounds.emplace(b,box);
            }
            chamberBounds[key]=std::move(bounds);++revision;
        }else if(h.worldId>=2304)chamberBounds.erase(key);
        for(unsigned bit=0;bit<4096;++bit)if(bits[bit/8]&(1u<<(bit%8))){lastCell={float(h.sx*16+int(bit&15)),float(-h.sz*16-int((bit>>4)&15)-1),float(h.sy*16+int(bit>>8))};break;}
        if(!dug.count(key)||dug.at(key)!=bits){dug[key]=bits;++revision;}
        cells=0;for(const auto& pair:dug)if(pair.first[0]<768)for(auto b:pair.second)for(;b;b&=b-1)++cells;
        ++packets;return true;
    }
    bool woodSurface(const NativeCol& col)const{return woodCols.count(&col)!=0;}
    bool terrainBoundary(int x,int y,int z)const{
        if(!enabled||sampling||world>255||bit(int(world),x,y,z))return false;
        const int dx[]={0,0,0,0,-1,1},dy[]={-1,1,0,0,0,0},dz[]={0,0,-1,1,0,0};
        for(int face=0;face<6;++face)
            if(bit(int(world),x+dx[face],y+dy[face],z+dz[face]))return true;
        return false;
    }
    bool woodCut(gta::Vec pos)const{
        if(!enabled||sampling||!std::isfinite(pos.x+pos.y+pos.z))return false;
        int x=int(std::floor(pos.x)),y=int(std::floor(pos.z)),z=int(std::floor(-pos.y));
        auto it=dug.find({int(world)+256,x>>4,y>>4,z>>4});if(it==dug.end())return false;
        unsigned bit=(x&15)+16*(z&15)+256*(y&15);return it->second[bit/8]&(1u<<(bit%8));
    }
    bool contains(gta::Vec pos,bool wood=false)const{
        if(!enabled||!std::isfinite(pos.x+pos.y+pos.z))return false;
        int x=int(std::floor(pos.x)),y=int(std::floor(pos.z)),z=int(std::floor(-pos.y));
        unsigned bit=(x&15)+16*(z&15)+256*(y&15);
        for(int scope:{sampling?-1:int(world),wood?(sampling?-1:int(world)+256):int(world)+512}){auto it=dug.find({scope,x>>4,y>>4,z>>4});
            if(scope==int(world)+512&&(sampling||!architectureReady(x,y,z)))continue;
            if(it!=dug.end()&&(it->second[bit/8]&(1u<<(bit%8)))){
                if(scope==int(world)+512){
                    auto section=chamberBounds.find({int(world)+2304,x>>4,y>>4,z>>4});
                    if(section==chamberBounds.end())continue;
                    auto cell=section->second.find(bit);if(cell==section->second.end())continue;
                    gta::Vec q{pos.x-x,pos.z-y,-pos.y-z};const auto& b=cell->second;
                    if(q.x<b.min.x||q.x>b.max.x||q.y<b.min.y||q.y>b.max.y||q.z<b.min.z||q.z>b.max.z)continue;
                }
                return true;
            }}
        return false;
    }
    std::vector<NativeBox> boxes(gta::Vec min,gta::Vec max)const{
        std::vector<NativeBox> out;if(!enabled)return out;
        for(const auto& pair:dug){auto k=pair.first;if(k[0]!=int(world)&&k[0]!=int(world)+(woodContext?256:512))continue;
            gta::Vec lo{float(k[1]*16),float(-k[3]*16-16),float(k[2]*16)},hi{lo.x+16,lo.y+16,lo.z+16};
            if(hi.x<min.x||lo.x>max.x||hi.y<min.y||lo.y>max.y||hi.z<min.z||lo.z>max.z)continue;
            if(k[0]==int(world)+512){
                auto section=chamberBounds.find({int(world)+2304,k[1],k[2],k[3]});
                if(section==chamberBounds.end())continue;
                // The render facade can extend beyond its collision footprint.
                // Clip exactly the volume lined by the guest, never the rest of
                // the unit cube (which would leave an unbacked transparent strip).
                for(const auto& cell:section->second){
                    int bit=cell.first,x=k[1]*16+(bit&15),y=k[2]*16+(bit>>8),z=k[3]*16+((bit>>4)&15);
                    if(!(pair.second[bit/8]&(1u<<(bit%8)))||!architectureReady(x,y,z))continue;
                    const auto& bounds=cell.second;NativeBox b;
                    b.min={x+bounds.min.x,-z-bounds.max.z,y+bounds.min.y};
                    b.max={x+bounds.max.x,-z-bounds.min.z,y+bounds.max.y};
                    if(b.max.x>=min.x&&b.min.x<=max.x&&b.max.y>=min.y&&b.min.y<=max.y&&b.max.z>=min.z&&b.min.z<=max.z)out.push_back(b);
                }
                continue;
            }
            auto occupied=[&](int x,int y,int z){unsigned b=x+16*z+256*y;return (pair.second[b/8]&(1u<<(b%8)))&&
                (k[0]!=int(world)+512||architectureReady(k[1]*16+x,k[2]*16+y,k[3]*16+z));};
            for(int y=0;y<16;++y)for(int z=0;z<16;++z)for(int x=0;x<16;){
                if(!occupied(x,y,z)){++x;continue;}int begin=x;while(x<16&&occupied(x,y,z))++x;
                NativeBox b;b.min={lo.x+begin,float(-k[3]*16-z-1),lo.z+y};b.max={lo.x+x,b.min.y+1,b.min.z+1};
                if(b.max.x>=min.x&&b.min.x<=max.x&&b.max.y>=min.y&&b.min.y<=max.y&&b.max.z>=min.z&&b.min.z<=max.z)out.push_back(b);
            }
        }return out;
    }
    int contacts(const Matrix& a,NativeCol& ca,const Matrix& b,NativeCol& cb,Point* spheres,Point* lines,float* distances,bool all){
        int count=physics(a,ca,b,cb,spheres,lines,distances,all);
        if(!enabled||sampling||!cells||(!nativeSurface(cb)&&!nativeSurface(ca))||count<0||count>31)return count;
        bool wood=woodSurface(cb)||woodSurface(ca);
        int keep=0;for(int i=0;i<count;++i){if(contains(spheres[i].pos,wood)){++removedContacts;continue;}spheres[keep++]=spheres[i];}
        // A line miss is encoded by its original seeded distance. Do not read
        // uninitialized contacts, and preserve contacts made by other models.
        if(lines&&distances&&ca.data&&ca.data->lines<=16)for(unsigned i=0;i<ca.data->lines;++i){
            if(distances[i]>=0&&distances[i]<1&&contains(lines[i].pos,wood)){distances[i]=1;lines[i]={};++removedContacts;}
        }
        return keep;
    }
    template<class Call> bool sight(const Line& ln,Point& cp,float& dist,Call call,bool wood=false){
        if(!enabled||!cells||(sampling&&wood))return call(ln,cp,dist);
        Line remaining=ln;float progress=0,limit=dist;
        for(unsigned i=0;i<64;++i){Point hit{};float local=(limit-progress)/(1-progress);
            if(local<=0||!call(remaining,hit,local))return false;
            float global=progress+(1-progress)*local;
            if(!contains(hit.pos,wood)){cp=hit;dist=global;return true;}
            ++removedRays;float len=std::sqrt((ln.to.x-ln.from.x)*(ln.to.x-ln.from.x)+(ln.to.y-ln.from.y)*(ln.to.y-ln.from.y)+(ln.to.z-ln.from.z)*(ln.to.z-ln.from.z));
            if(len<.001f)return false;progress=global+.01f/len;if(progress>=limit||progress>=1)return false;
            remaining.from={ln.from.x+(ln.to.x-ln.from.x)*progress,ln.from.y+(ln.to.y-ln.from.y)*progress,ln.from.z+(ln.to.z-ln.from.z)*progress};
        }return false;
    }
    bool init(){
        gta::OriginalImage image;if(!image.load())return false;
        auto a=image.scan("55 8B EC 64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 A1 ? ? ? ? 81 EC 14 01 00 00 A8 01");
        auto b=image.scan("55 8B EC 83 EC 38 F6 05 ? ? ? ? 01 75 20 83 0D ? ? ? ? 01 33 C0 68 ? ? ? ? A3 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? 83 C4 04 53 8B 5D 10 56 8B 73 2C");
        auto c=image.scan("55 8B EC 8B 4D 10 83 EC 34 56 8B 71 2C 57 33 FF 3B F7 75 08 5F 32 C0 5E 8B E5 5D C3");
        auto d=image.scan("55 8B EC 83 EC 38 53 8B 5D 10 56 8B 73 2C 33 C0 3B F0 75 08 5E 32 C0 5B 8B E5 5D C3");
        auto e=image.scan("55 8B EC 51 56 8B F1 8B 46 18 85 C0 0F 84 ? ? ? ? 80 38 01 75 1E 56 E8 ? ? ? ? 83 C4 04 84 C0 74 11");
        auto infos=image.scan("0F BF 41 22 8B 0C 85 ? ? ? ? 8B 51 14 D9 42 24 C3");
        // CPlantMgr::Render: quality gate followed by the documented grass
        // Z-write/Z-test/vertex-alpha render states. Optional on other builds.
        auto grass=image.scan("55 8B EC 83 EC 78 B9 ? ? ? ? E8 ? ? ? ? 85 C0 0F 84 ? ? ? ? A1 ? ? ? ? 8B 48 20 53 56 57 6A 00 6A 08 FF D1");
        if(grass.size()==1)plantsCode=image.code(grass[0]);
        if(a.size()!=1||b.size()!=1||c.size()!=1||d.size()!=1||e.size()!=1||infos.size()!=1)return false;
        DWORD table{};if(!gta::read(infos[0]+7,table))return false;modelTable=image.global(table);
        physicsCode=image.code(a[0]);rayCode=image.code(b[0]);verticalCode=image.code(c[0]);testCode=image.code(d[0]);renderCode=image.code(e[0]);return true;
    }
    // Fixture-only registration uses the same identity check as real building rendering.
    void registerSurfaceForTest(const NativeCol& col){staticCols.insert(&col);}
};
}
