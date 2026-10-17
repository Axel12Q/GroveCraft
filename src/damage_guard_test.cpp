#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <vector>
#include <cstdio>
#include <cassert>
#include <cstring>
#include "MinHook.h"
// The unit verifies the actual x86 detour, not executable discovery (audited separately).
namespace gta {
struct OriginalImage { bool load(){return false;} std::vector<std::uintptr_t> scan(const char*){return {};} std::uintptr_t code(std::uintptr_t a){return a;} };
template<class T> bool read(std::uintptr_t,T&){return false;}
}
#include "NativeDamageGuard.h"
extern "C" { static void* fixtureCode __attribute__((used)); }
__attribute__((naked)) static int invoke(void*){
    __asm__ __volatile__("pushl %esi\nmovl 8(%esp), %esi\ncall *_fixtureCode\npopl %esi\nret\n");
}
int main(){
    // Original faulting instructions followed by RET; fallback returns99.
    unsigned char code[]={0x8b,0x76,0x14,0x8b,0x46,0x14,0xc3,0x90,0xb8,99,0,0,0,0xc3};
    fixtureCode=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);assert(fixtureCode);
    std::memcpy(fixtureCode,code,sizeof code);
    grove::NativeDamageGuard guard;guard.code=reinterpret_cast<std::uintptr_t>(fixtureCode);guard.skip=guard.code+8;
    assert(MH_Initialize()==MH_OK);assert(guard.install());assert(MH_EnableHook(MH_ALL_HOOKS)==MH_OK);
    DWORD event[8]{},source[8]{};source[5]=42;
    for(int i=0;i<1000;++i){event[5]=0;assert(invoke(event)==99);event[5]=reinterpret_cast<DWORD>(source);assert(invoke(event)==42);}
    assert(groveDamageNullSkips==1000);
    assert(MH_DisableHook(MH_ALL_HOOKS)==MH_OK);assert(MH_Uninitialize()==MH_OK);
    VirtualFree(fixtureCode,0,MEM_RELEASE);
    std::puts("{\"passed\":true,\"actual_x86_detour\":true,\"null_source_skip\":true,\"native_source_unchanged\":true,\"iterations\":2000}");
}
