#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cassert>
#include <cstdio>
#include "GameBindings.h"
#include "NativeCash.h"
int main(){
    auto* ped=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(ped);
    DWORD info[100]{};BYTE focus=0;info[0]=reinterpret_cast<std::uintptr_t>(ped);
    gta::Bindings b;b.players=reinterpret_cast<std::uintptr_t>(info);b.focus=reinterpret_cast<std::uintptr_t>(&focus);
    gta::Snapshot s;s.valid=true;s.menu=s.mission=false;s.health=100;s.state=1;s.ped=reinterpret_cast<std::uintptr_t>(ped);
    auto& money=*reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(info)+0xb8);money=350;
    wchar_t dir[MAX_PATH],file[MAX_PATH];GetTempPathW(MAX_PATH,dir);assert(GetTempFileNameW(dir,L"gcw",0,file));
    std::wstring prefix=std::wstring(file)+L"-";grove::NativeCash cash;cash.setPrefix(prefix);
    assert(cash.request(b,s,1,80,1,2)==1&&money==430);
    assert(cash.request(b,s,1,80,1,2)==1&&money==430); // Lost acknowledgement/retry.
    grove::NativeCash reopened;reopened.setPrefix(prefix);
    assert(reopened.request(b,s,1,80,1,2)==1&&money==430); // Reloaded receipt.
    assert(reopened.request(b,s,1,100,1,2)==-2&&money==430);
    assert(reopened.request(b,s,3,80,1,2)==-2&&money==430);
    s.menu=true;assert(reopened.request(b,s,2,80,1,2)==-1&&money==430);s.menu=false;
    s.mission=true;assert(reopened.request(b,s,2,80,1,2)==-1);s.mission=false;
    s.vehicle=true;assert(reopened.request(b,s,2,80,1,2)==-1);s.vehicle=false;
    s.health=0;assert(reopened.request(b,s,2,80,1,2)==-1);s.health=100;
    assert(reopened.request(b,s,2,-500,1,2)==0&&money==430);
    assert(reopened.request(b,s,2,-500,1,2)==0&&money==430);
    assert(reopened.request(b,s,3,-40,1,2)==1&&money==390);
    money=99999999;assert(reopened.request(b,s,4,80,1,2)==0&&money==99999999);
    info[0]=0;assert(reopened.request(b,s,5,80,1,2)==-1&&money==99999999);info[0]=s.ped;
    wchar_t suffix[32];std::swprintf(suffix,32,L"%06x%06x.dat",2,1);auto journal=prefix+suffix;
    HANDLE h=CreateFileW(journal.c_str(),GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);assert(h!=INVALID_HANDLE_VALUE);
    DWORD written;unsigned corrupt=0;WriteFile(h,&corrupt,4,&written,nullptr);CloseHandle(h);
    assert(reopened.request(b,s,5,80,1,2)==-2&&money==99999999);
    DeleteFileW(journal.c_str());DeleteFileW(file);VirtualFree(ped,0,MEM_RELEASE);
    std::puts("Cash receipts: no replay, reload, guards, funds/cap and corruption passed (synthetic).");
}
