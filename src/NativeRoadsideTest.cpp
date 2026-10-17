#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cassert>
namespace gta {struct Snapshot {bool menu{},mission{},vehicle{},valid=true;float health=100;};}
namespace grove {
class NativeVehicles {public:float health=500;bool eligible=true;BYTE generation=2;
 bool repairTarget(const gta::Snapshot&,unsigned,std::uintptr_t& address,BYTE& gen,float& hp){address=reinterpret_cast<std::uintptr_t>(&health);gen=generation;hp=health;return eligible;}
};}
#include "NativeRoadside.h"
int main(){
 wchar_t dir[MAX_PATH];GetTempPathW(MAX_PATH,dir);auto prefix=std::wstring(dir)+L"GroveCraft-roadside-test-"+std::to_wstring(GetCurrentProcessId())+L"-";
 grove::NativeVehicles car;grove::NativeRoadside repair;gta::Snapshot s;repair.setPrefix(prefix);
 assert(repair.request(car,s,1,11,12,7)==1&&car.health==750&&repair.repairs==1);
 assert(repair.request(car,s,1,11,12,7)==1&&car.health==750&&repair.repairs==1); // Lost ack: no second effect.
 grove::NativeRoadside cold; cold.setPrefix(prefix);car.health=500;
 assert(cold.request(car,s,1,11,12,7)==1&&car.health==500); // Completed receipt reopened: no replay.
 car.health=750;
 assert(repair.request(car,s,1,11,12,8)==-2&&car.health==750); // Different lease cannot reuse receipt.
 s.menu=true;assert(repair.request(car,s,2,11,12,7)==-1&&car.health==750);s.menu=false;
 assert(repair.request(car,s,2,11,12,7)==1&&car.health==900); // Capped; garages still matter.
 assert(repair.request(car,s,3,11,12,7)==0&&car.health==900);
 car.eligible=false;assert(repair.request(car,s,4,11,12,7)==0&&car.health==900);
 car.eligible=true;car.health=500;grove::NativeRoadside reopened;reopened.setPrefix(prefix);
 assert(reopened.request(car,s,4,11,12,7)==0&&car.health==500); // Receipt survives full object reopening.
 assert(reopened.request(car,s,6,11,12,7)==-2&&car.health==500); // Out-of-order requests held.
 auto path=prefix+L"repair-00000c00000b.dat";HANDLE file=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);assert(file!=INVALID_HANDLE_VALUE);
 BYTE bad=0;DWORD n{};assert(WriteFile(file,&bad,1,&n,nullptr)&&n==1);CloseHandle(file);
 assert(reopened.request(car,s,5,11,12,7)==-2&&car.health==500); // Corrupt journal never grants repair.
 DeleteFileW(path.c_str());std::puts("roadside receipts: retry, cap, guards, reopening, order and corruption passed");
}
