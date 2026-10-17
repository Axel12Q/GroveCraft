#pragma once
// Small diagnostic overlay using the user's installed Windows font, no bundled assets.
static IDirect3DTexture9* hudTexture;
static std::string hudText;
static void drawHud(IDirect3DDevice9* dev){
    drawDriveHud(dev);
    if(!debugHud)return;
    int speed{},condition{};bool trip=connected()&&nativeActors.vehicles.trip(playerSnapshot,speed,condition);
    bool cargo=connected()&&nativeActors.vehicles.cargoCandidate;
    if(!debugHud&&!trip&&!cargo)return;
    if(!playerSnapshot.valid||playerSnapshot.menu||guestMenu)return;
    const char* items[]={"mano vacia","tablones","piedra","tierra","vidrio","mesa","azada","semillas","pan","arbol","herramienta","comida","antorcha","cofre ender","bloque","espada","objeto","cubeta","agua","lava","arco/ballesta","escudo","tridente"};
    char text[512];
    std::snprintf(text,sizeof text,"%s | %u: %s x%d | comida %d/20 | picado %d%%\n%s",
        connected()?"GroveCraft":"esperando Minecraft",selectedSlot+1,items[std::min(itemType,22u)],itemCount,foodLevel,static_cast<int>(miningProgress*100),
        playerSnapshot.mission?"Mision: construir bloqueado | 1-9 seleccionar | R comer":playerSnapshot.vehicle?"Vehiculo: construir y comer bloqueados":
        nativeActors.vehicles.cargoCandidate?"G abrir equipaje | E inventario | coche estacionado":
        "Izquierdo picar/atacar | derecho usar/colocar/comer | E inventario");
    if(trip)std::snprintf(text,sizeof text,"%d km/h             MOTOR %d%%\n%s",speed,condition,
        condition<30?"Motor critico: busca un taller.":condition<60?"Conduce con cuidado: hay danos.":"Estaciona para usar el equipaje.");
    else if(cargo&&!debugHud)std::snprintf(text,sizeof text,"G Equipaje     Alt+G Taller     B Cajuela\nEsc cierra el inventario para ver el equipo.");
    int w=!debugHud?(trip?300:470):560;constexpr int h=44;
    if(hudText!=text || !hudTexture){
        if(hudTexture){hudTexture->Release();hudTexture=nullptr;}
        HDC dc=CreateCompatibleDC(nullptr);
        BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;
        info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
        void* bits{};HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);
        if(bitmap&&bits){
            auto oldBitmap=SelectObject(dc,bitmap);std::memset(bits,0,w*h*4);
            auto font=CreateFontA(-14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,"Tahoma");
            auto oldFont=SelectObject(dc,font);SetTextColor(dc,RGB(255,255,255));SetBkMode(dc,TRANSPARENT);
            RECT area{8,4,w-8,h-4};DrawTextA(dc,text,-1,&area,DT_LEFT|DT_NOPREFIX);
            GdiFlush();
            if(SUCCEEDED(dev->CreateTexture(w,h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&hudTexture,nullptr))){
                D3DLOCKED_RECT target{};
                if(SUCCEEDED(hudTexture->LockRect(0,&target,nullptr,0))){
                    auto* pixels=static_cast<DWORD*>(bits);
                    if(trip&&!debugHud){
                        DWORD gauge=condition<30?0x00e45a50:condition<60?0x00dfb653:0x0067c887;
                        int end=8+(w-16)*condition/100;
                        for(int y=h-5;y<h-2;y++)for(int x=8;x<w-8;x++)pixels[y*w+x]=x<end?gauge:0x002a3230;
                    }
                    for(int y=0;y<h;++y)for(int x=0;x<w;++x){auto c=pixels[y*w+x];
                        // Opaque dark backing and antialiased glyphs.
                        reinterpret_cast<DWORD*>(static_cast<char*>(target.pBits)+y*target.Pitch)[x]=0xd0000000|c;
                    }
                    hudTexture->UnlockRect(0);hudText=text;
                }
            }
            SelectObject(dc,oldFont);DeleteObject(font);SelectObject(dc,oldBitmap);
        }
        if(bitmap)DeleteObject(bitmap);DeleteDC(dc);
    }
    if(!hudTexture)return;
    IDirect3DStateBlock9* saved{};if(FAILED(dev->CreateStateBlock(D3DSBT_ALL,&saved)))return;
    struct H {float x,y,z,rhw;DWORD color;float u,v;};
    D3DVIEWPORT9 vp{};dev->GetViewport(&vp);float width=std::min<float>(w,vp.Width-16.0f);
    float barWidth=std::min<float>(hotbarW,std::min<float>(546,vp.Width*.75f));
    float barHeight=hotbarW?barWidth*hotbarH/hotbarW:88;
    float x=vp.X+(!debugHud&&trip?vp.Width-width-24:(vp.Width-width)*.5f)-.5f,y=std::max<float>(vp.Y+8,std::min<float>(vp.Y+vp.Height*.66f,vp.Y+vp.Height-barHeight-h-14));
    const H quad[]={{x,y,0,1,0xffffffff,0,0},{x+width,y,0,1,0xffffffff,1,0},{x,y+h,0,1,0xffffffff,0,1},{x+width,y+h,0,1,0xffffffff,1,1}};
    dev->SetVertexShader(nullptr);dev->SetPixelShader(nullptr);dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);dev->SetTexture(0,hudTexture);
    dev->SetRenderState(D3DRS_ZENABLE,FALSE);dev->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
    dev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);dev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
    dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);dev->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
    dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,quad,sizeof(H));saved->Apply();saved->Release();
}
