#define GROVE_RENDERER_UNIT
#include "bridge.cpp"
#include <cassert>
int main(){
    WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"GroveArmorTextureTest";
    RegisterClassW(&wc);HWND window=CreateWindowW(wc.lpszClassName,L"",WS_OVERLAPPED,0,0,128,128,nullptr,nullptr,wc.hInstance,nullptr);
    auto* api=Direct3DCreate9(D3D_SDK_VERSION);assert(api);IDirect3DDevice9* dev{};
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window;
    pp.BackBufferWidth=128;pp.BackBufferHeight=128;pp.BackBufferFormat=D3DFMT_A8R8G8B8;
    pp.EnableAutoDepthStencil=TRUE;pp.AutoDepthStencilFormat=D3DFMT_D24S8;
    assert(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&dev)));
    grove::Scene emptyWorld;std::vector<unsigned char> packet(sizeof(p::RenTexture)+16,255);
    p::RenTexture header{9,2,2};std::memcpy(packet.data(),&header,sizeof header);
    assert(emptyWorld.texture(packet.data(),packet.size())&&!emptyWorld.textureGpu(9));
    // Equipment arrives even when the entity scene is empty or stale.
    emptyWorld.draw(dev,nullptr);assert(emptyWorld.vertexCount()==0&&emptyWorld.textureGpu(9));
    emptyWorld.resetDevice();assert(!emptyWorld.textureGpu(9));
    emptyWorld.draw(dev,nullptr);assert(emptyWorld.textureGpu(9));
    emptyWorld.clear();dev->Release();api->Release();DestroyWindow(window);
}
