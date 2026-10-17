#pragma once
// Crop Minecraft's own GPU-rendered hotbar and hunger row. No extracted assets or fake counts.
static IDirect3DTexture9* hotbarTexture;
static IDirect3DTexture9* menuTexture;
static unsigned menuW,menuH;
static unsigned overlayFront=2,hotbarW{},hotbarH{};
static unsigned overlayGuiScale;
static ULONGLONG lastOverlay;
static std::uint64_t overlayFrame;
static void consumeOverlay(IDirect3DDevice9* dev){
    auto now=GetTickCount64();if(now-lastOverlay<50)return;lastOverlay=now;
    auto* state=reinterpret_cast<volatile LONG*>(mem+p::kOffOverlayCtl);
    LONG current=InterlockedCompareExchange(state,0,0);
    if(!(current&p::kOverlayDirty)||(current&3)>2)return;
    LONG old=InterlockedExchange(state,overlayFront);overlayFront=old&3;
    if(overlayFront>2)return;
    p::OverlaySlotHdr header{};std::memcpy(&header,mem+p::kOffOverlaySlotHdr+overlayFront*0x40,sizeof header);
    if(!header.width||header.width>p::kMaxOverlayW||!header.height||header.height>p::kMaxOverlayH)return;
    unsigned scale=overlayGuiScale;
    for(unsigned attempt=0;attempt<3;++attempt){
        auto* shared=reinterpret_cast<volatile LONG*>(mem+p::kOffMcState);
        LONG first=InterlockedCompareExchange(shared,0,0);if(first&1)continue;
        unsigned candidate{};std::memcpy(&candidate,mem+p::kOffMcState+52,4);MemoryBarrier();
        if(first==InterlockedCompareExchange(shared,0,0)&&candidate>=1&&candidate<=8){scale=overlayGuiScale=candidate;break;}
    }
    if(!scale)return;
    unsigned width=182*scale,height=54*scale;
    if(width>header.width||height>header.height)return;
    if(!hotbarTexture||hotbarW!=width||hotbarH!=height){
        if(hotbarTexture){hotbarTexture->Release();hotbarTexture=nullptr;}
        if(FAILED(dev->CreateTexture(width,height,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&hotbarTexture,nullptr)))return;
        hotbarW=width;hotbarH=height;
    }
    D3DLOCKED_RECT target{};if(FAILED(hotbarTexture->LockRect(0,&target,nullptr,0)))return;
    auto* source=mem+p::kOffOverlayPixels+overlayFront*p::kOverlaySlotBytes;
    if(guestMenu){
        if(!menuTexture||menuW!=header.width||menuH!=header.height){
            if(menuTexture){menuTexture->Release();menuTexture=nullptr;}
            if(SUCCEEDED(dev->CreateTexture(header.width,header.height,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&menuTexture,nullptr))){menuW=header.width;menuH=header.height;}
        }
        D3DLOCKED_RECT menu{};
        if(menuTexture&&SUCCEEDED(menuTexture->LockRect(0,&menu,nullptr,0))){
            for(unsigned y=0;y<header.height;++y){auto* row=reinterpret_cast<DWORD*>(static_cast<char*>(menu.pBits)+y*menu.Pitch);
                unsigned sy=(header.flags&1)?header.height-1-y:y;
                for(unsigned x=0;x<header.width;++x){auto* pixel=source+(std::size_t(sy)*header.width+x)*4;
                    bool panel=x>=menuLeft&&x<menuLeft+menuWidth&&y>=menuTop&&y<menuTop+menuHeight;
                    unsigned alpha=panel||unsigned(pixel[0])+pixel[1]+pixel[2]>120?pixel[3]:0;
                    if(y>menuTop+menuHeight+20&&!panel)alpha=0;
                    row[x]=(alpha<<24)|(DWORD(pixel[0])<<16)|(DWORD(pixel[1])<<8)|pixel[2];
                }
            }menuTexture->UnlockRect(0);
        }
    }
    unsigned left=(header.width-width)/2;
    for(unsigned y=0;y<height;++y){
        auto* row=reinterpret_cast<DWORD*>(static_cast<char*>(target.pBits)+y*target.Pitch);
        unsigned logicalY=header.height-height+y;
        unsigned sourceY=(header.flags&1)?header.height-1-logicalY:logicalY;
        for(unsigned x=0;x<width;++x){
            bool bar=y>=height-22*scale;
            bool hunger=x>=101*scale&&x<182*scale&&y>=15*scale&&y<24*scale;
            bool protection=x<81*scale&&y>=5*scale&&y<14*scale;
            if(!bar&&!hunger&&!protection){row[x]=0;continue;}
            auto* pixel=source+(std::size_t(sourceY)*header.width+left+x)*4;
            row[x]=(DWORD(pixel[3])<<24)|(DWORD(pixel[0])<<16)|(DWORD(pixel[1])<<8)|pixel[2];
        }
    }
    hotbarTexture->UnlockRect(0);overlayFrame=header.frameId;
}
struct OverlayVertex {float x,y,z,rhw;DWORD color;float u,v;};
static void overlayState(IDirect3DDevice9* dev){
    dev->SetVertexShader(nullptr);dev->SetPixelShader(nullptr);
    dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
    dev->SetRenderState(D3DRS_ZENABLE,FALSE);dev->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);dev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
    dev->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
    dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);dev->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);
    dev->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
    dev->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT);dev->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
}
#include "GroveCJPreview.h"
static void drawHotbar(IDirect3DDevice9* dev){
    if(!hotbarTexture||!connected()||!playerSnapshot.valid||playerSnapshot.menu)return;
    IDirect3DStateBlock9* saved{};if(FAILED(dev->CreateStateBlock(D3DSBT_ALL,&saved)))return;
    D3DVIEWPORT9 vp{};dev->GetViewport(&vp);
    if(guestMenu&&menuTexture){
        float x=vp.X-.5f,y=vp.Y-.5f;OverlayVertex menu[]={{x,y,0,1,0xffffffff,0,0},{x+vp.Width,y,0,1,0xffffffff,1,0},
            {x,y+vp.Height,0,1,0xffffffff,0,1},{x+vp.Width,y+vp.Height,0,1,0xffffffff,1,1}};
        overlayState(dev);dev->SetTexture(0,menuTexture);dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,menu,sizeof(OverlayVertex));
        drawCJPreview(dev);overlayState(dev);
        struct Cursor {float x,y,z,rhw;DWORD color;};
        float cx=vp.X+guiCursorX,cy=vp.Y+guiCursorY;
        Cursor cursor[]={{cx,cy,0,1,0xffffffff},{cx+4,cy+12,0,1,0xffffffff},{cx+9,cy+7,0,1,0xffffffff}};
        dev->SetTexture(0,nullptr);dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
        dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
        dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
        dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,1,cursor,sizeof(Cursor));
        saved->Apply();saved->Release();return;
    }
    float width=std::min<float>(hotbarW,std::min<float>(546,vp.Width*.75f));
    float height=width*hotbarH/hotbarW,x=vp.X+(vp.Width-width)*.5f-.5f,y=vp.Y+vp.Height-height-10-.5f;
    OverlayVertex vertices[]={{x,y,0,1,0xffffffff,0,0},{x+width,y,0,1,0xffffffff,1,0},
        {x,y+height,0,1,0xffffffff,0,1},{x+width,y+height,0,1,0xffffffff,1,1}};
    overlayState(dev);dev->SetTexture(0,hotbarTexture);dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(OverlayVertex));
    saved->Apply();saved->Release();
}
static void drawAim(IDirect3DDevice9* dev){
    if(!buildMode||guestMenu||!playerSnapshot.valid||playerSnapshot.menu||playerSnapshot.vehicle||playerSnapshot.mission)return;
    IDirect3DStateBlock9* saved{};if(FAILED(dev->CreateStateBlock(D3DSBT_ALL,&saved)))return;
    D3DMATRIX previous{},identity{};dev->GetTransform(D3DTS_WORLD,&previous);identity._11=identity._22=identity._33=identity._44=1;
    dev->SetTransform(D3DTS_WORLD,&identity);dev->SetVertexShader(nullptr);dev->SetPixelShader(nullptr);
    dev->SetTexture(0,nullptr);dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
    dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);dev->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
    dev->SetRenderState(D3DRS_LIGHTING,FALSE);dev->SetRenderState(D3DRS_FOGENABLE,FALSE);
    dev->SetRenderState(D3DRS_ZENABLE,TRUE);dev->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);dev->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
    D3DVIEWPORT9 vp{};dev->GetViewport(&vp);float x=vp.X+vp.Width*.65f-.5f,y=vp.Y+vp.Height*.55f-.5f;
    DWORD color=buildTarget.valid?0xffffffff:0xffff7777;
    struct Cross {float x,y,z,rhw;DWORD color;};Cross cross[]={{x-6,y,0,1,color},{x+6,y,0,1,color},{x,y-6,0,1,color},{x,y+6,0,1,color}};
    dev->SetRenderState(D3DRS_ZENABLE,FALSE);dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
    dev->DrawPrimitiveUP(D3DPT_LINELIST,2,cross,sizeof(Cross));saved->Apply();saved->Release();dev->SetTransform(D3DTS_WORLD,&previous);
}
