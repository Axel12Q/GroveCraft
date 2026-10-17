#pragma once
// Minecraft owns fluid simulation; GTA owns swimming/buoyancy. No stock water is edited.
namespace grove {
class NativeWater {
    std::map<SectionKey,std::array<BYTE,4096>> sections;
    unsigned seq{},tile{};ULONGLONG due{};
public:
    using Query=bool (__cdecl*)(float,float,float,float*,bool,gta::Vec*);
    using Flat=bool (__cdecl*)(float,float,float,float*,float*,float*);
    Query original{};Flat flat{};std::uintptr_t queryCode{};
    bool supported{},enabled{};unsigned cells{},rejected{},queries{},nativeGrids{},wetColumns{};
    void clear(){sections.clear();cells=0;}
    bool packet(const void* data,unsigned bytes){
        if(bytes<sizeof(p::RenLiquids))return false;p::RenLiquids h{};std::memcpy(&h,data,sizeof h);
        if(h.count>4096||std::abs(static_cast<long long>(h.sx))>1000||std::abs(static_cast<long long>(h.sz))>1000||h.sy<-256||h.sy>256||bytes!=sizeof h+h.count*sizeof(p::RenLiquid))return false;
        SectionKey key{h.sx,h.sy,h.sz};if(!h.count){sections.erase(key);recount();return true;}
        if(!sections.count(key)&&sections.size()>=512)return false;
        std::array<BYTE,4096> bits{};
        for(unsigned i=0;i<h.count;++i){p::RenLiquid v{};std::memcpy(&v,static_cast<const BYTE*>(data)+sizeof h+i*sizeof v,sizeof v);
            unsigned kind=(v.info>>4)&3;if(v.x>15||v.y>15||v.z>15||kind<1||kind>2||(v.info&0x80))return false;
            unsigned bit=v.x+16*v.z+256*v.y;if(bits[bit])return false;bits[bit]=v.info;
        }sections[key]=bits;recount();return true;
    }
    void recount(){cells=0;for(const auto& [k,b]:sections)for(BYTE v:b)if(v)++cells;}
    BYTE at(int x,int y,int z)const{auto it=sections.find({x>>4,y>>4,z>>4});return it==sections.end()?0:it->second[(x&15)+16*(z&15)+256*(y&15)];}
    bool source(int x,int y,int z)const{BYTE v=at(x,y,z);return (v&64)&&((v>>4)&3)==p::kLiquidWater;}
    bool surface(float x,float y,float z,float& height,bool force=false)const{
        if(!enabled||!std::isfinite(x+y+z)||std::abs(x)>16000||std::abs(y)>16000||std::abs(z)>4096)return false;
        int mx=int(std::floor(x)),mz=int(std::floor(-y)),base=int(std::floor(z));
        for(int dy=-1;dy<=3;++dy){int by=base+dy;BYTE v=at(mx,by,mz);if(((v>>4)&3)!=p::kLiquidWater)continue;
            for(int n=0;n<64;++n){BYTE above=at(mx,by+1,mz);if(((above>>4)&3)!=p::kLiquidWater)break;++by;v=above;}
            float top=by+(v&15)/15.f;
            if(top<z-.5f||(!force&&top-z>3.f))continue;height=top;return true;
        }return false;
    }
    bool query(float x,float y,float z,float* height,bool force,gta::Vec* normal){
        float top{};if(height&&surface(x,y,z,top,force)){*height=top;if(normal)*normal={0,0,1};++queries;return true;}
        return original(x,y,z,height,force,normal);
    }
    bool init(){
        gta::OriginalImage image;if(!image.load())return false;
        auto matches=image.scan("55 8B EC D9 45 10 83 EC 08 56 8B 75 14 8D 45 FC 50 8D 4D F8 51 56 83 EC 0C D9 5C 24 08 D9 45 0C D9 5C 24 04 D9 45 08 D9 1C 24 E8 ? ? ? ? 83 C4 18 84 C0 74 1C 80 7D 18 00");
        if(matches.size()!=1)return false;queryCode=image.code(matches[0]);
        std::int32_t displacement{};if(!gta::read(matches[0]+43,displacement))return false;
        auto target=queryCode+47+displacement;BYTE first{};if(!gta::read(target,first)||first!=0x55)return false;
        flat=reinterpret_cast<Flat>(target);supported=true;return true;
    }
    void publish(unsigned char* mem,const gta::Snapshot& s,bool ready){
        enabled=ready&&s.valid&&!s.menu&&!s.mission;
        if(!supported||!ready||!s.valid||s.menu||GetTickCount64()<due)return;due=GetTickCount64()+100;
        p::WaterGrid grid{};int px=int(std::floor(s.position.x)),pz=int(std::floor(-s.position.y));
        // Main grid alternates with the 5x5 lattice, so mobs also see nearby native lakes.
        bool main=(tile%2)==0;unsigned index=(tile/2)%25;++tile;
        grid.originX=main?px-8:(px>>4)*16+(int(index%5)-2)*16;
        grid.originZ=main?pz-8:(pz>>4)*16+(int(index/5)-2)*16;
        BYTE area{};gta::read(s.ped+0x2f,area);grid.worldId=area|(main?0:p::kWaterGridTile);wetColumns=0;
        for(int z=0;z<16;++z)for(int x=0;x<16;++x){float h{},a{},b{};
            bool wet=area==0&&flat(grid.originX+x+.5f,-grid.originZ-z-.5f,s.position.z,&h,&a,&b)&&std::isfinite(h)&&h>-1000&&h<2000;
            grid.surface[z*16+x]=wet?h:p::kNoWater;if(wet)++wetColumns;
        }
        auto* out=reinterpret_cast<p::WaterGrid*>(mem+p::kOffWaterGrid);
        InterlockedExchange(reinterpret_cast<volatile LONG*>(&out->seq),++seq);
        std::memcpy(reinterpret_cast<BYTE*>(out)+4,reinterpret_cast<BYTE*>(&grid)+4,sizeof grid-4);
        MemoryBarrier();InterlockedExchange(reinterpret_cast<volatile LONG*>(&out->seq),++seq);++nativeGrids;
    }
};
}
