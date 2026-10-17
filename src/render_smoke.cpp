// Synthetic D3D9 host. Never opens or controls either game.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdint>
#ifdef GROVE_RENDERER_UNIT
#include "bridge.cpp"
#endif
struct V {float x,y,z;DWORD color;};
int main(){
    WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"GroveCraftSmoke";
    RegisterClassW(&wc);
    HWND wnd=CreateWindowW(wc.lpszClassName,L"",WS_OVERLAPPED,0,0,128,128,nullptr,nullptr,wc.hInstance,nullptr);
    auto* d3d=Direct3DCreate9(D3D_SDK_VERSION);IDirect3DDevice9* dev=nullptr;
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.BackBufferWidth=128;pp.BackBufferHeight=128;
    pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=wnd;
    pp.EnableAutoDepthStencil=TRUE;pp.AutoDepthStencilFormat=D3DFMT_D24S8;
    if(!d3d||FAILED(d3d->CreateDevice(0,D3DDEVTYPE_HAL,wnd,D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED,&pp,&dev)))return 2;
#ifdef GROVE_RENDERER_UNIT
    self=GetModuleHandleW(nullptr);
    if(startTransport())return 3;
#else
    if(!LoadLibraryW(L"GroveCraft.asi"))return 3;
#endif
    Sleep(1500);
    HANDLE map=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,L"Local\\GroveCraft_v1");
    if(!map)return 4;
    auto* mem=static_cast<unsigned char*>(MapViewOfFile(map,FILE_MAP_ALL_ACCESS,0,0,0));if(!mem)return 5;
    DWORD pixels[3]{};bool restored=true,resetPassed=true,replacePassed=true;
    D3DMATRIX identity{};identity._11=identity._22=identity._33=identity._44=1;
    D3DMATRIX proj{};proj._11=proj._22=1;proj._33=20.0f/19;proj._34=1;proj._43=-20.0f/19;
    const V occluder[]={{-2,-2,2,0xffff0000},{2,-2,2,0xffff0000},{0,2,2,0xffff0000}};
    const ULONGLONG start=GetTickCount64();
    while(!*reinterpret_cast<volatile DWORD*>(mem+12) && GetTickCount64()-start<10000)Sleep(20);
    if(!*reinterpret_cast<volatile DWORD*>(mem+12))return 6;
    for(int phase=0;phase<3;phase++){
        for(int i=0;i<40;i++){
            dev->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0);
            dev->BeginScene();
            dev->SetTransform(D3DTS_WORLD,&identity);dev->SetTransform(D3DTS_VIEW,&identity);dev->SetTransform(D3DTS_PROJECTION,&proj);
            dev->SetVertexShader(nullptr);dev->SetPixelShader(nullptr);dev->SetTexture(0,nullptr);
            dev->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE);
            dev->SetRenderState(D3DRS_LIGHTING,FALSE);dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
            dev->SetRenderState(D3DRS_ZENABLE,TRUE);dev->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
            if(phase==1)dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,1,occluder,sizeof(V));
            // A native radar/environment pass may leave texture-coordinate generation active.
            dev->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACEPOSITION);
            dev->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT3);
#ifdef GROVE_RENDERER_UNIT
            renderFrame(dev); // Direct call in own process: no ASI, loader or hooks.
#endif
            dev->EndScene();
            DWORD fvf=0;dev->GetFVF(&fvf);if(fvf!=(D3DFVF_XYZ|D3DFVF_DIFFUSE))restored=false;
            DWORD coords{},flags{};dev->GetTextureStageState(0,D3DTSS_TEXCOORDINDEX,&coords);dev->GetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,&flags);
            if(coords!=D3DTSS_TCI_CAMERASPACEPOSITION||flags!=D3DTTFF_COUNT3)restored=false;
            if(i==39){
                IDirect3DSurface9 *rt=nullptr,*copy=nullptr;
                dev->GetRenderTarget(0,&rt);
                dev->CreateOffscreenPlainSurface(128,128,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&copy,nullptr);
                if(rt&&copy&&SUCCEEDED(dev->GetRenderTargetData(rt,copy))){
                    D3DLOCKED_RECT lr{};
                    if(SUCCEEDED(copy->LockRect(&lr,nullptr,D3DLOCK_READONLY))){pixels[phase]=*reinterpret_cast<DWORD*>(static_cast<char*>(lr.pBits)+64*lr.Pitch+64*4);copy->UnlockRect();}
                }
                if(rt)rt->Release();if(copy)copy->Release();
            }
            dev->Present(nullptr,nullptr,nullptr,nullptr);Sleep(25);
        }
        if(phase==0&&FAILED(dev->Reset(&pp)))resetPassed=false;
        if(phase==1){
            IDirect3DDevice9* replacement{};
            if(FAILED(d3d->CreateDevice(0,D3DDEVTYPE_HAL,wnd,D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED,&pp,&replacement)))replacePassed=false;
            else{dev->Release();dev=replacement;}
        }
    }
    bool valid=(pixels[0]&0xffffff)==0x00ff00 && (pixels[1]&0xffffff)==0xff0000 && (pixels[2]&0xffffff)==0x00ff00 && restored&&resetPassed&&replacePassed;
    FILE* out=std::fopen("smoke-result.json","w");
    if(out){std::fprintf(out,"{\"synthetic_only\":true,\"guest_pixel\":\"%08lx\",\"occluded_pixel\":\"%08lx\",\"replacement_pixel\":\"%08lx\",\"state_restored\":%s,\"device_reset\":%s,\"device_replaced\":%s,\"passed\":%s}\n",pixels[0],pixels[1],pixels[2],restored?"true":"false",resetPassed?"true":"false",replacePassed?"true":"false",valid?"true":"false");std::fclose(out);}
    // Process exit tears down the plugin's worker; never unload a live ASI.
    return valid?0:7;
}
