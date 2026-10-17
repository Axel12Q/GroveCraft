#pragma once
// Exact-version signature bindings for the rendered hand hierarchy and native input gates.
namespace grove {
class NativeInteraction {
    using Hierarchy=void* (__cdecl*)(void*);
    Hierarchy hierarchy{};std::uintptr_t pad{};bool guiOwned{},fireOwned{},cycleOwned{};
    unsigned short oldDisable{};BYTE oldFire{},oldCycle{};
    std::uintptr_t weaponPlayerData{};bool weaponHidden{},oldRenderWeapon{},weaponVisibilitySupported{};
public:
    std::uintptr_t escapeCode{},menuCheckCode{},escapeState{};
    std::uintptr_t targetCode{},enterTargetCode{};
    bool ownsWeapon()const{return fireOwned||guiOwned;}
    bool supported{};bool handVisible{};gta::Vec hand{};
    bool init(){
        gta::OriginalImage image;if(!image.load())return false;
        auto h=image.scan("55 8B EC 83 EC 0C 80 7D 10 00 56 57 8B F1 74 36 BF 00 04 00 00 85 BE 74 04 00 00");
        auto p=image.scan("55 8B EC 8B 45 08 69 C0 34 01 00 00 05 ? ? ? ? 5D C3");
        if(h.size()==1){int relative{};gta::read(h[0]+45,relative);hierarchy=reinterpret_cast<Hierarchy>(image.code(h[0]+49+relative));}
        if(p.size()==1){DWORD base{};gta::read(p[0]+13,base);pad=image.global(base);}
        auto esc=image.scan("66 83 3D ? ? ? ? 00 74 0D 66 83 3D ? ? ? ? 00 75 03 B0 01 C3 32 C0 C3 CC CC CC CC CC CC 66 83 3D ? ? ? ? 00 74 10");
        if(esc.size()==1){escapeCode=image.code(esc[0]);DWORD address{};gta::read(esc[0]+3,address);escapeState=image.global(address);}
        // Steam's frontend inlines IsEscJustPressed: intercept its enclosing check too.
        auto menu=image.scan("55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 53 33 DB F6 05 ? ? ? ? 01 56 8B F1 75 2F");
        if(menu.size()==1)menuCheckCode=image.code(menu[0]);
        // Exact executable signatures: CPad::GetTarget/GetEnterTargeting.
        auto aim=image.scan("66 83 B9 0E 01 00 00 00 75 27 0F B7 81 0A 01 00 00 85 C0 78 1C 83 F8 02 7E 0E 83 F8 03 75 12 66 83 79 08 00 0F 95 C0 C3 66 83 79 0C 00 0F 95 C0 C3");
        auto enter=image.scan("66 83 B9 0E 01 00 00 00 75 40 0F B7 81 0A 01 00 00 85 C0 78 35 83 F8 02 7E 19 83 F8 03 75 2B 66 83 79 08 00");
        if(aim.size()==1)targetCode=image.code(aim[0]);
        if(enter.size()==1)enterTargetCode=image.code(enter[0]);
        // CPed::Render in this exact Steam build tests CPlayerData +0x34 bit 12.
        weaponVisibilitySupported=image.scan("F7 40 34 00 10 00 00 75 02 32 C9 83 BB F4 04 00 00 00").size()==1;
        supported=hierarchy&&pad;return supported;
    }
    void consumeEscape(){if(escapeState&&gta::accessible(reinterpret_cast<void*>(escapeState),2,true))*reinterpret_cast<unsigned short*>(escapeState)=0;}
    void hideNativeWeapon(const gta::Snapshot& s,bool hide){
        DWORD data{};
        if(!weaponVisibilitySupported||!s.valid||!gta::read(s.ped+0x480,data)||!data
            ||!gta::accessible(reinterpret_cast<void*>(data+0x34),4,true)){
            weaponPlayerData=0;weaponHidden=false;return;
        }
        // Never restore a pointer belonging to an old/dead player instance.
        if(weaponPlayerData!=data){weaponPlayerData=data;weaponHidden=false;}
        auto* flags=reinterpret_cast<DWORD*>(data+0x34);constexpr DWORD render=1u<<12;
        if(hide){if(!weaponHidden){oldRenderWeapon=(*flags&render)!=0;weaponHidden=true;}*flags&=~render;}
        else if(weaponHidden){*flags=(*flags&~render)|(oldRenderWeapon?render:0);weaponHidden=false;}
    }
    void gate(bool gui,bool minecraftItem){
        if(!pad||!gta::accessible(reinterpret_cast<void*>(pad+0x10e),16,true))return;
        auto gateByte=[&](std::uintptr_t address,bool take,bool& owned,BYTE& old){
            auto* value=reinterpret_cast<BYTE*>(address);
            if(take){if(!owned){old=*value;owned=true;}*value=1;}
            else if(owned){if(*value==1)*value=old;owned=false;}
        };
        auto* disabled=reinterpret_cast<unsigned short*>(pad+0x10e);
        if(gui){if(!guiOwned){oldDisable=*disabled;guiOwned=true;}*disabled|=2;}
        else if(guiOwned){*disabled=(*disabled&~2)|(oldDisable&2);guiOwned=false;}
        gateByte(pad+0x11b,minecraftItem||gui,fireOwned,oldFire);
        gateByte(pad+0x11d,minecraftItem||gui,cycleOwned,oldCycle);
    }
    bool matrix(const gta::Snapshot& s,D3DMATRIX& out){
        handVisible=false;if(s.vehicle||s.mission||!boneMatrix(s,24,out))return false;
        hand={out._41,out._42,out._43};handVisible=true;return true;
    }
    bool boneMatrix(const gta::Snapshot& s,int bone,D3DMATRIX& out){
        if(!hierarchy||!s.valid||s.menu)return false;
        DWORD clump{};if(!gta::read(s.ped+0x18,clump)||!gta::accessible(reinterpret_cast<void*>(clump),0x30))return false;
        auto ptr=reinterpret_cast<std::uintptr_t>(hierarchy(reinterpret_cast<void*>(clump)));
        int count{};DWORD nodes{},matrices{};
        if(!ptr||!gta::read(ptr+4,count)||count<1||count>64||!gta::read(ptr+8,matrices)||!gta::read(ptr+16,nodes))return false;
        for(int i=0;i<count;++i){int id{};if(!gta::read(nodes+i*16,id))return false;if(id!=bone)continue;
            struct Matrix {gta::Vec right;DWORD flags;gta::Vec up;DWORD pad1;gta::Vec at;DWORD pad2;gta::Vec pos;DWORD pad3;} m{};
            if(!gta::read(matrices+i*64,m)||!std::isfinite(m.pos.x)||!std::isfinite(m.pos.y)||!std::isfinite(m.pos.z)
                ||std::abs(m.pos.x-s.position.x)+std::abs(m.pos.y-s.position.y)+std::abs(m.pos.z-s.position.z)>4)return false;
            out={};out._11=m.right.x;out._12=m.right.y;out._13=m.right.z;
            out._21=m.up.x;out._22=m.up.y;out._23=m.up.z;
            out._31=m.at.x;out._32=m.at.y;out._33=m.at.z;
            out._41=m.pos.x;out._42=m.pos.y;out._43=m.pos.z;out._44=1;
            return true;
        }return false;
    }
};
}
