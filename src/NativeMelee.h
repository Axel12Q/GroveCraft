#pragma once
namespace grove {
// Use GTA's damage event API: native armour, reactions, crime reporting and invulnerability remain authoritative.
class NativeMelee {
    using Damage=bool (__cdecl*)(void*,void*,int,int,int,unsigned char);
    Damage damage{};std::uintptr_t poolGlobal{};ULONGLONG due{};gta::Vec direction{};int points{},weapon{};
public:
    bool supported{},nullSourceSafe{};unsigned hits{};std::uintptr_t lastVictim{};float before{},after{};
    std::uintptr_t pedPool()const{return poolGlobal;}
    bool apply(std::uintptr_t victim,std::uintptr_t creator,float amount,unsigned kind){
        if(!supported||!std::isfinite(amount)||amount<=0||amount>100)return false;
        if(!creator&&!nullSourceSafe)return false;
        float health{};BYTE type{};if(!gta::read(victim+0x36,type)||(type&7)!=3
            ||!gta::read(victim+0x540,health)||!std::isfinite(health)||health<=0)return false;
        lastVictim=victim;before=health;
        // The Minecraft combat calculation already includes armour/enchantments against
        // mobs. Native victims keep GTA's own armour and invulnerability calculation.
        bool hit=damage(reinterpret_cast<void*>(victim),reinterpret_cast<void*>(creator),
            kind==3?5:kind==1?8:0,std::clamp(int(std::ceil(amount*5)),1,500),3,0);
        gta::read(victim+0x540,after);if(hit)++hits;return hit;
    }
    bool init(){
        gta::OriginalImage image;if(!image.load())return false;
        auto f=image.scan("55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 5C 8B 45 10 DB 45 14 53 8B 5D 18 56 57 8B 7D 0C 6A 00 53 50 51 D9 1C 24 57 8D 4D DC");
        auto p=image.scan("55 8B EC 83 EC 14 53 56 8B 35 ? ? ? ? 57 33 FF 89 7D FC 8B 5E 08 33 C9 3B DF 76 34");
        if(f.size()!=1||p.size()!=1)return false;DWORD value{};gta::read(p[0]+10,value);
        poolGlobal=image.global(value);damage=reinterpret_cast<Damage>(image.code(f[0]));supported=true;return true;
    }
    void swing(gta::Vec look,unsigned kind,float strength){
        double n=std::hypot(look.x,look.y);if(!supported||n<.1)return;
        direction={float(look.x/n),float(look.y/n),0};weapon=kind==15?8:kind==10||kind==6?5:0;
        points=std::clamp(int(strength*5),1,100);due=GetTickCount64()+220;
    }
    template<class Solid> bool tick(const gta::Snapshot& s,bool allowed,Solid solid){
        if(!allowed){due=0;return false;}if(!due||GetTickCount64()<due)return false;due=0;
        DWORD pool{},objects{},flags{};int count{};
        if(!gta::read(poolGlobal,pool)||!gta::read(pool,objects)||!gta::read(pool+4,flags)||!gta::read(pool+8,count)||count<1||count>256)return false;
        std::uintptr_t victim{};double closest=2.3*2.3;
        for(int i=0;i<count;++i){BYTE free{},type{};DWORD state{},matrix{},bits{};float health{};gta::Vec p{};
            auto ped=objects+i*0x7c4;if(ped==s.ped||!gta::read(flags+i,free)||(free&128)||!gta::read(ped+0x36,type)||(type&7)!=3
                ||!gta::read(ped+0x540,health)||health<=0||!gta::read(ped+0x530,state)||state>=50
                ||!gta::read(ped+0x46c,bits)||(bits&256)||!gta::read(ped+0x14,matrix)||!gta::read(matrix+0x30,p))continue;
            if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)||!std::isfinite(health))continue;
            double dx=p.x-s.position.x,dy=p.y-s.position.y,dz=p.z-s.position.z,n=std::hypot(dx,dy),distance=dx*dx+dy*dy+dz*dz;
            if(distance>=closest||n<.15||std::abs(dz)>1.2||(dx*direction.x+dy*direction.y)/n<.7)continue;
            bool blocked=false;for(double t=.2;t<1;t+=.1)if(solid(int(std::floor(s.position.x+dx*t)),int(std::floor(s.position.z-0.1+dz*t)),int(std::floor(-s.position.y-dy*t)))){blocked=true;break;}
            if(!blocked){closest=distance;victim=ped;}
        }
        if(!victim)return false;lastVictim=victim;gta::read(victim+0x540,before);
        bool hit=damage(reinterpret_cast<void*>(victim),reinterpret_cast<void*>(s.ped),weapon,points,3,0);
        gta::read(victim+0x540,after);if(hit)++hits;return hit;
    }
};
}
