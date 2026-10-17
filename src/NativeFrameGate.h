#pragma once
namespace grove {
struct FrameGate {
    std::uintptr_t hudCode{},sprintCode{};
    void init(){
        gta::OriginalImage image;if(!image.load())return;
        auto h=image.scan("80 3D ? ? ? ? 01 0F 84 ? ? ? ? 80 3D ? ? ? ? 00 0F 85 ? ? ? ? 80 3D ? ? ? ? 00 0F 85 ? ? ? ? A1 ? ? ? ? 3B 05 ? ? ? ? 0F 84 ? ? ? ? 8B 0D ? ? ? ? 8B 51 20 6A 01 6A 09 FF D2");
        auto s=image.scan("66 83 B9 0E 01 00 00 00 75 25 0F B7 81 0A 01 00 00 83 F8 03 77 19 FF 24 85 ? ? ? ? 66 83 79 20 00 0F 95 C0 C3 66 83 79 22 00 0F 95 C0 C3");
        if(h.size()==1)hudCode=image.code(h[0]);if(s.size()==1)sprintCode=image.code(s[0]);
    }
    static bool mainTarget(IDirect3DDevice9* d){
        IDirect3DSurface9 *target{},*back{};D3DVIEWPORT9 vp{};D3DSURFACE_DESC desc{};
        bool ok=SUCCEEDED(d->GetRenderTarget(0,&target))&&SUCCEEDED(d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back))
            &&target==back&&SUCCEEDED(back->GetDesc(&desc))&&SUCCEEDED(d->GetViewport(&vp))
            // Letterboxing still uses the main backbuffer. Reject small HUD/map
            // viewports without losing fresh-frame updates in a cinematic viewport.
            &&vp.Width>0&&vp.Height>0&&vp.Width>=desc.Width*3/4&&vp.Height>=desc.Height*3/4
            &&vp.Width<=desc.Width&&vp.Height<=desc.Height
            &&vp.X<=desc.Width-vp.Width&&vp.Y<=desc.Height-vp.Height;
        if(target)target->Release();if(back)back->Release();return ok;
    }
};
}
