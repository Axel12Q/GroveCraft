#pragma once
// Pattern knowledge from SilentPatch (MIT, Adrian Zdanowicz), PlayerInfoSA.cpp.
// Layouts from public Plugin-SDK / SilentPatch. Resolve globals from instructions,
// never from hardcoded 1.0 addresses. First integration is read-only.
#include <set>
#include <string>
#include <sstream>
namespace gta {
struct Vec {float x{},y{},z{};};
inline bool accessible(const void* p,std::size_t size,bool writing=false){
    if(!p||size==0)return false;
    MEMORY_BASIC_INFORMATION info{};
    if(!VirtualQuery(p,&info,sizeof info)||info.State!=MEM_COMMIT || (info.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    if(writing && !(info.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))return false;
    auto end=reinterpret_cast<std::uintptr_t>(p)+size;
    return end>=reinterpret_cast<std::uintptr_t>(p) && end<=reinterpret_cast<std::uintptr_t>(info.BaseAddress)+info.RegionSize;
}
template<class T> bool read(std::uintptr_t address,T& out){
    if(!accessible(reinterpret_cast<void*>(address),sizeof out))return false;
    std::memcpy(&out,reinterpret_cast<void*>(address),sizeof out);return true;
}
inline std::vector<std::uintptr_t> scan(const char* text){
    std::vector<int> pattern;std::istringstream tokens(text);std::string s;
    while(tokens>>s)pattern.push_back(s=="?"?-1:std::stoi(s,nullptr,16));
    auto* base=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+dos->e_lfanew);
    std::vector<std::uintptr_t> result;
    for(auto* section=IMAGE_FIRST_SECTION(nt);section<IMAGE_FIRST_SECTION(nt)+nt->FileHeader.NumberOfSections;++section){
        if(!(section->Characteristics&IMAGE_SCN_MEM_EXECUTE))continue;
        auto* bytes=base+section->VirtualAddress;auto length=section->Misc.VirtualSize;
        if(!accessible(bytes,length)||length<pattern.size())continue;
        for(std::size_t i=0;i+pattern.size()<=length;++i){
            bool ok=true;for(std::size_t j=0;j<pattern.size();++j)if(pattern[j]>=0&&bytes[i+j]!=pattern[j]){ok=false;break;}
            if(ok)result.push_back(reinterpret_cast<std::uintptr_t>(bytes+i));
        }
    }
    return result;
}
// Inspect the user's original executable instructions in memory. Other legitimate ASIs
// can replace a function prologue before this plugin starts. Never patch them back.
struct OriginalImage {
    std::vector<unsigned char> bytes;std::uintptr_t originalBase{},loadedBase{};
    bool load(){
        wchar_t path[MAX_PATH];if(!GetModuleFileNameW(nullptr,path,MAX_PATH))return false;
        HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);if(file==INVALID_HANDLE_VALUE)return false;
        DWORD size=GetFileSize(file,nullptr),got{};if(size<0x1000||size>64*1024*1024){CloseHandle(file);return false;}bytes.resize(size);
        bool ok=ReadFile(file,bytes.data(),size,&got,nullptr)&&got==size;CloseHandle(file);if(!ok)return false;
        auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(bytes.data());if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||static_cast<unsigned>(dos->e_lfanew)>size-sizeof(IMAGE_NT_HEADERS))return false;
        auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(bytes.data()+dos->e_lfanew);if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC)return false;
        originalBase=nt->OptionalHeader.ImageBase;loadedBase=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));return true;
    }
    std::uintptr_t global(DWORD address)const{return address-originalBase+loadedBase;}
    std::uintptr_t code(std::uintptr_t instruction)const{
        auto offset=instruction-reinterpret_cast<std::uintptr_t>(bytes.data());
        auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS*>(bytes.data()+dos->e_lfanew);auto sections=IMAGE_FIRST_SECTION(nt);
        for(unsigned n=0;n<nt->FileHeader.NumberOfSections;++n){const auto& sec=sections[n];if(offset>=sec.PointerToRawData&&offset<sec.PointerToRawData+sec.SizeOfRawData)return loadedBase+sec.VirtualAddress+offset-sec.PointerToRawData;}return 0;
    }
    std::vector<std::uintptr_t> scan(const char* text)const{
        std::vector<int> pattern;std::istringstream tokens(text);std::string token;while(tokens>>token)pattern.push_back(token=="?"?-1:std::stoi(token,nullptr,16));
        std::vector<std::uintptr_t> result;if(bytes.empty())return result;
        auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS*>(bytes.data()+dos->e_lfanew);
        auto sections=IMAGE_FIRST_SECTION(nt);for(unsigned n=0;n<nt->FileHeader.NumberOfSections;++n){const auto& section=sections[n];if(!(section.Characteristics&IMAGE_SCN_MEM_EXECUTE)||section.PointerToRawData+section.SizeOfRawData>bytes.size()||section.SizeOfRawData<pattern.size())continue;
            auto data=bytes.data()+section.PointerToRawData;for(std::size_t i=0;i+pattern.size()<=section.SizeOfRawData;++i){bool match=true;for(std::size_t j=0;j<pattern.size();++j)if(pattern[j]>=0&&data[i+j]!=pattern[j]){match=false;break;}if(match)result.push_back(reinterpret_cast<std::uintptr_t>(data+i));}
        }return result;
    }
};
struct Snapshot{
    bool valid{},mission{true},vehicle{},menu{true},grounded{};std::uintptr_t ped{},matrix{};
    Vec position{};float health{},maxHealth{};int state{};
    float hour{12};
};
struct Bindings {
    bool foodSupported{};
    std::uintptr_t players{},focus{},missionOffset{},scriptSpace{},menuManager{},clockHour{},clockMinute{};
    bool init(){
        OriginalImage image;if(!image.load())return false;
        std::set<std::pair<std::uintptr_t,std::uintptr_t>> candidates;
        for(auto a:image.scan("08 85 C0 79 07 0F B6 05 ? ? ? ? 69 C0 90 01 00 00 8B 80 ? ? ? ? F7 80 6C 04 00 00 00 01 00 00")){
            DWORD f{},p{};if(read(a+8,f)&&read(a+20,p))candidates.emplace(f,p);
        }
        if(candidates.size()!=1)return false;
        focus=image.global(candidates.begin()->first);players=image.global(candidates.begin()->second);
        auto mission=image.scan("A1 ? ? ? ? 85 C0 74 0C 83 B8 ? ? ? ? 01 75 03 B0 01 C3");
        if(mission.size()!=1)return false;
        DWORD o{},s{};read(mission[0]+1,o);read(mission[0]+11,s);
        missionOffset=image.global(o);scriptSpace=image.global(s);
        std::set<DWORD> menus;
        for(auto a:image.scan("50 50 68 ? ? ? ? B9 ? ? ? ? E8")){DWORD m{};if(read(a+8,m))menus.insert(m);}
        if(menus.size()==1)menuManager=image.global(*menus.begin());
        // Clock initialization: noon, zero minutes, zero seconds. Confirm adjacent globals.
        auto clocks=image.scan("C6 05 ? ? ? ? 0C A2 ? ? ? ? 66 89 15 ? ? ? ? A2");
        if(clocks.size()==1){DWORD h{},m{},sec{};read(clocks[0]+2,h);read(clocks[0]+8,m);read(clocks[0]+15,sec);
            if(m+1==h&&sec+2==m){clockHour=image.global(h);clockMinute=image.global(m);}}
        // This executable's PlayerInfo hunger increments a ushort at +144, compares 48h,
        // and reads the same resolved clock hour. Fail closed on another executable.
        auto hunger=image.scan("66 89 86 44 01 00 00 66 83 BE 44 01 00 00 30 53 0F 8E ? ? ? ? 38 15 ? ? ? ?");
        if(hunger.size()==1){DWORD hour{};foodSupported=read(hunger[0]+24,hour)&&image.global(hour)==clockHour&&clockHour!=0;}
        return accessible(reinterpret_cast<void*>(players),0x190)&&accessible(reinterpret_cast<void*>(focus),1)
            &&accessible(reinterpret_cast<void*>(missionOffset),4);
    }
    Snapshot sample()const{
        Snapshot s;BYTE player{};DWORD ped{},matrix{},flags{},offset{};
        if(!players||!read(focus,player)||player>1||!read(players+player*0x190,ped)||!accessible(reinterpret_cast<void*>(ped),0x5a0))return s;
        s.ped=ped;
        if(!read(ped+0x14,matrix))return s;
        s.matrix=matrix;
        if(!read(matrix?matrix+0x30:ped+4,s.position)||!read(ped+0x540,s.health)||!read(ped+0x544,s.maxHealth)
            ||!read(ped+0x530,s.state)||!read(ped+0x46c,flags))return s;
        s.vehicle=(flags&(1u<<8))!=0;
        s.grounded=(flags&1)!=0;
        BYTE menu{},hour{},minute{};
        if(menuManager&&read(menuManager+0x5c,menu)&&menu<=1)s.menu=menu!=0;
        if(clockHour&&read(clockHour,hour)&&read(clockMinute,minute)&&hour<24&&minute<60)s.hour=hour+minute/60.0f;
        if(!std::isfinite(s.position.x)||!std::isfinite(s.position.y)||!std::isfinite(s.position.z)
            ||std::abs(s.position.x)>10000||std::abs(s.position.y)>10000||std::abs(s.position.z)>3000
            ||!std::isfinite(s.health)||s.health<0||s.health>1000||s.maxHealth<=0||s.maxHealth>1000||s.state<0||s.state>70)return s;
        if(read(missionOffset,offset)&&offset<200000){BYTE value{};if(offset==0)s.mission=false;else if(read(scriptSpace+offset,value)&&value<=1)s.mission=value!=0;}
        s.valid=true;return s;
    }
    bool money(const Snapshot& s,int& value)const{
        BYTE player{};DWORD ped{};
        if(!s.valid||!players||!read(focus,player)||player>1)return false;
        auto info=players+player*0x190;
        // Same signature-resolved CPlayerInfo layout used by sample/feed.
        return read(info,ped)&&ped==s.ped&&read(info+0xb8,value)&&value>=0&&value<=99999999;
    }
    bool changeMoney(const Snapshot& s,int before,int after)const{
        int current{};BYTE player{};
        if(!s.valid||s.menu||s.vehicle||s.mission||s.health<=0||s.state<=0||s.state>=20
            ||after<0||after>99999999||!money(s,current)||current!=before||!read(focus,player))return false;
        auto address=players+player*0x190+0xb8;
        if(!accessible(reinterpret_cast<void*>(address),4,true))return false;
        std::memcpy(reinterpret_cast<void*>(address),&after,4);return true;
    }
    bool feed(const Snapshot& s)const{
        if(!foodSupported||!s.valid||s.menu||s.vehicle||s.health<=0)return false;
        BYTE player{};DWORD current{};WORD hours{};
        if(!read(focus,player)||player>1)return false;
        auto info=players+player*0x190;
        if(!read(info,current)||current!=s.ped||!read(info+0x144,hours)||hours>720
            ||!accessible(reinterpret_cast<void*>(info+0x144),2,true))return false;
        WORD zero=0;if(hours)std::memcpy(reinterpret_cast<void*>(info+0x144),&zero,2);return true;
    }
    float heal(const Snapshot& s,float amount)const{
        if(!s.valid||s.menu||s.vehicle||s.health<=0||s.state<=0||s.state>=50||amount<=0||amount>1)return 0;
        DWORD current{};BYTE player{};float health{},maximum{};
        if(!read(focus,player)||player>1||!read(players+player*0x190,current)||current!=s.ped
            ||!read(s.ped+0x540,health)||!read(s.ped+0x544,maximum)||health<=0
            ||!std::isfinite(health)||!std::isfinite(maximum)||maximum<=0||maximum>1000
            ||!accessible(reinterpret_cast<void*>(s.ped+0x540),4,true))return 0;
        float next=std::min(maximum,health+amount);
        if(next<=health)return 0;
        std::memcpy(reinterpret_cast<void*>(s.ped+0x540),&next,4);return next-health;
    }
    bool correct(const Snapshot& s,Vec position,unsigned axes,bool grounded)const{
        // Optional laboratory correction on the game/render thread. Revalidate ownership and writable ranges.
        if(!s.valid||s.menu||s.mission||s.vehicle||!s.matrix)return false;
        DWORD current{};if(!read(s.ped+0x14,current)||current!=s.matrix)return false;
        auto* destination=reinterpret_cast<void*>(s.matrix+0x30);
        auto* velocity=reinterpret_cast<void*>(s.ped+0x44); // public CPhysical::m_vecMoveSpeed layout
        if(!accessible(destination,sizeof(Vec),true)||!accessible(velocity,sizeof(Vec),true))return false;
        Vec speed{};read(s.ped+0x44,speed);
        if(!std::isfinite(speed.x)||!std::isfinite(speed.y)||!std::isfinite(speed.z))return false;
        if(axes&1)speed.x=0;if(axes&4)speed.y=0;if(axes&2)speed.z=0;
        std::memcpy(destination,&position,sizeof position);std::memcpy(velocity,&speed,sizeof speed);
        if(grounded){DWORD flags{};if(read(s.ped+0x46c,flags)&&accessible(reinterpret_cast<void*>(s.ped+0x46c),4,true)){
            flags|=1;std::memcpy(reinterpret_cast<void*>(s.ped+0x46c),&flags,4);}}
        return true;
    }
};
}
