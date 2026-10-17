#pragma once
// Owned NewSteam ComputeDamageAnim: the low-health KO impulse dereferences a
// missing CEventDamage source. Keep damage/animations, skip only that impulse.
extern "C" {
static void* groveDamageOriginal __attribute__((used));
static void* groveDamageSkip __attribute__((used));
static volatile unsigned groveDamageNullSkips __attribute__((used));
}
__attribute__((naked)) static void groveDamageForceGuard(){
    __asm__ __volatile__(
        "cmpl $0, 0x14(%esi)\n"
        "je 1f\n"
        "jmp *_groveDamageOriginal\n"
        "1: incl _groveDamageNullSkips\n"
        "jmp *_groveDamageSkip\n");
}
namespace grove {
class NativeDamageGuard {
public:
    std::uintptr_t code{},skip{};bool supported{};
    bool init(){
        gta::OriginalImage image;if(!image.load())return false;
        auto hits=image.scan("0F 8A ? ? ? ? 8B 76 14 8B 46 14 85 C0 74 08 83 C0 30 89 45 0C EB 06 8D 56 04 89 55 0C");
        if(hits.size()!=1)return false;
        int relative{};if(!gta::read(hits[0]+2,relative)||relative<=0||relative>512)return false;
        code=image.code(hits[0]+6);skip=code+relative;return true;
    }
    bool install(){
#ifdef GROVE_RENDERER_UNIT
        return false;
#else
        if(!code||!skip)return false;
        groveDamageSkip=reinterpret_cast<void*>(skip);
        supported=MH_CreateHook(reinterpret_cast<void*>(code),reinterpret_cast<void*>(groveDamageForceGuard),&groveDamageOriginal)==MH_OK;
        return supported;
#endif
    }
};
}
