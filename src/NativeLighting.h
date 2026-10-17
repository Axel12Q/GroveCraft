#pragma once
#include <unordered_map>
namespace grove {
// Read-only GTA timecycle and frame point-light list. Operand signatures from
// the owned NewSteam executable; layouts/attenuation from GTA-reversed PointLights.
class NativeLighting {
    struct Point {gta::Vec position,direction;float radius,r,g,b;unsigned entity;unsigned char type,fog,shadows,pad;};
    static_assert(sizeof(Point)==48);
    std::uintptr_t ambientAddress{},countAddress{},lightsAddress{};
    std::array<Point,32> points{};unsigned count{};
    std::unordered_map<std::uint64_t,std::array<float,3>> localCache;
public:
    bool supported{};std::array<float,3> ambient{1,1,1},objectAmbient{1,1,1};float daylight{1};unsigned active{},samples{};
    std::uint64_t revision{};
    bool init(){
        gta::OriginalImage image;if(!image.load())return false;
        auto a=image.scan("55 8B EC 51 D9 05 ? ? ? ? D8 0D ? ? ? ? D9 5D FC D9 45 FC 8B E5 5D C3 CC CC CC CC CC CC D9 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC D9 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC D9 05 ? ? ? ? C3");
        auto n=image.scan("56 8B 35 ? ? ? ? 83 FE 20 0F 8D ? ? ? ? 85 C9 74 07 8D 41 30 8B C8");
        auto p=image.scan("8D 0C 76 C1 E1 04 88 81 ? ? ? ? 8B 45 0C 88 91 ? ? ? ? 8B 55 10 89 81 ? ? ? ?");
        if(a.size()!=1||n.size()!=1||p.size()!=1)return false;
        DWORD aa{},nn{},pp{};if(!gta::read(a[0]+34,aa)||!gta::read(n[0]+3,nn)||!gta::read(p[0]+26,pp))return false;
        ambientAddress=image.global(aa);countAddress=image.global(nn);lightsAddress=image.global(pp);
        return supported=gta::accessible(reinterpret_cast<void*>(ambientAddress),12)&&gta::accessible(reinterpret_cast<void*>(lightsAddress),sizeof points);
    }
    static float smooth(float x){x=std::clamp(x,0.f,1.f);return x*x*(3-2*x);}
    static float lampFactor(gta::Vec delta,gta::Vec direction,float radius,unsigned type){
        if((type!=0&&type!=1)||!std::isfinite(radius)||radius<=0||radius>100)return 0;
        float squared=delta.x*delta.x+delta.y*delta.y+delta.z*delta.z;
        if(!std::isfinite(squared)||squared>=radius*radius)return 0;
        float distance=std::sqrt(squared),falloff=smooth(1-distance/radius);
        if(type==1&&distance>.01f){float dot=(delta.x*direction.x+delta.y*direction.y+delta.z*direction.z)/distance;
            if(!std::isfinite(dot))return 0;falloff*=std::clamp((dot-.5f)*2.f,0.f,1.f);}
        return falloff;
    }
    static float daylightAt(float hour){return smooth((hour-5.f)/3.f)*(1-smooth((hour-18.f)/4.f));}
    void begin(float hour){
        revision=GetTickCount64()/125;localCache.clear();samples=0;count=active=0;
        daylight=daylightAt(hour);if(!supported){ambient={1,1,1};return;}
        std::array<float,6> value{};
        // Objects have a high base ambient in GTA, with day/night prelighting on
        // their own models. Mix both ambient triplets instead of treating that
        // object base alone as daylight (which would brighten the whole night).
        if(gta::read(ambientAddress-12,value)&&std::all_of(value.begin(),value.end(),[](float x){return std::isfinite(x)&&x>=0&&x<=4;})){
            std::copy_n(value.begin(),3,ambient.begin());std::copy_n(value.begin()+3,3,objectAmbient.begin());
        }
        unsigned size{};if(gta::read(countAddress,size)&&size<=32&&gta::read(lightsAddress,points))count=size;
        for(unsigned i=0;i<count;++i){const auto& p=points[i];
            if((p.type==0||p.type==1)&&std::isfinite(p.radius)&&p.radius>0&&p.radius<=100&&std::isfinite(p.r+p.g+p.b))++active;}
    }
    std::array<float,3> local(gta::Vec at){
        if(!active)return {}; // No map lookups for unlit daytime meshes.
        // One evaluation per 1m cell, shared by blocks, furniture and entities.
        int x=int(std::floor(at.x)),y=int(std::floor(at.y)),z=int(std::floor(at.z));
        auto key=(std::uint64_t(unsigned(x)&0xfffff)<<40)|(std::uint64_t(unsigned(y)&0xfffff)<<20)|(unsigned(z)&0xfffff);
        auto it=localCache.find(key);if(it!=localCache.end())return it->second;
        at={x+.5f,y+.5f,z+.5f};std::array<float,3> result{};++samples;
        for(unsigned i=0;i<count;++i){const auto& p=points[i];
            if((p.type!=0&&p.type!=1)||!std::isfinite(p.radius)||p.radius<=0||p.radius>100||!std::isfinite(p.r+p.g+p.b))continue;
            float dx=at.x-p.position.x,dy=at.y-p.position.y,dz=at.z-p.position.z;
            float falloff=lampFactor({dx,dy,dz},p.direction,p.radius,p.type);if(falloff<=0)continue;
            result[0]+=std::clamp(p.r,0.f,2.f)*falloff*.8f;
            result[1]+=std::clamp(p.g,0.f,2.f)*falloff*.8f;
            result[2]+=std::clamp(p.b,0.f,2.f)*falloff*.8f;
        }
        if(localCache.size()<16384)localCache.emplace(key,result);return result;
    }
    std::array<float,3> factors(gta::Vec at,unsigned light,unsigned flags){
        if(!supported)return {1,1,1};auto lamps=local(at);
        float sky=std::clamp(float((light>>8)&15)/15.f,0.f,1.f),block=std::clamp(float(light&15)/15.f,0.f,1.f);
        unsigned face=(flags>>4)&7;float direction=face==1?.78f:face==2||face==0?1.f:.92f;
        std::array<float,3> value{};
        for(unsigned i=0;i<3;++i){
            float outdoor=.14f+std::clamp(ambient[i]*1.5f,0.f,.5f)+std::clamp(objectAmbient[i],0.f,1.f)*.16f+daylight*.48f;
            float environment=(.14f*(1-sky)+outdoor*sky)*direction;
            // Minecraft emission remains useful at night, with no second global darkening.
            float emission=block*block*.94f;
            value[i]=std::clamp(std::max(environment,emission)+lamps[i],.12f,1.f);
        }return value;
    }
    DWORD shade(DWORD color,gta::Vec at,unsigned light=0xf00,unsigned flags=0){
        auto f=factors(at,light,flags);DWORD out=color&0xff000000;
        for(unsigned i=0;i<3;++i){unsigned shift=16-i*8;out|=DWORD(std::lround(((color>>shift)&255)*f[i]))<<shift;}return out;
    }
    template<class V> void shadeMesh(std::vector<V>& mesh){for(auto& v:mesh)v.color=shade(v.color,{v.x,v.y,v.z},v.light,v.flags);}
};
}
