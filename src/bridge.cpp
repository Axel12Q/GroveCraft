// GroveCraft experimental host. Uses LibertyCraft/SkyCraft's MIT wire layout.
// GTA globals are resolved from instruction signatures; CJ remains controlled by GTA.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
#include <map>
#include <array>
#include <mutex>
#include <algorithm>
#ifndef GROVE_RENDERER_UNIT
#include "MinHook.h"
#endif
#include "libertycraft_protocol.h"
#include "GameBindings.h"
#include "NativeCash.h"
static grove::NativeCash nativeCash;
#include "NativeCampaign.h"
static grove::NativeCampaign nativeCampaign;
#include "NativeFrameGate.h"
#include "NativeBedSpawn.h"
#include "RenderVisibility.h"
#include "PlayerLifeRules.h"
static grove::FrameGate frameGate;
static HWND processWindow(DWORD pid);
static grove::NativeBedSpawn bedSpawn;
static bool guestSleeping;
static bool nativeHudHooked{},nativeHudObserved{},nativeSprintHooked{},worldRenderedBeforeHud{};
#include "DeviceLease.h"
// Process lifetime: do not call COM Release from a static destructor in DllMain.
static auto* frameDeviceLease=new grove::DeviceLease<IDirect3DDevice9>;
static IDirect3DDevice9* frameDevice;
static bool playerLive;
static ULONGLONG lifeReadyAt;
static std::uintptr_t lifePed;
static gta::Vec lifePosition;
static unsigned lifeTransitions;
static bool deathObserved;
#include "NativeVoxelWorld.h"
#include "NativeActions.h"
#include "NativeInteraction.h"
#include "NativeMelee.h"
#include "NativeDamageGuard.h"
#include "VoxelCollision.h"
#include "BuildTarget.h"
#include "MiningInput.h"
#include "NativeArmor.h"
static grove::NativeArmor nativeArmor;
namespace p = libertycraft::proto;
#include "NativeWater.h"
static grove::NativeWater nativeWater;
#include "NativeExcavation.h"
#include "PlantVisibility.h"
#include "LiningTarget.h"
static grove::LiningTarget liningTarget;
static grove::PlantVisibility plantVisibility;
static LARGE_INTEGER plantClock{};
static double plantPassRayMicros{},plantRayMicros{};
#include "NativeWeather.h"
static grove::NativeWeather nativeWeather;
#include "NativeFelling.h"
#include "ExcavationRender.h"
#include "RenderBatchRules.h"
static grove::NativeExcavation nativeExcavation;
static grove::NativeFelling nativeFelling;
static unsigned publishedFalls;
#include "NativeActors.h"
#include "NativeRoadside.h"
static grove::NativeRoadside nativeRoadside;
static grove::NativeActors nativeActors;
#include "NativeCrime.h"
static grove::NativeCrime nativeCrime;
#include "NativeMobCombat.h"
static grove::NativeMobCombat nativeMobCombat;
#include "NativeReactions.h"
static grove::NativeReactions nativeReactions;
static unsigned char* mem;
static HANDLE mapping;
static HMODULE self;
static FILE* logfile;
static std::mutex logMutex;
static bool cameraValid;
static grove::NativeWorld nativeWorld;
static grove::NativeActions nativeActions;
static grove::NativeInteraction nativeInteraction;
static grove::NativeMelee nativeMelee;
static grove::NativeDamageGuard nativeDamageGuard;
static gta::Bindings bindings;
static gta::Snapshot playerSnapshot;
static ULONGLONG lastTelemetry;
static wchar_t telemetryPath[MAX_PATH];
static std::uint32_t stateSeq;
static IDirect3DTexture9* atlas;
struct Vertex { float x,y,z; DWORD color; float u,v; unsigned light{0xf00},flags{}; };
#include "NativeLighting.h"
static grove::NativeLighting nativeLighting;
struct LitSection {std::uint64_t revision{};std::vector<Vertex> vertices;};
static std::map<std::array<int,3>,LitSection> litSections;
#include "GroveScene.h"
static grove::Scene scene;
#include "GroveArmor.h"
static grove::Armor armor;
static std::map<std::array<int,3>,std::vector<Vertex>> sections;
#include "StreamTransition.h"
static std::map<std::array<int,3>,ULONGLONG> sectionBirth;
static std::set<std::array<int,3>> chamberMeshes;
static std::map<std::array<int,3>,std::pair<gta::Vec,gta::Vec>> sectionBounds;
static std::vector<unsigned char> atlasPixels;
static unsigned atlasW,atlasH;
static bool atlasDirty;
static std::size_t totalVertices;
static constexpr std::size_t maxVertices=1500000; // ~42 MiB, bounded on a 32-bit host.
static std::map<std::array<int,3>,std::array<unsigned char,512>> solidSections;
static bool buildMode;
static unsigned aimSource;
static gta::Vec aimPoint{},aimNormal{};
static bool aimCanMine{},aimAttackPriority{};
static std::uint64_t collisionCorrections;
static unsigned selectedSlot;
static std::uint64_t load64(void* ptr);
static void store64(void* ptr,std::uint64_t value);
#include "NativeTerrain.h"
static grove::NativeTerrain nativeTerrain;
static int foodLevel=-1,itemCount;
static unsigned itemType;
static unsigned hotbarKinds[9];
static int hotbarCounts[9];
static bool hotbarBlocks[9];
static bool guestMenu;
static bool cjPreview;
static float previewLeft{},previewTop{},previewWidth{},previewHeight{};
static ULONGLONG previewStamp{};
static unsigned previewTriangles{},previewArmorTriangles{};
static float menuLeft,menuTop,menuWidth,menuHeight;
static ULONGLONG lastMenu;
static float guiCursorX=400,guiCursorY=300;
static ULONGLONG lastSlotRequest;
static float guestHour;
static ULONGLONG lastVitals,lastHeal,lastFeed;
static bool nativeFoodLinked;
static bool debugHud;
static float regeneratedHealth;
static unsigned starvationHits;
static unsigned sprintDenials;
static ULONGLONG lastStarvation;
static grove::Aim buildTarget;
static float miningProgress;
static bool guestMining,guestEating,hostMining;
static unsigned guestUsing;
static ULONGLONG lastMineSend,lastActionVitals;
static float pendingWeight;
static bool placeAnimation;
static ULONGLONG attackUntil,lastAttackSend;
static bool mealRequested;
static bool armorTestFixture;
static float meleeInterval=.625f;
static void log(const char* msg);
static bool inputEvent(unsigned action,int x,int y,int z);
struct UiHint {unsigned kind{},id{},flags{};gta::Vec point{};ULONGLONG stamp{};};
#include "NativeDeliveryRadar.h"
static grove::NativeDeliveryRadar deliveryRadar;
#include "NativeStructureRadar.h"
static grove::NativeStructureRadar structureRadar;
static UiHint interactionHint,deliveryHint;
static unsigned roadsidePhase{},roadsideProgress{};static ULONGLONG roadsideStamp{};
static ULONGLONG hintAimStamp;
static void consumeEvents(){
    auto* ring=mem+p::kOffEventRing;
    auto head=load64(ring),tail=load64(ring+p::kEventRingTailOff);
    if(head<tail||head-tail>p::kEventRingEntries)return;
    while(tail<head){
        p::McEvent event;std::memcpy(&event,ring+p::kEventRingDataOff+(tail%p::kEventRingEntries)*sizeof event,sizeof event);
        if(event.type==54)structureRadar.receive(event);
        if((event.type==50||event.type==51)&&event.formId<=(event.type==50?5u:2u)&&std::isfinite(event.a+event.b+event.c)
            &&std::abs(event.a)<10000&&std::abs(event.c)<10000&&event.b>=-60&&event.b<=1000){
            auto& hint=event.type==50?interactionHint:deliveryHint;
            hint={event.formId,event.flags,event.flags,{event.a,-event.c,event.b},GetTickCount64()};
        }
        if(event.type==49&&event.formId<=3){nativeActors.vehicles.cargoContents=event.formId;nativeActors.vehicles.contentsStamp=GetTickCount64();}
        if(event.type==53&&event.formId<=3&&std::isfinite(event.a)&&event.a>=0&&event.a<=100){roadsidePhase=event.formId;roadsideProgress=unsigned(event.a);roadsideStamp=GetTickCount64();}
        if(event.type==52&&playerLive&&!guestMenu&&!bedSpawn.active&&std::isfinite(event.a+event.b+event.c)
            &&event.a>=0&&event.a<=0xffffff&&event.b>=0&&event.b<=0xffffff&&event.c>=1&&event.c<=0xfffff
            &&event.a==std::floor(event.a)&&event.b==std::floor(event.b)&&event.c==std::floor(event.c)){
            int result=nativeRoadside.request(nativeActors.vehicles,playerSnapshot,event.formId,unsigned(event.a),unsigned(event.b),unsigned(event.c));
            if(result!=-1)inputEvent(503,int(event.formId),result,int(unsigned(event.a)^unsigned(event.b)));
        }
        if(event.type==48&&playerLive&&!guestMenu&&!bedSpawn.active
            &&std::isfinite(event.a+event.b+event.c)&&event.a>=-5000&&event.a<=5000&&event.a!=0&&event.a==std::floor(event.a)
            &&event.b>=0&&event.b<=0xffffff&&event.b==std::floor(event.b)
            &&event.c>=0&&event.c<=0xffffff&&event.c==std::floor(event.c)){
            int result=nativeCash.request(bindings,playerSnapshot,event.formId,int(event.a),unsigned(event.b),unsigned(event.c));
            if(result!=-1)inputEvent(501,int(event.formId),result,nativeCash.cash);
        }
        if(event.type==46&&playerLive&&event.formId==nativeExcavation.world&&std::isfinite(event.a+event.b+event.c+event.d)
            &&event.d>=1&&event.d<=13&&std::hypot(event.a-playerSnapshot.position.x,event.c+playerSnapshot.position.y)<=192)
            nativeTerrain.requestParcel(event.a,event.c,int(event.d));
        if(event.type==45&&playerLive&&event.formId==nativeExcavation.world
            &&nativeCrime.vandalism(playerSnapshot,nativeActors,{event.a,-event.c,event.b},
                [](gta::Vec a,gta::Vec b){return nativeTerrain.visible(a,b);}))log("Architectural vandalism witnessed: at least one wanted star");
        if((event.type==43||event.type==44)&&event.a>=0&&event.a<=255&&std::isfinite(event.b+event.c+event.d)
            &&std::abs(event.b)<4000&&std::abs(event.d)<4000&&event.c>=-60&&event.c<1000
            &&(event.type==43?nativeFelling.remember(unsigned(event.a),short(event.formId),{event.b,-event.d,event.c})
                :nativeFelling.regrow(unsigned(event.a),short(event.formId),{event.b,-event.d,event.c})))nativeTerrain.invalidate();
        if(event.type==40&&std::isfinite(event.a+event.b+event.c+event.d)&&event.a>=0&&event.a<=30&&event.b>=0&&event.b<=30
            &&event.c>=0&&event.c<=100&&event.d>=0&&event.d<=100){
            nativeArmor.value=event.a;nativeArmor.toughness=event.b;nativeArmor.meleeProtection=event.c;nativeArmor.bulletProtection=event.d;nativeArmor.stamp=GetTickCount64();
        }
        if(event.type==39&&event.formId==1&&std::isfinite(event.a+event.b+event.c+event.d)
            &&event.a>=0&&event.b>=0&&event.c>0&&event.c<800&&event.d>0&&event.d<800){
            cjPreview=true;previewLeft=event.a;previewTop=event.b;previewWidth=event.c;previewHeight=event.d;previewStamp=GetTickCount64();
        }
        if(playerLive&&(event.type==p::kEvHitActor||event.type==p::kEvMobHitPlayer))nativeActors.hit(event,playerSnapshot,nativeMelee);
        if(event.type==p::kEvMob)nativeMobCombat.report(event,playerSnapshot);
        if(playerLive&&event.type==p::kEvExplosion)nativeMobCombat.blast(event,playerSnapshot);
        if(event.type==41&&playerLive&&foodLevel==0&&event.a==1&&event.b>=0&&event.b<=10
            &&GetTickCount64()-lastVitals<2500&&GetTickCount64()-lastStarvation>=3500){
            float current{};if(gta::read(playerSnapshot.ped+0x540,current)&&current>event.b*5
                &&gta::accessible(reinterpret_cast<void*>(playerSnapshot.ped+0x540),4,true)){
                float next=std::max(event.b*5,current-5);std::memcpy(reinterpret_cast<void*>(playerSnapshot.ped+0x540),&next,4);lastStarvation=GetTickCount64();++starvationHits;
            }
        }
        if(event.type==p::kEvSetTime&&std::isfinite(event.a)&&event.a>=0&&event.a<24){
            if(gta::accessible(reinterpret_cast<void*>(bindings.clockHour),1,true)&&gta::accessible(reinterpret_cast<void*>(bindings.clockMinute),1,true)){
                *reinterpret_cast<BYTE*>(bindings.clockHour)=static_cast<BYTE>(event.a);
                *reinterpret_cast<BYTE*>(bindings.clockMinute)=static_cast<BYTE>((event.a-std::floor(event.a))*60);
            }
        }
        if(event.type==32 && event.formId<9 && std::isfinite(event.a)&&event.a>=0&&event.a<=20
           && std::isfinite(event.c)&&event.c>=0&&event.c<=999 && std::isfinite(event.d)&&event.d>=0&&event.d<=24){
            // An older periodic server report must not undo a just-requested slot before its ack.
            if(event.formId==selectedSlot || GetTickCount64()-lastSlotRequest>2000){
                selectedSlot=event.formId;lastSlotRequest=0;
            }
            foodLevel=static_cast<int>(event.a);guestHour=event.d;
            lastVitals=GetTickCount64();
        }
        if(event.type==33&&event.formId<9&&std::isfinite(event.a)&&event.a>=0&&event.a<=999
                &&std::isfinite(event.b)&&event.b>=0&&event.b<=22){
            hotbarCounts[event.formId]=static_cast<int>(event.a);hotbarKinds[event.formId]=static_cast<unsigned>(event.b);
            hotbarBlocks[event.formId]=(event.flags&1)!=0;
        }
        if(event.type==47&&playerLive){
            if(bedSpawn.request(playerSnapshot,event.formId,lifeTransitions+1,deathObserved,
                {event.a,-event.c,event.b+1},nativeExcavation.world)){
                playerSnapshot=bindings.sample();lifePed=playerSnapshot.ped;lifePosition=playerSnapshot.position;
                lifeReadyAt=GetTickCount64()+1000;playerLive=false;nativeTerrain.invalidate();
                log("Bed respawn teleport accepted; awaiting safe ground");
            }
        }
        if(event.type==36){guestSleeping=event.formId==2;guestMenu=event.formId==1||guestSleeping;lastMenu=GetTickCount64();
            if(guestMenu&&std::isfinite(event.a)&&std::isfinite(event.b)&&std::isfinite(event.c)&&std::isfinite(event.d)
                &&event.c>0&&event.c<3840&&event.d>0&&event.d<2160){menuLeft=event.a;menuTop=event.b;menuWidth=event.c;menuHeight=event.d;}
            else guestMenu=false;
        }
        if(event.type==34&&std::isfinite(event.a)&&event.a>=0&&event.a<=1){
            miningProgress=event.a;guestMining=event.b==1;guestEating=event.c==1;lastActionVitals=GetTickCount64();
            guestUsing=event.d==20?20:event.d==21?21:event.d==22?22:0;
        }
        if(event.type==35&&std::isfinite(event.a)&&event.a>=-.1f&&event.a<=40)pendingWeight+=event.a;
        if(event.type==37&&event.formId==selectedSlot)mealRequested=true;
        if(event.type==39&&event.formId==0)placeAnimation=true;
        if(event.type==38&&std::isfinite(event.a)&&event.a>=0&&event.a<=50&&std::isfinite(event.b)&&event.b>=.1&&event.b<=5){meleeInterval=event.b;}
        ++tail;
    }
    store64(ring+p::kEventRingTailOff,tail);
    itemType=hotbarKinds[selectedSlot];itemCount=hotbarCounts[selectedSlot];
}
static bool keyStates[256];
static bool useOwned;
static ULONGLONG lastUseAim;
static bool connected();
static void log(const char* msg);
static bool inputEvent(unsigned action,int x=0,int y=0,int z=0){
    auto* ring=mem+p::kOffInputRing;
    auto head=load64(ring),tail=load64(ring+p::kInputRingTailOff);
    if(head<tail||head-tail>=p::kInputRingEntries)return false;
    p::InputEvent event{32,static_cast<std::uint16_t>(action),x,y,z};
    std::memcpy(ring+p::kInputRingDataOff+(head%p::kInputRingEntries)*sizeof event,&event,sizeof event);
    MemoryBarrier();store64(ring,head+1);return true;
}
static bool rawInput(unsigned type,unsigned code,int a=0,int b=0,int c=0){
    auto* ring=mem+p::kOffInputRing;auto head=load64(ring),tail=load64(ring+p::kInputRingTailOff);
    if(head<tail||head-tail>=p::kInputRingEntries)return false;
    p::InputEvent e{static_cast<std::uint16_t>(type),static_cast<std::uint16_t>(code),a,b,c};
    std::memcpy(ring+p::kInputRingDataOff+(head%p::kInputRingEntries)*sizeof e,&e,sizeof e);MemoryBarrier();store64(ring,head+1);return true;
}
static bool nativeMiningInput(const grove::Aim& cell,const gta::Vec& point){
    // Publish the exact cell and surface point together. Never let the guest
    // mine a new cell using the preceding frame's surface point.
    auto* ring=mem+p::kOffInputRing;
    auto head=load64(ring),tail=load64(ring+p::kInputRingTailOff);
    if(head<tail||head-tail>p::kInputRingEntries-2)return false;
    p::InputEvent events[]={{32,static_cast<std::uint16_t>(7000+cell.face),cell.x,cell.y,cell.z},
        {32,7100,int(point.x*256),int(point.z*256),int(-point.y*256)}};
    for(unsigned i=0;i<2;++i)std::memcpy(ring+p::kInputRingDataOff+((head+i)%p::kInputRingEntries)*sizeof(p::InputEvent),&events[i],sizeof(p::InputEvent));
    MemoryBarrier();store64(ring,head+2);return true;
}
static bool solid(int x,int y,int z){
    auto section=[](int a){return static_cast<int>(std::floor(a/16.0));};
    auto it=solidSections.find({section(x),section(y),section(z)});
    if(it==solidSections.end())return false;
    unsigned bit=(x&15)+16*(z&15)+256*(y&15);
    return (it->second[bit/8]&(1u<<(bit%8)))!=0;
}
static void controls(const D3DMATRIX& v,const D3DMATRIX& projection){
    if(bedSpawn.active){nativeInteraction.gate(true,true);return;}
    DWORD foregroundPid{};GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);
    bool active=foregroundPid==GetCurrentProcessId()&&playerLive;
    guestMenu=guestMenu&&GetTickCount64()-lastMenu<1500;
    // Same checked weapon layout used by NativeReactions on this supported exe.
    // Failed reads leave native fire alone rather than guessing that CJ is unarmed.
    BYTE nativeSlot{};int nativeWeapon=-1;
    bool nativeUnarmed=playerSnapshot.valid&&gta::read(playerSnapshot.ped+0x718,nativeSlot)
        &&nativeSlot<13&&gta::read(playerSnapshot.ped+0x5a0+nativeSlot*28,nativeWeapon)&&nativeWeapon==0;
    bool minecraftOwnsHand=active&&!playerSnapshot.menu&&!playerSnapshot.vehicle&&!playerSnapshot.mission
        &&(itemCount>0||((roadsidePhase==1||roadsidePhase==2)&&GetTickCount64()-roadsideStamp<1500)
            ||(hostMining&&grove::ownsMiningInput(itemCount,nativeUnarmed,keyStates['X'])));
    nativeInteraction.gate(active&&guestMenu&&!playerSnapshot.menu,minecraftOwnsHand);
    nativeInteraction.hideNativeWeapon(playerSnapshot,minecraftOwnsHand);
    bool previousKeys[256];std::memcpy(previousKeys,keyStates,sizeof keyStates);
    bool edges[256]{};
    for(unsigned i=0;i<256;++i){bool down=active&&(GetAsyncKeyState(i)&0x8000);edges[i]=down&&!keyStates[i];keyStates[i]=down;}
    if(!active||!playerSnapshot.valid||playerSnapshot.health<=0||playerSnapshot.state>=54||playerSnapshot.menu||playerSnapshot.vehicle||!connected()){
        if(useOwned&&connected())inputEvent(17);useOwned=false;
        if(hostMining&&connected())inputEvent(9);hostMining=false;buildMode=false;return;
    }
    guestMenu=guestMenu&&GetTickCount64()-lastMenu<1500;
    if(guestMenu){
        if(useOwned)inputEvent(17);useOwned=false;
        if(hostMining)inputEvent(9);hostMining=false;buildMode=false;buildTarget.valid=false;
        if(edges['E'])inputEvent(10);
        RECT rect{};HWND window=GetForegroundWindow();GetClientRect(window,&rect);
        POINT cursor{};GetCursorPos(&cursor);ScreenToClient(window,&cursor);
        guiCursorX=std::clamp(guiCursorX+cursor.x-rect.right/2.f,0.f,float(rect.right-1));
        guiCursorY=std::clamp(guiCursorY+cursor.y-rect.bottom/2.f,0.f,float(rect.bottom-1));
        rawInput(p::kInCursor,0,int(guiCursorX),int(guiCursorY));
        for(auto pair:{std::pair<unsigned,unsigned>{VK_LBUTTON,1},{VK_RBUTTON,3}})
            if(keyStates[pair.first]!=previousKeys[pair.first])rawInput(p::kInMouseButton,pair.second,keyStates[pair.first]?1:0);
        if(keyStates[VK_SHIFT]!=previousKeys[VK_SHIFT])rawInput(p::kInKey,225,keyStates[VK_SHIFT]?1:0);
        for(unsigned i=0;i<9;++i)if(keyStates['1'+i]!=previousKeys['1'+i])rawInput(p::kInKey,30+i,keyStates['1'+i]?1:0);
        POINT center{rect.right/2,rect.bottom/2};ClientToScreen(window,&center);SetCursorPos(center.x,center.y);return;
    }
    for(unsigned i=0;i<9;++i)if(edges['1'+i]){
        selectedSlot=i;lastSlotRequest=GetTickCount64();itemType=hotbarKinds[i];itemCount=hotbarCounts[i];inputEvent(100+i);
    }
    if(edges['R'])inputEvent(4,selectedSlot);
    if(mealRequested){if(!guestMenu)inputEvent(4,selectedSlot);mealRequested=false;}
    if(edges['E']||edges['I']){inputEvent(10);return;}
    if(playerSnapshot.mission){if(hostMining)inputEvent(9);hostMining=false;buildMode=false;buildTarget.valid=false;return;}
    if(armorTestFixture&&edges[VK_F9]){
        ++nativeArmor.fixtureCalls;
        std::uintptr_t source{};nativeActors.eachPed([&](unsigned,std::uintptr_t ped){
            float hp{};if(!source&&ped!=playerSnapshot.ped&&gta::read(ped+0x540,hp)&&hp>0)source=ped;
        });
        // This diagnostic does not depend on proxy lease freshness while a slow frame
        // is surveying terrain. Read the same checked native pool as melee targeting.
        if(!source){DWORD pool{},objects{},freeFlags{};int count{};
            if(gta::read(nativeMelee.pedPool(),pool)&&gta::read(pool,objects)&&gta::read(pool+4,freeFlags)
                &&gta::read(pool+8,count)&&count>0&&count<=256){
                for(int i=0;i<count&&!source;++i){BYTE free{},type{};float hp{};auto ped=std::uintptr_t(objects)+i*0x7c4;
                    if(ped!=playerSnapshot.ped&&gta::read(freeFlags+i,free)&&!(free&128)&&gta::read(ped+0x36,type)
                        &&(type&7)==3&&gta::read(ped+0x540,hp)&&hp>0)source=ped;
                }
            }
        }
        nativeArmor.fixtureSource=source;if(source)nativeArmor.fixtureApplied=nativeMelee.apply(playerSnapshot.ped,source,4,3);
    }
    if((edges['G']||edges['B'])&&nativeActors.vehicles.cargoCandidate&&GetTickCount64()-nativeActors.vehicles.cargoStamp<300){
        if(edges['B']&&keyStates[VK_MENU]){inputEvent(30,int(nativeActors.vehicles.cargoCandidate));return;}
        if(hostMining)inputEvent(9);hostMining=false;
        nativeActors.vehicles.toggleTrunk(playerSnapshot,edges['B']);
        if(!edges['B'])inputEvent(keyStates[VK_MENU]?26:18,int(nativeActors.vehicles.cargoCandidate));return;
    }
    if(edges['J']&&interactionHint.kind&&GetTickCount64()-interactionHint.stamp<700){
        if(interactionHint.kind==1)inputEvent(28,int(interactionHint.id));
        else inputEvent(29,int(std::floor(interactionHint.point.x)),int(std::floor(interactionHint.point.z)),int(std::floor(-interactionHint.point.y)));
        return;
    }
    if((roadsidePhase==1||roadsidePhase==2)&&GetTickCount64()-roadsideStamp<1500){
        if(useOwned)inputEvent(17);useOwned=false;
        if(hostMining)inputEvent(9);hostMining=false;buildMode=false;buildTarget.valid=false;
        nativeInteraction.gate(false,true);return; // movement/pause remain GTA-owned
    }
    buildMode=true;
    bool sustainedUse=itemType>=20&&itemType<=22;
    bool attackIntent=itemType==15||sustainedUse;
    nativeMobCombat.each([&](unsigned,unsigned,unsigned,gta::Vec pos,float height){
        double dx=pos.x-playerSnapshot.position.x,dy=pos.y-playerSnapshot.position.y,n=std::hypot(dx,dy);
        if(n<3&&std::abs(pos.z-playerSnapshot.position.z)<std::max(1.5f,height)
            &&(n<.15||(dx*v._13+dy*v._23)/n>.65))attackIntent=true;
    });
    if(itemCount>0&&(itemType==10||itemType==6))nativeActors.eachPed([&](unsigned,std::uintptr_t ped){
        DWORD matrix{};gta::Vec pos{};BYTE created{};float health{};
        if(!gta::read(ped+0x484,created)||created!=1||!gta::read(ped+0x540,health)||health<=0||!gta::read(ped+0x14,matrix)||!gta::read(matrix+0x30,pos))return;
        double dx=pos.x-playerSnapshot.position.x,dy=pos.y-playerSnapshot.position.y,n=std::hypot(dx,dy);
        if(n>.15&&n<3&&std::abs(pos.z-playerSnapshot.position.z)<1.5&&(dx*v._13+dy*v._23)/n>.65)attackIntent=true;
    });
    // Native collision is automatic, including outside build mode; F10 correction is retired.
    if(edges[VK_F8])inputEvent(6);
    if(edges[VK_F7])debugHud=!debugHud;
    if(edges[VK_F6])inputEvent(5);
    double cx=-(v._41*v._11+v._42*v._12+v._43*v._13);
    double cy=-(v._41*v._21+v._42*v._22+v._43*v._23);
    double cz=-(v._41*v._31+v._42*v._32+v._43*v._33);
    // Reticle beside CJ (65% width, 55% height), unprojected through the actual GTA camera.
    double rx=.30/projection._11,ry=-.10/projection._22;
    double dx=v._13+rx*v._11+ry*v._12,dy=v._23+rx*v._21+ry*v._22,dz=v._33+rx*v._31+ry*v._32;
    buildTarget=grove::aim({cx,cz,-cy},{dx,dz,-dy},
        {playerSnapshot.position.x,playerSnapshot.position.z-1,-playerSnapshot.position.y},[](int x,int y,int z){return solid(x,y,z)||(itemType==17&&nativeWater.source(x,y,z));});
    aimSource=buildTarget.hit?1:0;aimPoint={};aimNormal={};
    gta::Vec nativeTarget{},nativeNormal{};bool terrainTarget=false,nativeBoundary=false;unsigned nativeWood{};
    if(nativeTerrain.target({float(cx),float(cy),float(cz)},
        {float(cx+dx*12),float(cy+dy*12),float(cz+dz*12)},nativeTarget,nativeNormal,&nativeWood,&nativeBoundary)){
        double reach=std::hypot(nativeTarget.x-playerSnapshot.position.x,nativeTarget.y-playerSnapshot.position.y);
        double nativeDistance=std::sqrt((nativeTarget.x-cx)*(nativeTarget.x-cx)+(nativeTarget.y-cy)*(nativeTarget.y-cy)+(nativeTarget.z-cz)*(nativeTarget.z-cz));
        if(reach<6&&std::abs(nativeTarget.z-playerSnapshot.position.z)<6&&grove::nativePrecedes(nativeDistance,buildTarget)){
            terrainTarget=true;buildTarget.hit=true;buildTarget.valid=true;buildTarget.distance=nativeDistance;
            aimSource=nativeBoundary?3:nativeWood?4:2;aimPoint=nativeTarget;aimNormal=nativeNormal;
            buildTarget.x=int(std::floor(nativeTarget.x-nativeNormal.x*.01f));
            buildTarget.y=int(std::floor(nativeTarget.z-nativeNormal.z*.01f));
            buildTarget.z=int(std::floor(-nativeTarget.y+nativeNormal.y*.01f));
            if(!nativeWood)nativeTerrain.requestSurface(buildTarget.x,buildTarget.y,buildTarget.z);
            float ax=std::abs(nativeNormal.x),ay=std::abs(nativeNormal.y),az=std::abs(nativeNormal.z);
            buildTarget.face=az>=ax&&az>=ay?(nativeNormal.z>0?1:0):ax>=ay?(nativeNormal.x>0?5:4):(nativeNormal.y>0?2:3);
            buildTarget.px=buildTarget.x;buildTarget.py=buildTarget.y;buildTarget.pz=buildTarget.z;
            if(buildTarget.face<2)buildTarget.py+=buildTarget.face?1:-1;else if(buildTarget.face<4)buildTarget.pz+=buildTarget.face==2?-1:1;else buildTarget.px+=buildTarget.face==4?-1:1;
        }
    }
    auto lining=liningTarget.ray({cx,cz,-cy},{dx,dz,-dy},12,[](auto cell){
        return nativeExcavation.terrainBoundary(cell[0],cell[1],cell[2]);
    });
    if(lining.valid&&grove::nativePrecedes(lining.distance,buildTarget)
        &&std::hypot(lining.point[0]-playerSnapshot.position.x,-lining.point[2]-playerSnapshot.position.y)<6
        &&std::abs(lining.point[1]-playerSnapshot.position.z)<6){
        // Pick the exact face already on screen. Below the original surface,
        // the virtual soil wall has no stock GTA triangle for CWorld to hit.
        terrainTarget=true;nativeBoundary=false;nativeWood=0;aimSource=5;
        buildTarget.hit=buildTarget.valid=true;buildTarget.distance=lining.distance;
        buildTarget.x=lining.cell[0];buildTarget.y=lining.cell[1];buildTarget.z=lining.cell[2];buildTarget.face=lining.face;
        buildTarget.px=buildTarget.x;buildTarget.py=buildTarget.y;buildTarget.pz=buildTarget.z;
        if(lining.face<2)buildTarget.py+=lining.face?1:-1;else if(lining.face<4)buildTarget.pz+=lining.face==2?-1:1;else buildTarget.px+=lining.face==4?-1:1;
        nativeTarget={float(lining.point[0]),float(-lining.point[2]),float(lining.point[1])};aimPoint=nativeTarget;
        aimNormal={};if(lining.face<2)aimNormal.z=lining.face?1.f:-1.f;
        else if(lining.face<4)aimNormal.y=lining.face==2?1.f:-1.f;else aimNormal.x=lining.face==4?-1.f:1.f;
        nativeTerrain.requestSurface(buildTarget.x,buildTarget.y,buildTarget.z);
    }
    if(GetTickCount64()-hintAimStamp>=150){
        double n=std::sqrt(dx*dx+dy*dy+dz*dz);
        inputEvent(14,int((cx+dx/n*12)*256),int((cz+dz/n*12)*256),int(-(cy+dy/n*12)*256));hintAimStamp=GetTickCount64();
    }
    bool rightPress=edges['B']||edges[VK_RBUTTON];
    if(rightPress||(useOwned&&keyStates[VK_RBUTTON]&&GetTickCount64()-lastUseAim>=75)){
        double n=std::sqrt(dx*dx+dy*dy+dz*dz);
        double ax=cx+dx/n*12,ay=cz+dz/n*12,az=-(cy+dy/n*12);
        gta::Vec nativeUseAim{};
        if(nativeTerrain.aim({float(cx),float(cy),float(cz)},{float(ax),float(-az),float(ay)},playerSnapshot,nativeUseAim)){
            ax=nativeUseAim.x;ay=nativeUseAim.z;az=-nativeUseAim.y;
        }else if(buildTarget.hit){ax=buildTarget.x+.5;ay=buildTarget.y+.5;az=buildTarget.z+.5;}
        inputEvent(14,int(ax*256),int(ay*256),int(az*256));lastUseAim=GetTickCount64();
    }
    if(rightPress){
        inputEvent(buildTarget.valid?(keyStates[VK_SHIFT]?410:400)+buildTarget.face:(keyStates[VK_SHIFT]?25:15),buildTarget.x,buildTarget.y,buildTarget.z);
        placeAnimation=hotbarBlocks[selectedSlot];useOwned=sustainedUse;
    }
    if(useOwned&&!keyStates[VK_RBUTTON]){inputEvent(16);useOwned=false;}
    bool canMine=!guestEating&&grove::ownsMiningInput(itemCount,nativeUnarmed,keyStates['X']);
    aimCanMine=canMine;aimAttackPriority=attackIntent;
    nativeInteraction.gate(false,itemCount>0||(canMine&&!attackIntent&&!useOwned
        &&buildTarget.valid&&buildTarget.hit&&(keyStates['X']||keyStates[VK_LBUTTON])));
    if(buildTarget.valid){
        if(canMine&&!attackIntent&&!useOwned&&(keyStates['X']||keyStates[VK_LBUTTON])&&buildTarget.hit&&GetTickCount64()-lastMineSend>=75){
            if(terrainTarget&&!nativeBoundary){
                if(nativeWood)inputEvent(6000+buildTarget.face*64+int(nativeWood),int(nativeTarget.x*256),int(nativeTarget.z*256),int(-nativeTarget.y*256));
                else nativeMiningInput(buildTarget,nativeTarget);
            }
            else inputEvent(2,buildTarget.x,buildTarget.y,buildTarget.z);
            lastMineSend=GetTickCount64();hostMining=true;
        }
    }
    if(hostMining&&(useOwned||attackIntent||!canMine||(!keyStates['X']&&!keyStates[VK_LBUTTON])||!buildTarget.valid||!buildTarget.hit)){inputEvent(9);hostMining=false;}
    if(!useOwned&&!guestEating&&edges[VK_LBUTTON]&&(attackIntent||!(buildTarget.valid&&buildTarget.hit))&&GetTickCount64()-lastAttackSend>=500){
        float strength=std::clamp(float(GetTickCount64()-lastAttackSend)/(meleeInterval*1000),0.f,1.f);
        // An empty slot leaves GTA's own unarmed/weapon attack active. Do not
        // also enqueue our damage event or replace its native animation.
        (void)strength; // Minecraft chooses one target among mobs and the native NPC proxies.
        inputEvent(12);attackUntil=GetTickCount64()+600;lastAttackSend=GetTickCount64();
    }
}
static void survival(){
    auto now=GetTickCount64();
    bool safe=connected()&&playerLive&&!playerSnapshot.menu&&!playerSnapshot.vehicle;
    if(safe&&now-lastVitals<=2500&&foodLevel>0&&now-lastFeed>=1000){
        lastFeed=now;nativeFoodLinked=bindings.feed(playerSnapshot);
    }
    if(!safe||now-lastVitals>2500||foodLevel!=20){lastHeal=now;return;}
    if(now-lastHeal>=1000){
        // Minecraft food is authoritative; heal actual CJ gradually, never resurrect.
        lastHeal=now;
        float amount=bindings.heal(playerSnapshot,1.0f);
        regeneratedHealth+=amount;playerSnapshot.health+=amount;
        if(amount>0)inputEvent(8); // pay vanilla hunger exhaustion for regeneration
    }
}
#ifndef GROVE_RENDERER_UNIT
using EndFn = HRESULT (WINAPI*)(IDirect3DDevice9*);
using CampaignSaveFn=unsigned (__thiscall*)(void*,int);
using CampaignLoadFn=bool (__cdecl*)(bool*);
using CampaignResetFn=void (__cdecl*)();
static CampaignSaveFn originalCampaignSave;static CampaignLoadFn originalCampaignLoad;static CampaignResetFn originalCampaignReset;
static void clearCampaignState(){
    nativeInteraction.gate(false,false);useOwned=hostMining=guestEating=false;guestUsing=0;guestMenu=guestSleeping=false;
    nativeWorld.clear();nativeTerrain.invalidate();nativeExcavation.clear();nativeWater.clear();nativeFelling.resetDevice();
    sections.clear();sectionBirth.clear();litSections.clear();chamberMeshes.clear();sectionBounds.clear();solidSections.clear();liningTarget.clear();totalVertices=0;scene.clear();armor.clear();
    deathObserved=false;lifePed=0;playerLive=false;lifeReadyAt=GetTickCount64()+500;bedSpawn.active=false;
}
static void __cdecl pairedCampaignReset(){
    if(nativeCampaign.enabled){clearCampaignState();try{nativeCampaign.newGame();}catch(...){log("Cannot create isolated campaign; previous worlds preserved");nativeCampaign.blocked=true;}}
    nativeActors.vehicles.invalidateLeases();originalCampaignReset();
}
static unsigned __fastcall pairedCampaignSave(void* helper,void*,int slot){
    if(!nativeCampaign.enabled)return originalCampaignSave(helper,slot);
    if(!nativeCampaign.supported||nativeCampaign.blocked){log("Paired save refused: campaign unavailable or still switching");return 2;}
    auto file=nativeCampaign.savePath(slot);grove::NativeCampaign::Pair plan{};
    try{
        if(!nativeCampaign.prepareSave(file,plan)){nativeCampaign.resume();log("Paired save refused: Minecraft snapshot not ready");return 2;}
        unsigned result=originalCampaignSave(helper,slot)&255;
        if(result==0&&nativeCampaign.commit(file,plan)){nativeCampaign.resume();log("GTA and Minecraft save committed together");return 0;}
        nativeCampaign.recover(file);nativeCampaign.resume();log("Paired save rolled back; previous slot preserved");return 2;
    }catch(...){try{if(!file.empty())nativeCampaign.recover(file);nativeCampaign.resume();}catch(...){}log("Paired save IO failure; recovery journal preserved");return 2;}
}
static bool __cdecl pairedCampaignLoad(bool* variables){
    if(!nativeCampaign.enabled)return originalCampaignLoad(variables);
    if(!nativeCampaign.supported){if(variables)*variables=false;return false;}
    try{
        grove::NativeCampaign::Pair pair{};bool linked{};
        if(!nativeCampaign.beforeLoad(nativeCampaign.loadPath(),pair,linked)){log("Paired load refused: missing or inconsistent snapshot");if(variables)*variables=false;return false;}
        if(linked&&!nativeCampaign.verifySnapshot(pair)){nativeCampaign.resume();log("GTA load refused before mutation: Minecraft checkpoint failed verification");if(variables)*variables=false;return false;}
        bool ok=originalCampaignLoad(variables);if(ok){nativeActors.vehicles.invalidateLeases();nativeCampaign.loaded(pair,linked);nativeCash.setPrefix(nativeCampaign.walletPrefix());nativeRoadside.setPrefix(nativeCampaign.walletPrefix());log("GTA loaded: selecting its personal Minecraft checkpoint");}return ok;
    }catch(...){log("Paired load IO failure; all previous worlds preserved");if(variables)*variables=false;return false;}
}
using EscapeFn=bool (__cdecl*)();
static EscapeFn originalEscape;
using MenuCheckFn=void (__thiscall*)(void*);
static MenuCheckFn originalMenuCheck;
static void __fastcall interceptedMenuCheck(void* menu,void*){
    if(nativeCampaign.testMode&&playerSnapshot.valid&&playerSnapshot.menu&&!playerSnapshot.mission&&!nativeCampaign.blocked){
        auto command=nativeCampaign.root/L"test-save.txt";
        if(std::filesystem::exists(command)){
            std::ifstream in(command);std::string value;std::getline(in,value);in.close();std::filesystem::remove(command);
            if(value=="save8"||value=="save7"){unsigned result=pairedCampaignSave(nullptr,nullptr,value=="save8"?7:6);std::string text=std::to_string(result)+"\n";
                grove::NativeCampaign::atomic(nativeCampaign.root/L"test-save-result.txt",text.data(),DWORD(text.size()));}
        }
    }
    static bool closing;
    bool down=(GetAsyncKeyState(VK_ESCAPE)&0x8000)!=0;
    if(!down)closing=false;
    if(down&&guestMenu&&!playerSnapshot.menu&&!closing){inputEvent(10);closing=true;}
    // Keep consuming the held key after the guest acknowledges closure.
    if(closing)nativeInteraction.consumeEscape();
    originalMenuCheck(menu);
}
static bool interceptedEscape(){
    static bool consumed;bool pressed=originalEscape();
    if(!(GetAsyncKeyState(VK_ESCAPE)&0x8000))consumed=false;
    if(guestMenu&&!playerSnapshot.menu&&pressed){if(!consumed){inputEvent(10);consumed=true;}return false;}
    return pressed;
}
static WNDPROC previousWindowProc;static HWND inputWindow;static LONG wheelPending;
static LRESULT CALLBACK minecraftWindowProc(HWND w,UINT message,WPARAM wp,LPARAM lp){
    if(message==WM_MOUSEWHEEL&&connected()&&playerSnapshot.valid&&!playerSnapshot.menu&&!playerSnapshot.vehicle&&!playerSnapshot.mission){
        if(guestMenu)rawInput(p::kInScroll,0,GET_WHEEL_DELTA_WPARAM(wp));
        else InterlockedExchangeAdd(&wheelPending,GET_WHEEL_DELTA_WPARAM(wp));return 0;
    }
    return CallWindowProcW(previousWindowProc,w,message,wp,lp);
}
static void windowInput(){
    DWORD pid{};HWND w=GetForegroundWindow();GetWindowThreadProcessId(w,&pid);
    if(!inputWindow&&pid==GetCurrentProcessId()){
        previousWindowProc=reinterpret_cast<WNDPROC>(GetWindowLongPtrW(w,GWLP_WNDPROC));
        if(previousWindowProc&&SetWindowLongPtrW(w,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(minecraftWindowProc)))inputWindow=w;
    }
    auto amount=InterlockedExchange(&wheelPending,0);
    if(amount&&!guestMenu){int step=amount>0?-1:1;selectedSlot=(selectedSlot+9+step)%9;lastSlotRequest=GetTickCount64();inputEvent(100+selectedSlot);}
}
using ResetFn = HRESULT (WINAPI*)(IDirect3DDevice9*,D3DPRESENT_PARAMETERS*);
static EndFn originalEnd;
static ResetFn originalReset;
using TargetFn=bool (__thiscall*)(void*);
static TargetFn originalTarget,originalEnterTarget;
static bool __fastcall ownedTarget(void* pad,void*){return !nativeInteraction.ownsWeapon()&&originalTarget(pad);}
static bool __fastcall ownedEnterTarget(void* pad,void*){return !nativeInteraction.ownsWeapon()&&originalEnterTarget(pad);}
#endif
static void log(const char* msg) {
    std::lock_guard<std::mutex> lock(logMutex);
    if(logfile){std::fprintf(logfile,"[%llu] %s\n",GetTickCount64(),msg);std::fflush(logfile);}
}
static std::uint64_t load64(void* ptr) {
    return InterlockedCompareExchange64(static_cast<volatile LONG64*>(ptr),0,0);
}
static void store64(void* ptr,std::uint64_t value) {
    InterlockedExchange64(static_cast<volatile LONG64*>(ptr),value);
}
static bool connected() {
    const auto now=GetTickCount64();
    const auto stamp=load64(mem+24);
    return *reinterpret_cast<volatile DWORD*>(mem+12)!=0 && stamp<=now && now-stamp<3000;
}
static void publishState(IDirect3DDevice9* dev,const D3DMATRIX& v,bool valid) {
    store64(mem+0x28,GetTickCount64()); // render-thread freshness; independent of transport heartbeat
    p::SkyState state{};
    state.flags=valid && playerSnapshot.valid ? (p::kSkyInGame|p::kSkyHostDrives) : (p::kSkyLoading|p::kSkyMenuOpen);
    if(nativeCampaign.enabled&&nativeCampaign.blocked)state.flags|=p::kSkyLoading;
    if(!playerLive)state.flags|=1u<<21; // host death/respawn transition, retain the frozen CJ mirror
    if(deathObserved)state.flags|=1u<<22;
    state.collisionEpoch=1; state.teleportSeq=lifeTransitions+1;
    state.gameHour=playerSnapshot.hour;
    if(playerSnapshot.menu)state.flags|=p::kSkyMenuOpen;
    if(guestMenu&&!guestSleeping)state.flags|=1u<<19;
    if(guestSleeping&&guestMenu)state.flags|=1u<<23;
    if(bedSpawn.active)state.flags|=1u<<24;
    if(bedSpawn.failed())state.flags|=1u<<25;
    if((roadsidePhase==1||roadsidePhase==2)&&GetTickCount64()-roadsideStamp<1500)state.flags|=1u<<26;
    D3DVIEWPORT9 vp{};dev->GetViewport(&vp);
    state.viewportW=std::min<DWORD>(vp.Width,1280);state.viewportH=std::min<DWORD>(vp.Height,720);
    // Inverse of a rigid row-vector view matrix. Diagnostic camera position only.
    double x=-(v._41*v._11+v._42*v._12+v._43*v._13);
    double y=-(v._41*v._21+v._42*v._22+v._43*v._23);
    double z=-(v._41*v._31+v._42*v._32+v._43*v._33);
    state.posX=x;state.posY=z-1.6;state.posZ=-y;
    if(playerSnapshot.valid){
        state.posX=playerSnapshot.position.x;state.posY=playerSnapshot.position.z-1.0;state.posZ=-playerSnapshot.position.y;
        if(playerSnapshot.vehicle)state.flags|=p::kSkyInVehicle;
        // Missions are not cutscenes. Keep the guest simulation running, but let
        // our gameplay extension distinguish mission time from free-roam.
        if(playerSnapshot.mission)state.flags|=1u<<16;
        if(playerSnapshot.grounded)state.flags|=1u<<17;
        // GTA's configured default sprint is Space. Only movement contributes exhaustion.
        if(keyStates[VK_SPACE])state.flags|=1u<<18;
    }
    state.yaw=static_cast<float>(std::atan2(-v._13,-v._23)*180.0/3.141592653589793);
    state.pitch=static_cast<float>(std::asin(std::clamp(-v._33,-1.0f,1.0f))*180.0/3.141592653589793);
    auto* out=reinterpret_cast<p::SkyState*>(mem+p::kOffSkyState);
    state.collisionEpoch=nativeTerrain.epoch();
    BYTE area{};if(playerSnapshot.valid&&gta::read(playerSnapshot.ped+0x2f,area))state.worldId=area;
    InterlockedExchange(reinterpret_cast<volatile LONG*>(&out->seq),++stateSeq);
    std::memcpy(reinterpret_cast<char*>(out)+4,reinterpret_cast<char*>(&state)+4,sizeof(state)-4);
    MemoryBarrier();InterlockedExchange(reinterpret_cast<volatile LONG*>(&out->seq),++stateSeq);
}
static void consumeRender() {
    auto* ring=mem+p::kOffRenderRing;
    const auto head=load64(ring);
    auto tail=load64(ring+p::kRenRingTailOff);
    constexpr auto capacity=p::kRenRingDataBytes;
    if(head<tail || head-tail>capacity){log("Invalid render counters; waiting for restart");return;}
    unsigned messages=0,batchRemaining=0;
    const auto started=GetTickCount64();
    while(tail<head && (batchRemaining||(messages<32&&GetTickCount64()-started<4))){
        const auto offset=tail%capacity;
        const auto remain=capacity-offset;
        if(remain<8){tail+=remain;continue;}
        auto* message=ring+p::kRenRingDataOff+offset;
        std::uint32_t type,bytes;std::memcpy(&type,message,4);std::memcpy(&bytes,message+4,4);
        if(type==p::kRenPad){tail+=remain;continue;}
        std::uint64_t total=(8ull+bytes+7)&~7ull;
        if(total>remain||total>head-tail){log("Invalid render message; consumer stopped");break;}
        const auto* body=message+8;
        ++messages;
        if(type==grove::renderBatchBegin){
            unsigned count{};if(bytes==4)std::memcpy(&count,body,4);
            if(batchRemaining||bytes!=4||!grove::completeRenderBatch(ring+p::kRenRingDataOff,capacity,tail+total,head,count)){
                log("Incomplete render transaction; consumer stopped");break;
            }
            batchRemaining=count+1;tail+=total;continue;
        }
        if(batchRemaining)--batchRemaining;
        if(type==grove::renderBatchEnd){tail+=total;continue;}
        if(type==p::kRenClearAll){sections.clear();sectionBirth.clear();litSections.clear();chamberMeshes.clear();sectionBounds.clear();solidSections.clear();liningTarget.clear();totalVertices=0;scene.clear();armor.clear();nativeExcavation.clear();nativeWater.clear();nativeFelling.resetDevice();}
        else if(type==p::kRenRagdoll){if(!armor.frame(body,bytes))++armor.rejected;}
        else if(type==p::kRenLiquids){if(!nativeWater.packet(body,bytes))++nativeWater.rejected;}
        else if(type==p::kRenDug){
            if(!nativeExcavation.frame(body,bytes))++nativeExcavation.rejectedDraws;
            // Cuts change masks, not original sampled geometry. Clearing the
            // entire collision epoch here caused arrival delays and repeated
            // transparent refreshes after every adjacent wall cut.
            }
        else if(type==p::kRenTexture){if(!scene.texture(body,bytes))++scene.rejected;}
        else if(type==p::kRenScene){if(!scene.frame(body,bytes))++scene.rejected;}
        else if(type==p::kRenSolids && bytes>=sizeof(p::RenSolids)){
            p::RenSolids h;std::memcpy(&h,body,sizeof h);
            std::array<int,3> key={h.sx,h.sy,h.sz};
            if(!h.count && bytes==sizeof h)solidSections.erase(key);
            else if(h.count<=4096 && bytes==sizeof(h)+512 && (solidSections.size()<512||solidSections.count(key)))
                std::memcpy(solidSections[key].data(),body+sizeof h,512);
        }
        else if(type==p::kRenAtlas && bytes>=sizeof(p::RenAtlas)){
            p::RenAtlas h;std::memcpy(&h,body,sizeof h);
            if(h.width && h.height && h.width<=4096 && h.height<=4096 &&
               8ull+4ull*h.width*h.height==bytes){
                atlasW=h.width;atlasH=h.height;atlasPixels.assign(body+8,body+bytes);atlasDirty=true;
                log("Received Minecraft texture atlas");
            }
        }else if(type==p::kRenSection && bytes>=sizeof(p::RenSection)){
            p::RenSection h;std::memcpy(&h,body,sizeof h);
            // Reject incomplete replacements without retiring the last valid mesh.
            if(h.vertexCount<=300000 && h.vertexCount%3==0 && sizeof(h)+std::uint64_t(h.vertexCount)*sizeof(p::RenVertex)==bytes){
                std::array<int,3> key={h.sx,h.sy,h.sz};
                litSections.erase(key);
                auto found=sections.find(key);auto oldCount=found==sections.end()?0:found->second.size();
                if(!h.vertexCount){totalVertices-=oldCount;sections.erase(key);sectionBirth.erase(key);sectionBounds.erase(key);chamberMeshes.erase(key);liningTarget.erase(key);}
                else if((sections.size()<384 || found!=sections.end()) && totalVertices-oldCount+h.vertexCount<=maxVertices
                        && std::abs(static_cast<long long>(h.sx))<1000 && std::abs(static_cast<long long>(h.sy))<1000 && std::abs(static_cast<long long>(h.sz))<1000){
                    bool finite=true;
                    for(unsigned i=0;i<h.vertexCount && finite;++i){
                        p::RenVertex v;std::memcpy(&v,body+sizeof(h)+i*sizeof(v),sizeof v);
                        finite=std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z)&&std::isfinite(v.u)&&std::isfinite(v.v);
                    }
                    if(!finite){tail+=total;continue;}
                    sectionBirth.emplace(key,GetTickCount64());
                    auto& list=sections[key];list.resize(h.vertexCount);
                    totalVertices=totalVertices-oldCount+h.vertexCount;
                    gta::Vec lo{1e9f,1e9f,1e9f},hi{-1e9f,-1e9f,-1e9f};
                    for(unsigned i=0;i<h.vertexCount;++i){
                        p::RenVertex v;std::memcpy(&v,body+sizeof(h)+i*sizeof(v),sizeof v);
                        DWORD c=(v.color&0xff00ff00)|((v.color&0xff)<<16)|((v.color>>16)&0xff);
                        list[i]={v.x+h.sx*16.0f,-(v.z+h.sz*16.0f),v.y+h.sy*16.0f,c,v.u,v.v,v.light,v.flags};
                        auto n=list[i];lo={std::min(lo.x,n.x),std::min(lo.y,n.y),std::min(lo.z,n.z)};hi={std::max(hi.x,n.x),std::max(hi.y,n.y),std::max(hi.z,n.z)};
                    }
                    sectionBounds[key]={lo,hi};
                    liningTarget.replace(key,list);
                    chamberMeshes.insert(key);
                }
            }
        }
        tail+=total;
    }
    store64(ring+p::kRenRingTailOff,tail);
}
#include "GroveWorldItems.h"
static void draw(IDirect3DDevice9* dev) {
    if(atlasDirty){
        if(atlas){atlas->Release();atlas=nullptr;}
        if(SUCCEEDED(dev->CreateTexture(atlasW,atlasH,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&atlas,nullptr))){
            D3DLOCKED_RECT rect{};
            if(SUCCEEDED(atlas->LockRect(0,&rect,nullptr,0))){
                for(unsigned y=0;y<atlasH;y++)for(unsigned x=0;x<atlasW;x++){
                    auto* dst=static_cast<unsigned char*>(rect.pBits)+y*rect.Pitch+x*4;
                    const auto* src=atlasPixels.data()+(std::size_t(y)*atlasW+x)*4;
                    dst[0]=src[2];dst[1]=src[1];dst[2]=src[0];dst[3]=src[3];
                }
                atlas->UnlockRect(0);atlasDirty=false;
            }
        }
    }
    if(!atlas)return;
    IDirect3DStateBlock9* saved=nullptr;
    if(FAILED(dev->CreateStateBlock(D3DSBT_ALL,&saved)))return;
    D3DMATRIX world{};world._11=world._22=world._33=world._44=1;
    D3DMATRIX oldWorld{};dev->GetTransform(D3DTS_WORLD,&oldWorld);
    dev->SetTransform(D3DTS_WORLD,&world);
    dev->SetVertexShader(nullptr);dev->SetPixelShader(nullptr);
    dev->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1);dev->SetTexture(0,atlas);
    dev->SetRenderState(D3DRS_ZENABLE,TRUE);dev->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
    dev->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    dev->SetRenderState(D3DRS_LIGHTING,FALSE);nativeWeather.apply(dev);
    dev->SetRenderState(D3DRS_STENCILENABLE,FALSE);dev->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);
    dev->SetRenderState(D3DRS_COLORWRITEENABLE,15);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);dev->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);
    dev->SetRenderState(D3DRS_ALPHAREF,128);dev->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATEREQUAL);
    dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);
    dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);dev->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
    dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
    dev->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);dev->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
    dev->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);dev->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
    // Apply host ambient, local lamps and Minecraft emission once, in vertex colors.
    nativeLighting.begin(playerSnapshot.hour);
    dev->SetTexture(1,nullptr);dev->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
    dev->SetTextureStageState(2,D3DTSS_COLOROP,D3DTOP_DISABLE);
    dev->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT);dev->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
    D3DMATRIX cameraView{},cameraProjection{};dev->GetTransform(D3DTS_VIEW,&cameraView);dev->GetTransform(D3DTS_PROJECTION,&cameraProjection);
    auto clip=grove::viewProjection(cameraView,cameraProjection);
    auto streamNow=GetTickCount64();
    dev->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
    dev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);
    for(const auto& [key,list]:sections)if(!list.empty()){
        auto bounds=sectionBounds.find(key);
        if(bounds==sectionBounds.end()||grove::visibleSection(clip,bounds->second.first,bounds->second.second)){
            auto& lit=litSections[key];
            if(lit.revision!=nativeLighting.revision||lit.vertices.size()!=list.size()){
                lit.vertices=list;nativeLighting.shadeMesh(lit.vertices);lit.revision=nativeLighting.revision;
            }
            float distance=0;
            if(playerSnapshot.valid&&bounds!=sectionBounds.end()){
                auto lo=bounds->second.first,hi=bounds->second.second;
                float dx=std::max({lo.x-playerSnapshot.position.x,0.f,playerSnapshot.position.x-hi.x});
                float dy=std::max({lo.y-playerSnapshot.position.y,0.f,playerSnapshot.position.y-hi.y});
                distance=std::hypot(dx,dy);
            }
            auto birth=sectionBirth.find(key);
            unsigned opacity=birth==sectionBirth.end()?255:grove::streamOpacity(birth->second,streamNow,distance);
            // Constant blend retains the atlas alpha test (leaf holes), native
            // depth and fog. No per-frame vertex copies or translucent floors.
            dev->SetRenderState(D3DRS_ALPHABLENDENABLE,opacity<255);
            dev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_BLENDFACTOR);
            dev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVBLENDFACTOR);
            dev->SetRenderState(D3DRS_BLENDFACTOR,opacity*0x01010101u);
            dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,static_cast<UINT>(lit.vertices.size()/3),lit.vertices.data(),sizeof(Vertex));
        }
    }
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
    drawWorldItems(dev);
    scene.draw(dev,atlas);
    nativeFelling.draw(dev);
    armor.draw(dev,scene,playerSnapshot,nativeInteraction);
    saved->Apply();saved->Release();dev->SetTransform(D3DTS_WORLD,&oldWorld);
}
#include "GroveOverlay.h"
#include "GroveDriveHud.h"
#include "GroveHud.h"
static void renderFrame(IDirect3DDevice9* dev,bool worldPass=true){
    if(!grove::FrameGate::mainTarget(dev))return;
    playerSnapshot=bindings.sample();
    if(nativeCampaign.testMode)nativeCampaign.testPrefix();
    if(nativeCampaign.enabled&&playerSnapshot.valid){
        try{nativeCampaign.ensure();nativeCash.setPrefix(nativeCampaign.walletPrefix());nativeRoadside.setPrefix(nativeCampaign.walletPrefix());}
        catch(...){nativeCampaign.blocked=true;log("Campaign world initialization failed; gameplay held and previous saves preserved");}
    }
    auto lifeNow=GetTickCount64();
    bool alive=playerSnapshot.valid&&playerSnapshot.health>0&&playerSnapshot.state>0&&!grove::dyingState(playerSnapshot.state);
    if(playerSnapshot.valid&&(playerSnapshot.health<=0||grove::dyingState(playerSnapshot.state)))deathObserved=true;
    bool changed=alive&&(lifePed!=playerSnapshot.ped||std::hypot(playerSnapshot.position.x-lifePosition.x,playerSnapshot.position.y-lifePosition.y)>96||std::abs(playerSnapshot.position.z-lifePosition.z)>96);
    if(!alive||changed){
        if(playerLive||changed)++lifeTransitions;
        playerLive=false;lifeReadyAt=lifeNow+2500;nativeActions.reset();
        attackUntil=0;pendingWeight=0;placeAnimation=guestMining=guestEating=false;guestUsing=0;
        guestMenu=cjPreview=false;buildTarget.valid=false;
        if(useOwned)inputEvent(17);if(hostMining)inputEvent(9);useOwned=hostMining=false;
        nativeInteraction.gate(false,false);
    }
    if(alive){lifePed=playerSnapshot.ped;lifePosition=playerSnapshot.position;playerLive=lifeNow>=lifeReadyAt;}
    if(playerLive&&lifeNow>lifeReadyAt+1500)deathObserved=false;
    if(!bindings.players)playerLive=true; // standalone renderer, no game/player memory
    if(nativeCampaign.enabled&&nativeCampaign.blocked)playerLive=false;
    if(connected()&&(roadsidePhase==1||roadsidePhase==2)
        &&(playerSnapshot.menu||(!playerLive&&!deathObserved)))roadsideStamp=lifeNow;
    BYTE excavationArea{};if(playerSnapshot.valid)gta::read(playerSnapshot.ped+0x2f,excavationArea);
    nativeExcavation.world=excavationArea;
    nativeExcavation.enabled=nativeExcavation.supported&&connected()&&playerSnapshot.valid&&(!nativeCampaign.enabled||!nativeCampaign.blocked);
#ifndef GROVE_RENDERER_UNIT
    windowInput();
#endif
    D3DMATRIX v{},proj{};
    bool valid=SUCCEEDED(dev->GetTransform(D3DTS_VIEW,&v))&&SUCCEEDED(dev->GetTransform(D3DTS_PROJECTION,&proj))
        && std::isfinite(v._41)&&std::isfinite(v._42)&&std::isfinite(v._43)
        && std::abs(proj._34)>0.5f && std::abs(v._44-1)<0.01f;
    uiCamera=valid;if(valid){uiView=v;uiProjection=proj;}
    if(valid!=cameraValid){cameraValid=valid;log(valid?"Perspective camera available":"No perspective camera");}
    if(connected()){consumeEvents();consumeRender();consumeOverlay(dev);if(valid){
        controls(v,proj);survival();
        if(bedSpawn.active)nativeInteraction.gate(true,true);
        // A GTA menu retains the last perspective transform. Do not paint world geometry over it.
        // The standalone diagnostic renderer has no GTA bindings.
        if(worldPass&&(!bindings.players || ((playerLive||deathObserved)&&playerSnapshot.valid&&!playerSnapshot.menu)))draw(dev);
        drawAim(dev);drawHotbar(dev);
    }}
    nativeFelling.tick(playerSnapshot,excavationArea,playerLive&&connected()&&nativeWorld.ready(),[](int model,gta::Vec origin,int logs,float dx,float dy,float length){
        double heading=std::atan2(-dy,dx);if(heading<0)heading+=6.283185307;
        unsigned angle=unsigned(std::lround(heading*256/6.283185307))&255;
        unsigned reach=unsigned(std::clamp(int(std::lround(length/4)),1,7));
        unsigned packed=(reach<<29)|(angle<<21)|(unsigned(logs)<<16)|(unsigned(std::lround(origin.z*16))&65535);
        inputEvent(2000+model,int(std::lround(origin.x*16)),int(packed),int(std::lround(-origin.y*16)));
    });
    if(publishedFalls!=nativeFelling.falls){publishedFalls=nativeFelling.falls;nativeTerrain.invalidate();}
    auto collisionSections=solidSections;nativeFelling.collision(collisionSections);
    nativeWorld.sync(collisionSections,playerSnapshot,playerLive&&connected(),nativeExcavation.chambers());
    nativeExcavation.chamberReady=[](const grove::SectionKey& key){
        return atlas&&!atlasDirty&&chamberMeshes.count(key)&&nativeWorld.hasSection(key);
    };
    nativeWater.publish(mem,playerSnapshot,playerLive&&connected()&&nativeWorld.ready());
    nativeTerrain.tick(playerSnapshot,playerLive&&connected()&&nativeWorld.ready());
    deliveryRadar.tick(playerSnapshot,playerLive&&connected(),deliveryHint.kind,deliveryHint.point,deliveryHint.stamp);
    structureRadar.tick(deliveryRadar,playerSnapshot,connected());
    nativeActors.publish(mem,playerSnapshot,nativeMelee,playerLive&&connected()&&nativeWorld.ready());
    nativeActors.vehicles.tickTrunk(playerSnapshot,connected(),!IsIconic(processWindow(GetCurrentProcessId()))&&GetForegroundWindow()==processWindow(GetCurrentProcessId()));
    bedSpawn.tick(playerSnapshot,bindings,connected()&&!playerSnapshot.menu&&!IsIconic(processWindow(GetCurrentProcessId()))
        &&GetForegroundWindow()==processWindow(GetCurrentProcessId()),[](gta::Vec origin){
            if(nativeWorld.supports(origin))return true;gta::Vec hit{},normal{};
            return nativeTerrain.target({origin.x,origin.y,origin.z+.1f},{origin.x,origin.y,origin.z-1.3f},hit,normal)
                &&normal.z>.5f&&std::abs(hit.z-(origin.z-1))<.2f;
        });
    nativeReactions.tick(playerSnapshot,nativeActors,nativeMobCombat,playerLive&&connected()&&!guestMenu&&!bedSpawn.active,
        [](gta::Vec a,gta::Vec b){return nativeTerrain.visible(a,b);});
    if(nativeMelee.tick(playerSnapshot,playerLive&&!bedSpawn.active&&nativeWorld.ready()&&connected()&&!guestMenu&&!playerSnapshot.menu&&!playerSnapshot.vehicle&&!playerSnapshot.mission&&!((roadsidePhase==1||roadsidePhase==2)&&GetTickCount64()-roadsideStamp<1500),solid))inputEvent(13);
    nativeActions.tick(playerSnapshot,playerLive&&!bedSpawn.active&&!guestMenu&&nativeWorld.ready()&&connected()&&GetTickCount64()-lastActionVitals<1500,
        guestMining,guestEating,placeAnimation,pendingWeight,itemType,itemCount>0&&GetTickCount64()<attackUntil,
        itemCount>0&&guestUsing==itemType&&GetTickCount64()-lastActionVitals<600,
        roadsidePhase==1&&GetTickCount64()-roadsideStamp<1500);placeAnimation=false;
    publishState(dev,v,valid);
    if(valid)drawHud(dev);
    auto now=GetTickCount64();
    if(now-lastTelemetry>1000){
        lastTelemetry=now;
        int cash{};bool cashValid=bindings.money(playerSnapshot,cash);
        if(connected())inputEvent(502,cash,cashValid?1:0,0);
        std::wstring moneyPath=telemetryPath;auto moneyName=moneyPath.find(L"GroveCraft-status.json");
        if(moneyName!=std::wstring::npos){moneyPath.replace(moneyName,std::wstring::npos,L"GroveCraft-economy-status.json");
            auto temp=moneyPath+L".tmp";if(auto* file=_wfopen(temp.c_str(),L"w")){
                std::fprintf(file,"{\"pid\":%lu,\"supported\":%s,\"cash\":%d,\"last_transaction\":%u,\"amount\":%d,\"result\":%d}\n",GetCurrentProcessId(),cashValid?"true":"false",cashValid?cash:-1,nativeCash.lastId,nativeCash.lastAmount,nativeCash.lastResult);
                std::fclose(file);MoveFileExW(temp.c_str(),moneyPath.c_str(),MOVEFILE_REPLACE_EXISTING);
            }}
        std::wstring expeditionPath=telemetryPath;auto expeditionName=expeditionPath.find(L"GroveCraft-status.json");
        if(expeditionName!=std::wstring::npos){
            expeditionPath.replace(expeditionName,std::wstring::npos,L"GroveCraft-expedition-status.json");
            auto temp=expeditionPath+L".tmp";if(auto* file=_wfopen(temp.c_str(),L"w")){
                auto& car=nativeActors.vehicles;int speed{},condition{};bool trip=car.trip(playerSnapshot,speed,condition);
                std::fprintf(file,"{\"trunk_supported\":%s,\"trunk\":%u,\"opening\":%.3f,\"opens\":%u,\"closes\":%u,\"props\":%u,\"contents\":%u,\"candidate\":%u,\"trip\":%s,\"kmh\":%d,\"condition\":%d}\n",
                    car.trunkSupported()?"true":"false",car.trunkId(),car.trunkOpening(),car.trunkOpens,car.trunkCloses,car.cargoProps,car.cargoContents,car.cargoCandidate,trip?"true":"false",speed,condition);
                std::fclose(file);MoveFileExW(temp.c_str(),expeditionPath.c_str(),MOVEFILE_REPLACE_EXISTING);
            }
        }
        std::wstring uiPath=telemetryPath;auto uiName=uiPath.find(L"GroveCraft-status.json");
        std::wstring mapPath=telemetryPath;auto mapName=mapPath.find(L"GroveCraft-status.json");
        if(mapName!=std::wstring::npos){mapPath.replace(mapName,std::wstring::npos,L"GroveCraft-map-status.json");auto temp=mapPath+L".tmp";
            if(auto* f=_wfopen(temp.c_str(),L"w")){structureRadar.telemetry(f,deliveryRadar);
                std::fclose(f);MoveFileExW(temp.c_str(),mapPath.c_str(),MOVEFILE_REPLACE_EXISTING);}}
        if(uiName!=std::wstring::npos){uiPath.replace(uiName,std::wstring::npos,L"GroveCraft-hud-status.json");auto temp=uiPath+L".tmp";
            if(auto* f=_wfopen(temp.c_str(),L"w")){
                std::fprintf(f,"{\"speed\":%.2f,\"acceleration_g\":%.3f,\"font\":%s,\"interaction\":%u,\"hint\":%u,\"hint_id\":%u,\"point\":[%.3f,%.3f,%.3f],\"delivery\":%u,\"job\":%u,\"cargo_progress\":%u,\"radar_supported\":%s,\"radar_handle\":%u,\"repair_phase\":%u,\"repair_progress\":%u,\"repair_loaded\":%s,\"repair_animations\":%u}\n",
                    dialSpeed,driveAcceleration,driveFontTexture?"true":"false",drawnInteractionKind,interactionHint.kind,interactionHint.id,
                    interactionHint.point.x,interactionHint.point.y,interactionHint.point.z,drawnDeliveryKind,deliveryHint.kind,deliveryHint.flags,deliveryRadar.supported?"true":"false",deliveryRadar.id(),roadsidePhase,roadsideProgress,nativeActions.repairLoaded?"true":"false",nativeActions.repairAnimations);
                std::fclose(f);MoveFileExW(temp.c_str(),uiPath.c_str(),MOVEFILE_REPLACE_EXISTING);
            }
        }
        std::wstring fellPath=telemetryPath;auto fellName=fellPath.find(L"GroveCraft-status.json");
        if(fellName!=std::wstring::npos){fellPath.replace(fellName,std::wstring::npos,L"GroveCraft-felling-status.json");
            FILE* report=_wfopen(fellPath.c_str(),L"w");if(report){std::fprintf(report,"{\"falls\":%u,\"capture_failures\":%u,\"hidden\":%u,\"vertices\":%u,\"textured\":%u,\"uv_attributes\":%u,\"mesh_min\":[%.3f,%.3f,%.3f],\"mesh_max\":[%.3f,%.3f,%.3f],\"draw_translation\":[%.3f,%.3f,%.3f]}\n",
                nativeFelling.falls,nativeFelling.captureFailures,nativeFelling.hidden,nativeFelling.vertices,nativeFelling.textured,nativeFelling.uvAttributes,
                nativeFelling.lastMin.x,nativeFelling.lastMin.y,nativeFelling.lastMin.z,nativeFelling.lastMax.x,nativeFelling.lastMax.y,nativeFelling.lastMax.z,
                nativeFelling.drawTranslation.x,nativeFelling.drawTranslation.y,nativeFelling.drawTranslation.z);std::fclose(report);}}
        std::wstring stabilityPath=telemetryPath;auto stabilityName=stabilityPath.find(L"GroveCraft-status.json");
        if(stabilityName!=std::wstring::npos){stabilityPath.replace(stabilityName,std::wstring::npos,L"GroveCraft-stability-status.json");
            FILE* stability=_wfopen(stabilityPath.c_str(),L"w");if(stability){std::fprintf(stability,
                "{\"alive\":%s,\"transitions\":%u,\"pre_hud\":%s,\"sprint_hook\":%s,\"starvation_hits\":%u,\"sprint_denials\":%u}\n",
                playerLive?"true":"false",lifeTransitions,nativeHudHooked?"true":"false",nativeSprintHooked?"true":"false",starvationHits,sprintDenials);std::fclose(stability);}}
        std::wstring armorPath=telemetryPath;auto armorName=armorPath.find(L"GroveCraft-status.json");
        if(armorName!=std::wstring::npos){auto fitPath=armorPath;fitPath.replace(armorName,std::wstring::npos,L"GroveCraft-armor-fit-status.json");
            FILE* fitReport=_wfopen(fitPath.c_str(),L"w");if(fitReport){auto b=armor.poseBounds;
                std::fprintf(fitReport,"{\"fit\":%s,\"missing_bone\":%u,\"bounds\":[%.3f,%.3f,%.3f,%.3f,%.3f,%.3f],\"cj\":[%.3f,%.3f,%.3f],\"drawn\":%u}\n",
                    armor.fit.ready?"true":"false",armor.missingBone,b[0],b[1],b[2],b[3],b[4],b[5],playerSnapshot.position.x,playerSnapshot.position.y,playerSnapshot.position.z,armor.drawn);std::fclose(fitReport);}}
        if(armorName!=std::wstring::npos){armorPath.replace(armorName,std::wstring::npos,L"GroveCraft-armor-status.json");
            FILE* report=_wfopen(armorPath.c_str(),L"w");if(report){std::fprintf(report,
                "{\"vertices\":%zu,\"batches\":%zu,\"drawn\":%u,\"rejected\":%u,\"preview\":%s,\"cj_triangles\":%u,\"armor_triangles\":%u,\"native_supported\":%s,\"value\":%.2f,\"protected_hits\":%u,\"wear_reports\":%u,\"raw\":%.3f,\"reduced\":%.3f,\"calls\":%u,\"fixture_calls\":%u,\"fixture_source\":%u,\"fixture_applied\":%s,\"fixture_enabled\":%s,\"cursor\":[%.2f,%.2f],\"panel\":[%.2f,%.2f,%.2f,%.2f]}\n",
                armor.vertices.size(),armor.batches.size(),armor.drawn,armor.rejected,cjPreview&&now-previewStamp<600?"true":"false",previewTriangles,previewArmorTriangles,nativeArmor.supported?"true":"false",nativeArmor.value,nativeArmor.protectedHits,nativeArmor.wearReports,nativeArmor.lastRaw,nativeArmor.lastReduced,nativeArmor.calls,nativeArmor.fixtureCalls,unsigned(nativeArmor.fixtureSource),nativeArmor.fixtureApplied?"true":"false",armorTestFixture?"true":"false",guiCursorX,guiCursorY,previewLeft,previewTop,previewWidth,previewHeight);std::fclose(report);}}
        std::wstring digPath=telemetryPath;auto digName=digPath.find(L"GroveCraft-status.json");
        if(digName!=std::wstring::npos){std::wstring aimPath=telemetryPath;aimPath.replace(digName,std::wstring::npos,L"GroveCraft-aim-status.json");
            if(auto* aim=_wfopen(aimPath.c_str(),L"w")){
                std::fprintf(aim,"{\"source\":%u,\"hit\":%s,\"cell\":[%d,%d,%d],\"place\":[%d,%d,%d],\"face\":%d,\"native_point\":[%.5f,%.5f,%.5f],\"native_normal\":[%.5f,%.5f,%.5f],\"can_mine\":%s,\"attack_priority\":%s,\"item_kind\":%u,\"query_reason\":%u,\"query_model\":%d,\"query_type\":%u,\"query_from_gta\":[%.5f,%.5f,%.5f],\"query_to_gta\":[%.5f,%.5f,%.5f],\"query_point_gta\":[%.5f,%.5f,%.5f]}\n",
                    aimSource,buildTarget.hit?"true":"false",buildTarget.x,buildTarget.y,buildTarget.z,buildTarget.px,buildTarget.py,buildTarget.pz,buildTarget.face,
                    aimPoint.x,aimPoint.y,aimPoint.z,aimNormal.x,aimNormal.y,aimNormal.z,aimCanMine?"true":"false",aimAttackPriority?"true":"false",itemType,
                    nativeTerrain.targetReason,nativeTerrain.targetModel,nativeTerrain.targetType,
                    nativeTerrain.targetFrom.x,nativeTerrain.targetFrom.y,nativeTerrain.targetFrom.z,
                    nativeTerrain.targetTo.x,nativeTerrain.targetTo.y,nativeTerrain.targetTo.z,
                    nativeTerrain.targetPoint.x,nativeTerrain.targetPoint.y,nativeTerrain.targetPoint.z);
                std::fclose(aim);
            }}
        if(digName!=std::wstring::npos){digPath.replace(digName,std::wstring::npos,L"GroveCraft-excavation-status.json");
            FILE* dig=_wfopen(digPath.c_str(),L"w");if(dig){std::fprintf(dig,
                "{\"supported\":%s,\"enabled\":%s,\"world\":%u,\"cells\":%u,\"packets\":%u,\"revision\":%u,\"contacts_removed\":%u,\"rays_removed\":%u,\"draws_clipped\":%u,\"draws_rejected\":%u,\"indexed_calls\":%u,\"world_draws\":%u,\"affected_entities\":%u,\"no_box_draws\":%u,\"last_type\":%u,\"stage\":%u,\"last_cell\":[%.3f,%.3f,%.3f],\"last_min\":[%.3f,%.3f,%.3f],\"last_max\":[%.3f,%.3f,%.3f]}\n",
                nativeExcavation.supported?"true":"false",nativeExcavation.enabled?"true":"false",nativeExcavation.world,nativeExcavation.cells,nativeExcavation.packets,nativeExcavation.revision,nativeExcavation.removedContacts,nativeExcavation.removedRays,nativeExcavation.clippedDraws,nativeExcavation.rejectedDraws,
                nativeExcavation.indexedCalls,nativeExcavation.worldDraws,nativeExcavation.affectedEntities,nativeExcavation.noBoxDraws,nativeExcavation.lastType,nativeExcavation.stage,nativeExcavation.lastCell.x,nativeExcavation.lastCell.y,nativeExcavation.lastCell.z,nativeExcavation.lastDrawMin.x,nativeExcavation.lastDrawMin.y,nativeExcavation.lastDrawMin.z,nativeExcavation.lastDrawMax.x,nativeExcavation.lastDrawMax.y,nativeExcavation.lastDrawMax.z);std::fclose(dig);}
        }
        std::wstring waterPath=telemetryPath;auto waterName=waterPath.find(L"GroveCraft-status.json");
        std::wstring performancePath=telemetryPath;auto performanceName=performancePath.find(L"GroveCraft-status.json");
        if(performanceName!=std::wstring::npos){performancePath.replace(performanceName,std::wstring::npos,L"GroveCraft-performance.json");
            if(FILE* perf=_wfopen(performancePath.c_str(),L"w")){
                std::fprintf(perf,"{\"native_frames\":%u,\"cut_cpu_us\":%.2f,\"cut_cache_hits\":%u,\"cut_cache_misses\":%u,\"cut_cache_bytes\":%zu,\"cut_cache_entries\":%zu,\"plant_hook\":%s,\"plant_draws\":%u,\"plants_removed\":%u,\"plant_sight_queries\":%u,\"plant_sight_hits\":%u,\"plant_sight_pending\":%u,\"plant_sight_hidden\":%u,\"plant_sight_cpu_us\":%.2f}\n",
                    stateSeq/2,nativeExcavation.cutMicros,nativeExcavation.cutCacheHits,nativeExcavation.cutCacheMisses,nativeExcavation.cutCache.memory(),nativeExcavation.cutCache.size(),nativeExcavation.plantsCode&&nativeExcavation.plants?"true":"false",nativeExcavation.plantDraws,nativeExcavation.plantsRemoved,plantVisibility.queries,plantVisibility.hits,plantVisibility.pending,plantVisibility.hidden,plantRayMicros);std::fclose(perf);
            }}
        if(waterName!=std::wstring::npos){waterPath.replace(waterName,std::wstring::npos,L"GroveCraft-water-status.json");
            FILE* water=_wfopen(waterPath.c_str(),L"w");if(water){std::fprintf(water,"{\"supported\":%s,\"enabled\":%s,\"liquid_cells\":%u,\"rejected\":%u,\"swim_queries\":%u,\"native_grids\":%u,\"wet_columns\":%u}\n",nativeWater.supported?"true":"false",nativeWater.enabled?"true":"false",nativeWater.cells,nativeWater.rejected,nativeWater.queries,nativeWater.nativeGrids,nativeWater.wetColumns);std::fclose(water);}}
        std::wstring combatPath=telemetryPath;auto combatName=combatPath.find(L"GroveCraft-status.json");
        if(combatName!=std::wstring::npos){combatPath.replace(combatName,std::wstring::npos,L"GroveCraft-combat-status.json");
            FILE* combat=_wfopen(combatPath.c_str(),L"w");if(combat){std::fprintf(combat,"{\"null_source_safe\":%s,\"null_impulses_skipped\":%u}\n",nativeMelee.nullSourceSafe?"true":"false",unsigned(groveDamageNullSkips));std::fclose(combat);}}
        std::wstring mobsPath=telemetryPath;auto mobsName=mobsPath.find(L"GroveCraft-status.json");
        if(mobsName!=std::wstring::npos){mobsPath.replace(mobsName,std::wstring::npos,L"GroveCraft-mobs-status.json");
            FILE* mobs=_wfopen(mobsPath.c_str(),L"w");if(mobs){std::fprintf(mobs,
                "{\"scene_vertices\":%zu,\"scene_rejected\":%u,\"npc_proxies\":%u,\"native_damage_hits\":%u,\"damage_rejected\":%u,\"terrain_supported\":%s,\"terrain_columns\":%zu,\"terrain_regions\":%u,\"terrain_epoch\":%u,\"terrain_rays\":%u,\"terrain_hits\":%u,\"terrain_rejected\":%u,\"vehicles\":%u,\"mob_reports\":%zu,\"bullet_hits\":%u,\"minecraft_blasts\":%u,\"gta_blasts\":%u,\"npc_shoot_tasks\":%u,\"npc_flee_tasks\":%u,\"bullet_calls\":%u,\"bullet_stage\":%d,\"weapon\":%d,\"shot_from\":[%.3f,%.3f,%.3f],\"shot_to\":[%.3f,%.3f,%.3f]}\n",
                scene.vertexCount(),scene.rejected,nativeActors.count,nativeActors.damageHits,nativeActors.rejected,
                nativeTerrain.supported?"true":"false",nativeTerrain.columns(),nativeTerrain.regions.load(),nativeTerrain.epoch(),
                nativeTerrain.rays,nativeTerrain.hits,nativeTerrain.rejected.load(),nativeActors.vehicles.count,
                nativeMobCombat.count(),nativeMobCombat.bulletHits,nativeMobCombat.blasts,nativeMobCombat.hostBlasts,nativeReactions.shotsAssigned,nativeReactions.flightsAssigned,nativeMobCombat.bulletCalls,nativeMobCombat.bulletStage,nativeMobCombat.lastWeapon,
                nativeMobCombat.lastFrom.x,nativeMobCombat.lastFrom.y,nativeMobCombat.lastFrom.z,
                nativeMobCombat.lastTo.x,nativeMobCombat.lastTo.y,nativeMobCombat.lastTo.z);std::fclose(mobs);}
        }
        wchar_t pending[MAX_PATH];std::swprintf(pending,MAX_PATH,L"%ls.tmp",telemetryPath);
        std::wstring lightPath=telemetryPath;lightPath.replace(lightPath.find(L"GroveCraft-status.json"),std::wstring::npos,L"GroveCraft-lighting.json");
        if(FILE* light=_wfopen(lightPath.c_str(),L"w")){
            auto factor=nativeLighting.factors(playerSnapshot.position,0xf00,0);
            auto lamps=nativeLighting.local(playerSnapshot.position);
            std::fprintf(light,"{\"supported\":%s,\"ambient\":[%.4f,%.4f,%.4f],\"object_ambient\":[%.4f,%.4f,%.4f],\"daylight\":%.4f,\"point_lights\":%u,\"samples\":%u,\"cj_lamp_rgb\":[%.4f,%.4f,%.4f],\"cj_factors\":[%.4f,%.4f,%.4f]}\n",
                nativeLighting.supported?"true":"false",nativeLighting.ambient[0],nativeLighting.ambient[1],nativeLighting.ambient[2],nativeLighting.objectAmbient[0],nativeLighting.objectAmbient[1],nativeLighting.objectAmbient[2],nativeLighting.daylight,nativeLighting.active,nativeLighting.samples,lamps[0],lamps[1],lamps[2],factor[0],factor[1],factor[2]);std::fclose(light);
        }
        FILE* status=_wfopen(pending,L"w");
        if(status){
            std::fprintf(status,"{\"pid\":%lu,\"camera\":%s,\"guest\":%s,\"player\":%s,\"mission\":%s,\"vehicle\":%s,\"menu\":%s,\"hour\":%.3f,\"x\":%.4f,\"y\":%.4f,\"z\":%.4f,\"health\":%.2f,\"ped_state\":%d,\"sections\":%zu,\"vertices\":%zu,\"build\":%s,\"collision\":%s,\"native_collision\":%s,\"collider_sections\":%zu,\"collision_boxes\":%zu,\"collision_failures\":%u,\"corrections\":%llu,\"food\":%d,\"guest_hour\":%.3f,\"healed\":%.2f,\"gta_food_linked\":%s,\"overlay_frame\":%llu,\"target_valid\":%s,\"target\":[%d,%d,%d],\"face\":%d,\"mining_progress\":%.3f,\"mining\":%s,\"eating\":%s,\"animations_supported\":%s,\"animations\":%u,\"food_animation_loaded\":%s,\"fat\":%.2f,\"weight_supported\":%s,\"drops\":%u,\"cracks\":%u}\n",GetCurrentProcessId(),valid?"true":"false",connected()?"true":"false",playerSnapshot.valid?"true":"false",playerSnapshot.mission?"true":"false",playerSnapshot.vehicle?"true":"false",playerSnapshot.menu?"true":"false",playerSnapshot.hour,playerSnapshot.position.x,playerSnapshot.position.y,playerSnapshot.position.z,playerSnapshot.health,playerSnapshot.state,sections.size(),totalVertices,buildMode?"true":"false",nativeWorld.entities()?"true":"false",nativeWorld.supported?"true":"false",nativeWorld.entities(),nativeWorld.boxCount,nativeWorld.failures,collisionCorrections,foodLevel,guestHour,regeneratedHealth,nativeFoodLinked?"true":"false",overlayFrame,buildTarget.valid?"true":"false",buildTarget.px,buildTarget.py,buildTarget.pz,buildTarget.face,miningProgress,guestMining?"true":"false",guestEating?"true":"false",nativeActions.supported?"true":"false",nativeActions.animations,nativeActions.foodLoaded?"true":"false",nativeActions.fat,nativeActions.weightSupported?"true":"false",visibleDrops,visibleCracks);
            std::fclose(status);
            MoveFileExW(pending,telemetryPath,MOVEFILE_REPLACE_EXISTING);
            std::wstring uiPath=telemetryPath;auto name=uiPath.find(L"GroveCraft-status.json");
            if(name!=std::wstring::npos){uiPath.replace(name,std::wstring::npos,L"GroveCraft-interface-status.json");
                FILE* ui=_wfopen(uiPath.c_str(),L"w");if(ui){std::fprintf(ui,
                    "{\"gui\":%s,\"cursor\":[%.1f,%.1f],\"slot\":%u,\"kind\":%u,\"count\":%d,\"hand_visible\":%s,\"hand\":[%.3f,%.3f,%.3f],\"native_melee\":%s,\"hits\":%u,\"victim\":%u,\"before\":%.2f,\"after\":%.2f,\"panel\":[%.1f,%.1f,%.1f,%.1f]}\n",
                    guestMenu?"true":"false",guiCursorX,guiCursorY,selectedSlot,itemType,itemCount,nativeInteraction.handVisible?"true":"false",
                    nativeInteraction.hand.x,nativeInteraction.hand.y,nativeInteraction.hand.z,nativeMelee.supported?"true":"false",nativeMelee.hits,
                    unsigned(nativeMelee.lastVictim),nativeMelee.before,nativeMelee.after,menuLeft,menuTop,menuWidth,menuHeight);std::fclose(ui);}
            }
        }
    }
}
#ifndef GROVE_RENDERER_UNIT
static void __fastcall armoredNativeHit(void* calculator,void*,void* victim,void* response,bool speak){
    auto address=reinterpret_cast<std::uintptr_t>(calculator),ped=reinterpret_cast<std::uintptr_t>(victim);
    if(ped==playerSnapshot.ped)++nativeArmor.calls;
    float raw{};DWORD source{};int weapon{};BYTE calculated{};
    bool reduce=connected()&&playerSnapshot.valid&&ped==playerSnapshot.ped&&nativeArmor.value>0
        &&GetTickCount64()-nativeArmor.stamp<1000&&gta::read(address,source)&&source
        &&gta::read(address+4,raw)&&std::isfinite(raw)&&raw>0&&raw<=500&&gta::read(address+12,weapon)&&grove::armorWeapon(weapon)
        &&gta::read(reinterpret_cast<std::uintptr_t>(response)+10,calculated)&&!calculated
        &&gta::accessible(reinterpret_cast<void*>(address+4),4,true);
    if(reduce){nativeArmor.lastRaw=raw;nativeArmor.lastReduced=grove::armorDamage(raw,nativeArmor.value,nativeArmor.toughness,
        weapon>=22?nativeArmor.bulletProtection:nativeArmor.meleeProtection);
        std::memcpy(reinterpret_cast<void*>(address+4),&nativeArmor.lastReduced,4);}
    nativeArmor.original(calculator,victim,response,speak);
    if(reduce){std::memcpy(reinterpret_cast<void*>(address+4),&raw,4);
        float health{},vest{};if(gta::read(reinterpret_cast<std::uintptr_t>(response),health)
            &&gta::read(reinterpret_cast<std::uintptr_t>(response)+4,vest)&&std::isfinite(health+vest)&&health+vest>0){
            ++nativeArmor.protectedHits;if(inputEvent(19,int(std::ceil(raw/5*256)),weapon))++nativeArmor.wearReports;
        }
    }
}
static HRESULT (WINAPI* originalIndexed)(IDirect3DDevice9*,D3DPRIMITIVETYPE,INT,UINT,UINT,UINT,UINT);
static HRESULT WINAPI excavationIndexed(IDirect3DDevice9* d,D3DPRIMITIVETYPE t,INT base,UINT minimum,UINT count,UINT start,UINT primitives){
    nativeWeather.capture(d);
    nativeFelling.noteDraw();
    if(nativeExcavation.drawingPlants&&nativeExcavation.enabled&&nativeExcavation.cells){
        ++nativeExcavation.plantDraws;D3DMATRIX world{};
        if(SUCCEEDED(d->GetTransform(D3DTS_WORLD,&world))){
            bool removed=nativeExcavation.contains({world._41,world._42,world._43-.04f});
            if(removed||!plantVisibility.visible({world._41,world._42,world._43},[](auto from,auto root){
                LARGE_INTEGER start{},end{};QueryPerformanceCounter(&start);
                bool visible=nativeTerrain.plantVisible({from[0],from[1],from[2]},{root[0],root[1],root[2]});
                QueryPerformanceCounter(&end);double cost=(end.QuadPart-start.QuadPart)*1000000.0/plantClock.QuadPart;
                plantPassRayMicros+=cost;plantRayMicros+=cost;return visible;
            },[](){return !plantClock.QuadPart||plantPassRayMicros>=350;})){
                ++nativeExcavation.plantsRemoved;return D3D_OK;
            }
        }
    }
    // A cut exposes the underside of GTA's originally single-sided mesh.
    // Keep the remaining surface opaque from inside the excavation too.
    DWORD cull{};bool backfaces=nativeExcavation.enabled&&nativeExcavation.rendering
        &&SUCCEEDED(d->GetRenderState(D3DRS_CULLMODE,&cull));
    if(backfaces)d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    HRESULT result{};
    if(!grove::cutNativeDraw(nativeExcavation,d,t,base,minimum,count,start,primitives,result))result=originalIndexed(d,t,base,minimum,count,start,primitives);
    if(backfaces)d->SetRenderState(D3DRS_CULLMODE,cull);
    return result;
}
static void __cdecl excavationPlants(){
    D3DMATRIX view{},projection{};grove::PlantVisibility::Point camera{};bool underground=false;
    if(frameDevice&&nativeExcavation.enabled&&nativeExcavation.cells&&!playerSnapshot.menu
        &&SUCCEEDED(frameDevice->GetTransform(D3DTS_VIEW,&view))&&SUCCEEDED(frameDevice->GetTransform(D3DTS_PROJECTION,&projection))
        &&std::abs(projection._34)>.5f&&std::abs(view._44-1)<.01f){
        camera={-(view._41*view._11+view._42*view._12+view._43*view._13),
                -(view._41*view._21+view._42*view._22+view._43*view._23),
                -(view._41*view._31+view._42*view._32+view._43*view._33)};
        bool finite=true;for(auto v:camera)finite=finite&&std::isfinite(v);
        auto feet=playerSnapshot.position;feet.z+=.2f;
        underground=finite&&(nativeExcavation.contains({camera[0],camera[1],camera[2]})||nativeExcavation.contains(feet));
    }
    plantVisibility.begin(camera,nativeExcavation.world,nativeExcavation.revision,underground);
    if(!plantClock.QuadPart)QueryPerformanceFrequency(&plantClock);plantPassRayMicros=0;
    bool previous=nativeExcavation.drawingPlants;nativeExcavation.drawingPlants=true;
    nativeExcavation.plants();nativeExcavation.drawingPlants=previous;
}
static void __fastcall excavationEntity(void* entity,void*){
    bool previous=nativeExcavation.rendering,oldWood=nativeExcavation.woodContext;auto old=nativeExcavation.renderMatrix;
    auto oldRoot=nativeExcavation.woodRoot;auto oldRadius=nativeExcavation.woodRadius;BYTE type{};DWORD matrix{};
    auto ptr=reinterpret_cast<std::uintptr_t>(entity);
    nativeExcavation.rendering=false;
    if(nativeExcavation.enabled&&gta::read(ptr+0x36,type)&&(type&7)==1){
        if(gta::read(ptr+0x14,matrix)&&matrix&&gta::read(matrix,nativeExcavation.renderMatrix))nativeExcavation.rendering=true;
        else {nativeExcavation.renderMatrix={};float heading{};gta::read(ptr+0x10,heading);
            nativeExcavation.renderMatrix.right={std::cos(heading),std::sin(heading),0};nativeExcavation.renderMatrix.forward={-std::sin(heading),std::cos(heading),0};nativeExcavation.renderMatrix.up.z=1;
            if(gta::read(ptr+4,nativeExcavation.renderMatrix.pos))nativeExcavation.rendering=true;}
    }
    bool hide=false;
    if(nativeExcavation.rendering){hide=nativeFelling.begin(entity,nativeExcavation.renderMatrix,nativeExcavation,
        playerLive&&!playerSnapshot.menu&&!playerSnapshot.mission&&nativeWorld.ready());
        nativeExcavation.rendering=nativeExcavation.affected(entity,nativeExcavation.renderMatrix);
    }
    if(!hide){if(nativeFelling.collectingMesh())nativeExcavation.capture=[](IDirect3DDevice9* dev,const std::vector<BYTE>& data,unsigned stride,const std::vector<D3DVERTEXELEMENT9>& decl){nativeFelling.capture(dev,data,stride,decl);};
        nativeExcavation.render(entity);nativeFelling.end();nativeExcavation.capture={};}
    nativeExcavation.rendering=previous;nativeExcavation.woodContext=oldWood;nativeExcavation.renderMatrix=old;
    nativeExcavation.woodRoot=oldRoot;nativeExcavation.woodRadius=oldRadius;
}
static int __cdecl excavationPhysics(const grove::NativeExcavation::Matrix& a,grove::NativeCol& ca,const grove::NativeExcavation::Matrix& b,grove::NativeCol& cb,grove::NativeExcavation::Point* spheres,grove::NativeExcavation::Point* lines,float* distances,bool all){
    if(nativeFelling.suppresses(a,ca)||nativeFelling.suppresses(b,cb))return 0;
    return nativeExcavation.contacts(a,ca,b,cb,spheres,lines,distances,all);}
static bool __cdecl excavationRay(const grove::NativeExcavation::Line& ln,const grove::NativeExcavation::Matrix& m,grove::NativeCol& col,grove::NativeExcavation::Point& cp,float& dist,bool see,bool shoot){
    if(nativeFelling.suppresses(m,col))return false;
    if(!nativeExcavation.nativeSurface(col))return nativeExcavation.ray(ln,m,col,cp,dist,see,shoot);
    return nativeExcavation.sight(ln,cp,dist,[&](const auto& line,auto& point,float& distance){return nativeExcavation.ray(line,m,col,point,distance,see,shoot);},nativeExcavation.woodSurface(col));
}
static bool __cdecl excavationVertical(const grove::NativeExcavation::Line& ln,const grove::NativeExcavation::Matrix& m,grove::NativeCol& col,grove::NativeExcavation::Point& cp,float& dist,bool see,bool shoot,void* poly){
    if(nativeFelling.suppresses(m,col))return false;
    if(!nativeExcavation.nativeSurface(col))return nativeExcavation.vertical(ln,m,col,cp,dist,see,shoot,poly);
    return nativeExcavation.sight(ln,cp,dist,[&](const auto& line,auto& point,float& distance){return nativeExcavation.vertical(line,m,col,point,distance,see,shoot,poly);},nativeExcavation.woodSurface(col));
}
static bool __cdecl excavationTest(const grove::NativeExcavation::Line& ln,const grove::NativeExcavation::Matrix& m,grove::NativeCol& col,bool see,bool shoot){
    if(nativeFelling.suppresses(m,col))return false;
    if(!nativeExcavation.enabled||!nativeExcavation.nativeSurface(col)||(nativeExcavation.sampling&&nativeExcavation.woodSurface(col)))return nativeExcavation.test(ln,m,col,see,shoot);
    grove::NativeExcavation::Point point{};float distance=1;return excavationRay(ln,m,col,point,distance,see,shoot);
}
static void __fastcall mobBulletImpact(void* weapon,void*,void* shooter,void* victim,const gta::Vec& from,const gta::Vec& to,const grove::NativeHitPoint& cp,int incremental){
    gta::Vec stopped{};
    bool hit=connected()&&nativeMobCombat.bullet(weapon,shooter,victim,from,to,cp,playerSnapshot,
        nativeActors.identity(reinterpret_cast<std::uintptr_t>(shooter)),rawInput,stopped);
    // Preserve GTA's bullet trace; stop this bullet at the mob so it cannot also
    // damage the native ped/car/wall behind it. Its Minecraft hurt event owns loot.
    nativeMobCombat.impact(weapon,shooter,hit?nullptr:victim,from,hit?stopped:to,cp,incremental);
}
static bool __cdecl mobExplosion(void* victim,void* creator,int type,gta::Vec pos,unsigned lifetime,unsigned char sound,float shake,unsigned char invisible){
    bool made=nativeMobCombat.explosion(victim,creator,type,pos,lifetime,sound,shake,invisible);
    if(made&&connected()&&!nativeMobCombat.mirroring&&playerSnapshot.valid&&!playerSnapshot.menu&&!playerSnapshot.mission
        &&std::isfinite(pos.x+pos.y+pos.z)&&std::hypot(pos.x-playerSnapshot.position.x,pos.y-playerSnapshot.position.y)<128){
        unsigned code=(unsigned(type)&31)|(18u<<p::kGtaBlastRadiusShift);
        if(creator==reinterpret_cast<void*>(playerSnapshot.ped))code|=p::kGtaBlastByPlayer;
        if(rawInput(p::kInGtaExplosion,code,int(pos.x*256),int(pos.z*256),int(-pos.y*256)))++nativeMobCombat.hostBlasts;
    }return made;
}
static bool __cdecl minecraftWater(float x,float y,float z,float* height,bool force,gta::Vec* normal){return nativeWater.query(x,y,z,height,force,normal);}
static void resetGraphicsCaches(){
    nativeFelling.resetDevice();
    if(driveFontTexture){driveFontTexture->Release();driveFontTexture=nullptr;}
    if(hudTexture){hudTexture->Release();hudTexture=nullptr;}
    if(hotbarTexture){hotbarTexture->Release();hotbarTexture=nullptr;}
    if(menuTexture){menuTexture->Release();menuTexture=nullptr;}
    if(atlas){atlas->Release();atlas=nullptr;atlasDirty=!atlasPixels.empty();}
    scene.resetDevice();cameraValid=false;
}
static HRESULT WINAPI endScene(IDirect3DDevice9* dev){
    // The intro player owns a temporary D3D device. Do not retain it or upload
    // guest textures into it; let its owner tear down the movie completely.
    // The first real GTA HUD pass establishes that the game renderer is live.
    if(nativeHudHooked&&!nativeHudObserved)return originalEnd(dev);
    if(grove::FrameGate::mainTarget(dev)){
        // Movie and game devices can share the hooked implementation. GPU
        // objects belong to one device; never bind a previous device's atlas.
        if(frameDevice&&frameDevice!=dev){resetGraphicsCaches();worldRenderedBeforeHud=false;}
        frameDeviceLease->reset(dev);frameDevice=frameDeviceLease->get();
        if(!nativeHudHooked)renderFrame(dev);
        else if(!worldRenderedBeforeHud)renderFrame(dev,false); // frontend/loading still needs fresh state, never world geometry
        if(nativeHudHooked){drawAim(dev);drawHotbar(dev);drawHud(dev);}
        worldRenderedBeforeHud=false;
    }
    return originalEnd(dev);
}
using HudFn=void (__cdecl*)();static HudFn originalHud;
static void beforeNativeHud(){
    nativeHudObserved=true;
    if(frameDevice&&grove::FrameGate::mainTarget(frameDevice)){renderFrame(frameDevice);worldRenderedBeforeHud=true;}
    originalHud();
}
using SprintFn=bool (__thiscall*)(void*);static SprintFn originalSprint;
static bool __fastcall minecraftSprint(void* pad,void*){
    bool pressed=originalSprint(pad);
    if(pressed&&connected()&&playerLive&&!playerSnapshot.vehicle&&!playerSnapshot.menu&&GetTickCount64()-lastVitals<2500&&foodLevel>=0&&foodLevel<=6){++sprintDenials;return false;}
    return pressed;
}
static HRESULT WINAPI reset(IDirect3DDevice9* d,D3DPRESENT_PARAMETERS* params){
    if(frameDevice==d){
        resetGraphicsCaches();frameDevice=nullptr;frameDeviceLease->reset();
        worldRenderedBeforeHud=false;
    }
    log("D3D9 Reset");return originalReset(d,params);
}
static bool hookGraphics(){
    WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=self;wc.lpszClassName=L"GroveCraftProbe";
    RegisterClassW(&wc);
    HWND window=CreateWindowW(wc.lpszClassName,L"",WS_OVERLAPPED,0,0,64,64,nullptr,nullptr,self,nullptr);
    IDirect3D9* d3d=Direct3DCreate9(D3D_SDK_VERSION);IDirect3DDevice9* dev=nullptr;
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window;
    if(!d3d||FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&dev))){
        if(d3d)d3d->Release();DestroyWindow(window);return false;
    }
    auto** table=*reinterpret_cast<void***>(dev);
    bool ok=MH_Initialize()==MH_OK && MH_CreateHook(table[42],reinterpret_cast<void*>(endScene),reinterpret_cast<void**>(&originalEnd))==MH_OK
        && MH_CreateHook(table[16],reinterpret_cast<void*>(reset),reinterpret_cast<void**>(&originalReset))==MH_OK;
    frameGate.init();
    if(ok&&frameGate.hudCode)nativeHudHooked=MH_CreateHook(reinterpret_cast<void*>(frameGate.hudCode),reinterpret_cast<void*>(beforeNativeHud),reinterpret_cast<void**>(&originalHud))==MH_OK;
    if(ok&&frameGate.sprintCode)nativeSprintHooked=MH_CreateHook(reinterpret_cast<void*>(frameGate.sprintCode),reinterpret_cast<void*>(minecraftSprint),reinterpret_cast<void**>(&originalSprint))==MH_OK;
    if(ok&&nativeInteraction.escapeCode)MH_CreateHook(reinterpret_cast<void*>(nativeInteraction.escapeCode),reinterpret_cast<void*>(interceptedEscape),reinterpret_cast<void**>(&originalEscape));
    if(ok&&nativeDamageGuard.code){nativeMelee.nullSourceSafe=nativeDamageGuard.install();log(nativeMelee.nullSourceSafe?"Native missing-source damage impulse guarded":"Native missing-source damage disabled");}
    if(ok&&nativeArmor.code){nativeArmor.supported=MH_CreateHook(reinterpret_cast<void*>(nativeArmor.code),reinterpret_cast<void*>(armoredNativeHit),reinterpret_cast<void**>(&nativeArmor.original))==MH_OK;log(nativeArmor.supported?"Native armor calculator hooked":"Native armor calculator unsupported");}
    if(ok&&nativeWater.queryCode)nativeWater.supported=MH_CreateHook(reinterpret_cast<void*>(nativeWater.queryCode),reinterpret_cast<void*>(minecraftWater),reinterpret_cast<void**>(&nativeWater.original))==MH_OK;
    if(ok&&nativeInteraction.menuCheckCode)MH_CreateHook(reinterpret_cast<void*>(nativeInteraction.menuCheckCode),reinterpret_cast<void*>(interceptedMenuCheck),reinterpret_cast<void**>(&originalMenuCheck));
    if(ok&&nativeInteraction.targetCode)MH_CreateHook(reinterpret_cast<void*>(nativeInteraction.targetCode),reinterpret_cast<void*>(ownedTarget),reinterpret_cast<void**>(&originalTarget));
    if(ok&&nativeInteraction.enterTargetCode)MH_CreateHook(reinterpret_cast<void*>(nativeInteraction.enterTargetCode),reinterpret_cast<void*>(ownedEnterTarget),reinterpret_cast<void**>(&originalEnterTarget));
    if(ok&&nativeMobCombat.impactCode)MH_CreateHook(reinterpret_cast<void*>(nativeMobCombat.impactCode),reinterpret_cast<void*>(mobBulletImpact),reinterpret_cast<void**>(&nativeMobCombat.impact));
    if(ok&&nativeMobCombat.explosionCode)MH_CreateHook(reinterpret_cast<void*>(nativeMobCombat.explosionCode),reinterpret_cast<void*>(mobExplosion),reinterpret_cast<void**>(&nativeMobCombat.explosion));
    if(ok&&nativeExcavation.physicsCode){
        nativeExcavation.supported=MH_CreateHook(table[82],reinterpret_cast<void*>(excavationIndexed),reinterpret_cast<void**>(&originalIndexed))==MH_OK
            &&MH_CreateHook(reinterpret_cast<void*>(nativeExcavation.physicsCode),reinterpret_cast<void*>(excavationPhysics),reinterpret_cast<void**>(&nativeExcavation.physics))==MH_OK
            &&MH_CreateHook(reinterpret_cast<void*>(nativeExcavation.rayCode),reinterpret_cast<void*>(excavationRay),reinterpret_cast<void**>(&nativeExcavation.ray))==MH_OK
            &&MH_CreateHook(reinterpret_cast<void*>(nativeExcavation.verticalCode),reinterpret_cast<void*>(excavationVertical),reinterpret_cast<void**>(&nativeExcavation.vertical))==MH_OK
            &&MH_CreateHook(reinterpret_cast<void*>(nativeExcavation.testCode),reinterpret_cast<void*>(excavationTest),reinterpret_cast<void**>(&nativeExcavation.test))==MH_OK
            &&MH_CreateHook(reinterpret_cast<void*>(nativeExcavation.renderCode),reinterpret_cast<void*>(excavationEntity),reinterpret_cast<void**>(&nativeExcavation.render))==MH_OK;
        if(nativeExcavation.supported&&nativeExcavation.plantsCode
            &&MH_CreateHook(reinterpret_cast<void*>(nativeExcavation.plantsCode),reinterpret_cast<void*>(excavationPlants),reinterpret_cast<void**>(&nativeExcavation.plants))!=MH_OK)nativeExcavation.plantsCode=0;
    }
    if(ok&&nativeCampaign.enabled){
        bool saved=MH_CreateHook(reinterpret_cast<void*>(nativeCampaign.saveCode),reinterpret_cast<void*>(pairedCampaignSave),reinterpret_cast<void**>(&originalCampaignSave))==MH_OK;
        bool loaded=MH_CreateHook(reinterpret_cast<void*>(nativeCampaign.loadCode),reinterpret_cast<void*>(pairedCampaignLoad),reinterpret_cast<void**>(&originalCampaignLoad))==MH_OK;
        bool reset=MH_CreateHook(reinterpret_cast<void*>(nativeCampaign.resetCode),reinterpret_cast<void*>(pairedCampaignReset),reinterpret_cast<void**>(&originalCampaignReset))==MH_OK;
        if(!saved||!loaded||!reset){nativeCampaign.supported=false;nativeCampaign.blocked=true;log("Paired campaign hooks unavailable; world selection held");}
        else log("New game, native SaveSlot and GenericLoad linked to Minecraft");
    }
    if(ok)ok=MH_EnableHook(MH_ALL_HOOKS)==MH_OK;
    dev->Release();d3d->Release();DestroyWindow(window);return ok;
}
#endif
static int startTransport(){
    wchar_t path[MAX_PATH];GetModuleFileNameW(self,path,MAX_PATH);
    wchar_t* slash=wcsrchr(path,L'\\');if(slash)wcscpy(slash+1,L"GroveCraft.log");
    wcscpy(telemetryPath,path);auto* telemetrySlash=wcsrchr(telemetryPath,L'\\');if(telemetrySlash)wcscpy(telemetrySlash+1,L"GroveCraft-status.json");
    std::wstring cashPrefix=telemetryPath;auto cashName=cashPrefix.find(L"GroveCraft-status.json");
    if(cashName!=std::wstring::npos){cashPrefix.replace(cashName,std::wstring::npos,L"GroveCraft-wallet-");nativeCash.setPrefix(cashPrefix);nativeRoadside.setPrefix(cashPrefix);}
    logfile=_wfopen(path,L"a");log("GroveCraft host loaded; native GTA voxel collisions automatic");
    wchar_t fitPath[MAX_PATH];wcscpy(fitPath,telemetryPath);auto* fitSlash=wcsrchr(fitPath,L'\\');
    if(fitSlash)wcscpy(fitSlash+1,L"GroveCraft-armor-fit.bin");
    log(armor.fit.load(fitPath)?"CJ calibrated armour skin loaded":"CJ armour calibration unavailable; legacy fit");
    mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,static_cast<DWORD>(p::kMappingBytes),L"Local\\GroveCraft_v1");
    if(!mapping||GetLastError()==ERROR_ALREADY_EXISTS){log("Mapping unavailable or owned by another host; refusing");return 1;}
    mem=static_cast<unsigned char*>(MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,0));
    if(!mem){log("MapViewOfFile failed");return 2;}
    auto* header=reinterpret_cast<p::Header*>(mem);
    header->version=p::kVersion;header->skyrimPid=GetCurrentProcessId();
    store64(&header->skyrimHeartbeatMs,GetTickCount64());
    auto* state=reinterpret_cast<p::SkyState*>(mem+p::kOffSkyState);
    state->flags=p::kSkyLoading|p::kSkyMenuOpen;state->viewportW=960;state->viewportH=540;
    reinterpret_cast<p::OverlayCtl*>(mem+p::kOffOverlayCtl)->state=2;
    MemoryBarrier();header->magic=0x564F5247;
    log("Created GroveCraft v11 bridge");
    log(nativeWorld.init()?"Native voxel world bindings resolved":"Native voxel world unsupported");
    log(nativeLighting.init()?"GTA timecycle ambient and local point lights resolved":"GTA lighting unavailable; readable unlit fallback");
    log(nativeWater.init()?"Minecraft water swimming and native lake query resolved":"Water swimming bridge unsupported");
    log(bindings.init()?"GTA player and mission bindings resolved":"No GTA bindings; renderer diagnostic mode");
    try{log(nativeCampaign.init()?"Personal campaign save bindings resolved":"Personal campaign mode disabled or unavailable");}catch(...){log("Campaign initialization failed; preserve previous saves");}
    log(nativeActions.init()?"Native action animations resolved":"Native action animations unsupported");
    log(bedSpawn.init()?"Native bed Teleport signature resolved":"Native bed Teleport unavailable; hospital fallback");
    log(nativeInteraction.init()?"Native hand/input resolved":"Native hand/input unsupported");
    log(nativeArmor.init()?"Native armor calculator signature resolved":"Native armor calculator signature unavailable");
    wchar_t armorFixture[4]{};armorTestFixture=GetEnvironmentVariableW(L"GROVECRAFT_ARMOR_NATIVE_TEST",armorFixture,4)==1&&armorFixture[0]==L'1';
    nativeDamageGuard.init();
    log(nativeMelee.init()?"Native melee damage resolved":"Native melee damage unsupported");
    log(nativeTerrain.init(mem)?"Native terrain probes resolved":"Native terrain probes unsupported");
    log(nativeExcavation.init()?"Native excavation bindings resolved":"Native excavation unsupported");
    log(nativeMobCombat.init()?"Native mob bullets/explosions resolved":"Native mob bullets/explosions unsupported");
    log(deliveryRadar.init()?"Native delivery radar resolved":"Native delivery radar unsupported");
    log(nativeActors.vehicles.init()?"Native vehicle mob proxies resolved":"Native vehicle mob proxies unsupported");
    log(nativeCrime.init()?"Native vandalism wanted binding resolved":"Native vandalism wanted unavailable");
    log(nativeReactions.init()?"Native mob retaliation/flee tasks resolved":"Native mob reaction tasks unsupported");
    return 0;
}
#ifndef GROVE_RENDERER_UNIT
static LONG CALLBACK diagnoseCrash(EXCEPTION_POINTERS* e){
    if(e->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION&&logfile){
        auto c=e->ContextRecord;std::fprintf(logfile,"Crash EIP=%08lx ECX=%08lx EDX=%08lx EAX=%08lx\n",c->Eip,c->Ecx,c->Edx,c->Eax);std::fflush(logfile);
    }return EXCEPTION_CONTINUE_SEARCH;
}
struct ProcessWindow {DWORD pid;HWND window{};};
static BOOL CALLBACK findProcessWindow(HWND h,LPARAM p){
    auto& w=*reinterpret_cast<ProcessWindow*>(p);DWORD owner{};GetWindowThreadProcessId(h,&owner);
    if(owner==w.pid&&GetWindow(h,GW_OWNER)==nullptr&&IsWindowVisible(h)){w.window=h;return FALSE;}return TRUE;
}
static HWND processWindow(DWORD pid){ProcessWindow w{pid};EnumWindows(findProcessWindow,reinterpret_cast<LPARAM>(&w));return w.window;}
static DWORD WINAPI run(void*){
    int result=startTransport();if(result)return result;
    AddVectoredExceptionHandler(0,diagnoseCrash);
    auto* header=reinterpret_cast<p::Header*>(mem);
    log(hookGraphics()?"D3D9 diagnostic hooks active":"D3D9 hooks unavailable; transport only");
    bool wasConnected=false;
    DWORD linkedGuest=0;bool closing=false,hostWasMinimized=false,guestWasMinimized=false;
    for(;;){
        store64(&header->skyrimHeartbeatMs,GetTickCount64());
        nativeCampaign.poll();InterlockedExchange(reinterpret_cast<volatile LONG*>(mem+0x900),nativeCampaign.enabled&&nativeCampaign.blocked?1:0);
        HWND host=processWindow(GetCurrentProcessId());DWORD foreground{};GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        DWORD guest=header->mcPid;if(guest)linkedGuest=guest;
        HWND guestWindow=guest?processWindow(guest):nullptr;
        bool hostMinimized=host&&IsIconic(host),guestMinimized=guestWindow&&IsIconic(guestWindow);
        if(host&&guestWindow){
            // Mirror edges, not levels: restoring one must not immediately minimize
            // it again because the other window is still awaiting restoration.
            if(hostMinimized&&!hostWasMinimized&&!guestMinimized)ShowWindowAsync(guestWindow,SW_MINIMIZE);
            else if(guestMinimized&&!guestWasMinimized&&!hostMinimized)ShowWindowAsync(host,SW_MINIMIZE);
            else if(!hostMinimized&&hostWasMinimized&&guestMinimized)ShowWindowAsync(guestWindow,SW_SHOWNOACTIVATE);
            else if(!guestMinimized&&guestWasMinimized&&hostMinimized)ShowWindowAsync(host,SW_RESTORE);
        }
        hostWasMinimized=hostMinimized;guestWasMinimized=guestMinimized;
        LONG paused=!host||IsIconic(host)||foreground!=GetCurrentProcessId();
        InterlockedExchange(reinterpret_cast<volatile LONG*>(mem+0x20),paused);
        if(linkedGuest&&!closing){HANDLE process=OpenProcess(SYNCHRONIZE,FALSE,linkedGuest);
            bool gone=process&&WaitForSingleObject(process,0)==WAIT_OBJECT_0;if(process)CloseHandle(process);
            if(gone&&host){closing=true;log("Minecraft closed: requesting normal paired GTA exit");PostMessage(host,WM_CLOSE,0,0);}
        }
        bool isConnected=connected();if(isConnected!=wasConnected){wasConnected=isConnected;log(isConnected?"Minecraft guest connected":"Minecraft guest disconnected");}
        Sleep(250);
    }
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID){
    if(reason==DLL_PROCESS_ATTACH){self=module;DisableThreadLibraryCalls(module);HANDLE thread=CreateThread(nullptr,0,run,nullptr,0,nullptr);if(thread)CloseHandle(thread);}
    return TRUE;
}
#endif
