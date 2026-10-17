#pragma once
namespace grove {
/** Reuse the actual native world-pass fog state, not the HUD's disabled fog. */
class NativeWeather {
    DWORD enable{},colour{},start{},end{},density{},table{},vertex{},range{};
    ULONGLONG stamp{};
public:
    void capture(IDirect3DDevice9* dev){
        if(!dev)return;
        DWORD enabled{};if(FAILED(dev->GetRenderState(D3DRS_FOGENABLE,&enabled)))return;
        // Non-fogged alpha/shadow materials are not a change of weather.
        if(!enabled)return;
        dev->GetRenderState(D3DRS_FOGCOLOR,&colour);dev->GetRenderState(D3DRS_FOGSTART,&start);
        dev->GetRenderState(D3DRS_FOGEND,&end);dev->GetRenderState(D3DRS_FOGDENSITY,&density);
        dev->GetRenderState(D3DRS_FOGTABLEMODE,&table);dev->GetRenderState(D3DRS_FOGVERTEXMODE,&vertex);
        dev->GetRenderState(D3DRS_RANGEFOGENABLE,&range);enable=enabled;stamp=GetTickCount64();
    }
    void apply(IDirect3DDevice9* dev){
        bool fresh=stamp&&GetTickCount64()-stamp<250;
        dev->SetRenderState(D3DRS_FOGENABLE,fresh?enable:FALSE);
        if(!fresh)return;
        dev->SetRenderState(D3DRS_FOGCOLOR,colour);dev->SetRenderState(D3DRS_FOGSTART,start);
        dev->SetRenderState(D3DRS_FOGEND,end);dev->SetRenderState(D3DRS_FOGDENSITY,density);
        dev->SetRenderState(D3DRS_FOGTABLEMODE,table);dev->SetRenderState(D3DRS_FOGVERTEXMODE,vertex);
        dev->SetRenderState(D3DRS_RANGEFOGENABLE,range);
    }
};
}
