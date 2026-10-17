#pragma once
// Code-drawn vector UI: no downloaded art, no world geometry or GTA HUD replacement.
static IDirect3DTexture9* driveFontTexture;
static D3DMATRIX uiView{},uiProjection{};static bool uiCamera{};
static float dialSpeed{},driveAcceleration{},lastDriveSpeed{};static ULONGLONG driveSample{};
static unsigned drawnInteractionKind{},drawnDeliveryKind{};
struct GroveCanvas {
    std::vector<OverlayVertex> shapes,glyphs;
    void triangle(float ax,float ay,float bx,float by,float cx,float cy,DWORD c){
        for(auto p:{std::pair<float,float>{ax,ay},{bx,by},{cx,cy}})shapes.push_back({p.first,p.second,0,1,c,0,0});
    }
    void rect(float x,float y,float w,float h,DWORD c){triangle(x,y,x+w,y,x,y+h,c);triangle(x,y+h,x+w,y,x+w,y+h,c);}
    void line(float x,float y,float xx,float yy,float width,DWORD c){
        float dx=xx-x,dy=yy-y,n=std::hypot(dx,dy);if(n<.001f)return;dx=dx/n*width*.5f;dy=dy/n*width*.5f;
        triangle(x-dy,y+dx,xx-dy,yy+dx,x+dy,y-dx,c);triangle(x+dy,y-dx,xx-dy,yy+dx,xx+dy,yy-dx,c);
    }
    void circle(float x,float y,float r,DWORD c,int count=40){
        for(int i=0;i<count;i++){float a=i*6.2831853f/count,b=(i+1)*6.2831853f/count;triangle(x,y,x+std::cos(a)*r,y+std::sin(a)*r,x+std::cos(b)*r,y+std::sin(b)*r,c);}
    }
    void ring(float x,float y,float r,float width,DWORD c,float start=0,float end=360,int count=48){
        for(int i=0;i<count;i++){float a=(start+(end-start)*i/count)*.0174532925f,b=(start+(end-start)*(i+1)/count)*.0174532925f;
            line(x+std::cos(a)*r,y+std::sin(a)*r,x+std::cos(b)*r,y+std::sin(b)*r,width,c);}
    }
    void text(float x,float y,const std::string& s,float height,DWORD color,bool centered=false){
        float width=height*.55f;if(centered)x-=s.size()*width*.5f;
        for(unsigned char c:s){if(c<32||c>126)c='?';unsigned index=c-32;float u=float(index%16)/16,v=float(index/16)/8;
            float u1=u+1.f/16,v1=v+1.f/8;float x1=x+width,y1=y+height;
            OverlayVertex q[]={{x,y,0,1,color,u,v},{x1,y,0,1,color,u1,v},{x,y1,0,1,color,u,v1},
                {x,y1,0,1,color,u,v1},{x1,y,0,1,color,u1,v},{x1,y1,0,1,color,u1,v1}};
            glyphs.insert(glyphs.end(),std::begin(q),std::end(q));x+=width;
        }
    }
    void icon(unsigned kind,float x,float y,float s,DWORD c){
        auto ln=[&](float a,float b,float d,float e){line(x+a*s,y+b*s,x+d*s,y+e*s,1.8f*s,c);};
        if(kind==1){ // Trade: emerald and two opposite arrows.
            ln(-6,0,0,-8);ln(0,-8,6,0);ln(6,0,0,8);ln(0,8,-6,0);
            ln(-11,-7,-7,-11);ln(-7,-11,2,-11);ln(7,11,11,7);ln(-2,11,7,11);
        }else if(kind==2){ln(-11,-6,-11,9);ln(11,0,11,9);ln(-11,4,11,4);ln(-11,-1,11,-1);ln(-7,-1,-7,-6);ln(-7,-6,-11,-6);
        }else if(kind==3){ln(-10,-4,10,-4);ln(-10,-1,10,-1);ln(-7,-1,-7,9);ln(7,-1,7,9);ln(0,-7,0,-12);ln(-5,-12,5,-12);
        }else if(kind==4){ln(-9,-10,9,-10);ln(9,-10,9,10);ln(9,10,-9,10);ln(-9,10,-9,-10);ln(-5,-5,5,-5);ln(-4,6,0,0);ln(0,0,4,6);ln(4,6,-4,6);
        }else{ln(-11,-7,11,-7);ln(11,-7,11,9);ln(11,9,-11,9);ln(-11,9,-11,-7);ln(-11,-1,11,-1);rect(x-2*s,y-3*s,4*s,6*s,c);}
    }
    void car(float x,float y,float s,DWORD color){
        // Side profile keeps the damage pictogram unmistakably a car at small sizes.
        auto ln=[&](float a,float b,float c,float d){line(x+a*s,y+b*s,x+c*s,y+d*s,2*s,color);};
        triangle(x-23*s,y-1*s,x+25*s,y-1*s,x-23*s,y+10*s,0xc01a2329);
        triangle(x-23*s,y+10*s,x+25*s,y-1*s,x+25*s,y+10*s,0xc01a2329);
        triangle(x-12*s,y-1*s,x-5*s,y-14*s,x+11*s,y-14*s,0xc01a2329);
        triangle(x-12*s,y-1*s,x+11*s,y-14*s,x+18*s,y-1*s,0xc01a2329);
        ln(-23,10,-23,0);ln(-23,0,-12,-2);ln(-12,-2,-5,-14);ln(-5,-14,11,-14);
        ln(11,-14,18,-2);ln(18,-2,25,1);ln(25,1,25,10);ln(-23,10,25,10);
        triangle(x-10*s,y-3*s,x-4*s,y-11*s,x-1*s,y-3*s,0xff8ba2ae);
        triangle(x-1*s,y-3*s,x-4*s,y-11*s,x-1*s,y-11*s,0xff8ba2ae);
        triangle(x+2*s,y-11*s,x+9*s,y-11*s,x+14*s,y-3*s,0xff8ba2ae);
        triangle(x+2*s,y-11*s,x+14*s,y-3*s,x+2*s,y-3*s,0xff8ba2ae);
        for(float xx:{-13.f,15.f}){circle(x+xx*s,y+10*s,6*s,0xff10181d,20);ring(x+xx*s,y+10*s,6*s,s,0xffb5c6cf,0,360,20);circle(x+xx*s,y+10*s,2*s,color,12);}
        rect(x+20*s,y+2*s,4*s,3*s,0xffe3e8c9);
    }
    void wrench(float x,float y,float s,DWORD color){
        line(x-10*s,y+10*s,x+7*s,y-7*s,5*s,color);
        ring(x-10*s,y+10*s,5*s,2*s,color,0,360,16);
        ring(x+8*s,y-8*s,8*s,3*s,color,35,300,16);
    }

};
static bool driveFont(IDirect3DDevice9* dev){
    if(driveFontTexture)return true;
    constexpr int w=256,h=256;HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO info{};
    info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    void* bits{};HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);if(!bitmap||!bits){if(bitmap)DeleteObject(bitmap);DeleteDC(dc);return false;}
    auto old=SelectObject(dc,bitmap);std::memset(bits,0,w*h*4);
    auto font=CreateFontA(-24,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,"Consolas");
    auto oldFont=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(255,255,255));
    for(int c=32;c<=126;c++){char letter=char(c);int i=c-32;RECT area{i%16*16,i/16*32,i%16*16+16,i/16*32+32};DrawTextA(dc,&letter,1,&area,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);}
    GdiFlush();bool ok=SUCCEEDED(dev->CreateTexture(w,h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&driveFontTexture,nullptr));
    if(ok){D3DLOCKED_RECT target{};ok=SUCCEEDED(driveFontTexture->LockRect(0,&target,nullptr,0));
        if(ok){auto* pixels=static_cast<DWORD*>(bits);for(int y=0;y<h;y++)for(int x=0;x<w;x++){DWORD c=pixels[y*w+x];unsigned a=std::max({c&255u,(c>>8)&255u,(c>>16)&255u});reinterpret_cast<DWORD*>(static_cast<char*>(target.pBits)+y*target.Pitch)[x]=(a<<24)|0xffffffu;}driveFontTexture->UnlockRect(0);}
        else{driveFontTexture->Release();driveFontTexture=nullptr;}}
    SelectObject(dc,oldFont);DeleteObject(font);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);return ok;
}
static bool uiProject(gta::Vec p,const D3DVIEWPORT9& vp,float& x,float& y){
    if(!uiCamera)return false;
    float a=p.x*uiView._11+p.y*uiView._21+p.z*uiView._31+uiView._41;
    float b=p.x*uiView._12+p.y*uiView._22+p.z*uiView._32+uiView._42;
    float c=p.x*uiView._13+p.y*uiView._23+p.z*uiView._33+uiView._43;
    float w=a*uiProjection._14+b*uiProjection._24+c*uiProjection._34+uiProjection._44;
    if(!std::isfinite(a+b+c+w)||w<=.1f)return false;
    x=vp.X+(1+(a*uiProjection._11+b*uiProjection._21+c*uiProjection._31+uiProjection._41)/w)*vp.Width*.5f;
    y=vp.Y+(1-(a*uiProjection._12+b*uiProjection._22+c*uiProjection._32+uiProjection._42)/w)*vp.Height*.5f;
    return std::isfinite(x+y)&&x>vp.X+50&&x<vp.X+vp.Width-50&&y>vp.Y+120&&y<vp.Y+vp.Height*.75f;
}
static void drawDriveHud(IDirect3DDevice9* dev){
    drawnInteractionKind=drawnDeliveryKind=0;
    if(!connected()||!playerLive||!playerSnapshot.valid||playerSnapshot.health<=0||playerSnapshot.menu||guestMenu||playerSnapshot.mission){driveSample=0;return;}
    D3DVIEWPORT9 vp{};dev->GetViewport(&vp);float s=std::clamp(std::min(vp.Width/800.f,vp.Height/600.f),.65f,1.4f);GroveCanvas ui;
    auto now=GetTickCount64();int speed{},condition{};float exact{};
    if(nativeActors.vehicles.trip(playerSnapshot,speed,condition,&exact)){
        float dt=driveSample?float(now-driveSample)/1000:0;
        if(dt>.5f||!driveSample){dialSpeed=lastDriveSpeed=exact;driveAcceleration=0;}
        else if(dt>.001f){float f=1-std::exp(-dt*9);dialSpeed+=(exact-dialSpeed)*f;
            float g=std::clamp((exact-lastDriveSpeed)/(3.6f*dt*9.80665f),-1.5f,1.5f);driveAcceleration+=(g-driveAcceleration)*(1-std::exp(-dt*4));}
        if(dt>.001f||!driveSample){lastDriveSpeed=exact;driveSample=now;}
        float x=vp.X+vp.Width-94*s,y=vp.Y+vp.Height*.64f,r=65*s;
        ui.circle(x,y,r+6*s,0xa8141414);ui.ring(x,y,r+6*s,1.5f*s,0xa0555555);
        ui.ring(x,y,r-3*s,3*s,0xff77776b,135,405);ui.ring(x,y,r-3*s,3*s,0xffff865f,360,405,12);
        for(int i=0;i<=24;i++){float a=(135+i*270.f/24)*.0174532925f,inner=r-(i%2?7:12)*s;
            ui.line(x+std::cos(a)*inner,y+std::sin(a)*inner,x+std::cos(a)*(r-3*s),y+std::sin(a)*(r-3*s),s,0xffdce9ed);
            if(i%4==0){char n[12];std::snprintf(n,sizeof n,"%d",i*10);ui.text(x+std::cos(a)*(r-23*s),y+std::sin(a)*(r-23*s)-5*s,n,10*s,0xffb4cbd5,true);}}
        float a=(135+std::clamp(dialSpeed/240,0.f,1.f)*270)*.0174532925f;
        ui.line(x-std::cos(a)*8*s,y-std::sin(a)*8*s,x+std::cos(a)*45*s,y+std::sin(a)*45*s,2.5f*s,0xffffd36a);ui.circle(x,y,4*s,0xffdfedf0,16);
        char number[12];std::snprintf(number,sizeof number,"%03d",int(std::lround(dialSpeed)));ui.text(x,y+22*s,number,21*s,0xfff0f8fa,true);ui.text(x,y+43*s,"KM/H",9*s,0xffbdbab2,true);
        DWORD color=condition<30?0xffff6f65:condition<60?0xffffc15e:0xff77b86b;
        ui.car(x-114*s,y-3*s,s*.82f,color);ui.text(x-114*s,y+30*s,"ESTADO",9*s,0xffb4cbd5,true);
        if(condition<60)ui.text(x-114*s,y+43*s,condition<30?"DETENTE":"REVISA MOTOR",9*s,color,true);
        ui.rect(x-48*s,y+57*s,96*s,3*s,0xff3d515c);ui.rect(x-1*s,y+54*s,2*s,9*s,0xffabc1cb);
        float force=std::clamp(driveAcceleration,-1.f,1.f)*47*s;ui.rect(x+std::min(0.f,force),y+57*s,std::abs(force),3*s,force<0?0xffffba65:0xffffd36a);
    }else driveSample=0;
    if(!playerSnapshot.vehicle){gta::Vec point{};float x{},y{};unsigned kind=0;std::string label;
        if(nativeActors.vehicles.cargoMarker(point)){kind=5;label="G EQUIPAJE  B CAJUELA  ALT+B SERVICIO";}
        else if(interactionHint.kind&&now-interactionHint.stamp<700){kind=interactionHint.kind;point=interactionHint.point;
            const char* names[]={"","J COMERCIAR","J DORMIR","J FABRICAR","J COCINAR","J ABRIR"};label=names[kind];
            if(kind==1&&std::hypot(point.x-playerSnapshot.position.x,point.y-playerSnapshot.position.y)>=3)label="ACERCATE PARA COMERCIAR";
            if(kind>1)point.z+=.9f;
        }
        if(kind){
            bool projected=uiProject(point,vp,x,y);
            if(!projected){x=vp.X+vp.Width*.65f;y=vp.Y+vp.Height*.64f;}
            // Upper-left mission card reserves a small area only.
            if(deliveryHint.kind&&now-deliveryHint.stamp<1500&&x<vp.X+300*s&&y<vp.Y+102*s){y=vp.Y+118*s;}
            ui.circle(x,y,15*s,0x95000000,32);ui.ring(x,y,15*s,1.4f*s,0xffffd36a);ui.icon(kind,x,y,s*.8f,0xfff0eee4);
            float width=label.size()*6.5f*s;ui.rect(x-width*.5f-7*s,y+20*s,width+14*s,19*s,0x95000000);ui.text(x,y+22*s,label,12*s,0xfff0eee4,true);drawnInteractionKind=kind;}
    }
    if(deliveryHint.kind&&now-deliveryHint.stamp<1500){
        float x=vp.X+18*s,y=vp.Y+28*s;auto p=deliveryHint.point;
        float distance=std::hypot(p.x-playerSnapshot.position.x,p.y-playerSnapshot.position.y);
        ui.rect(x,y,270*s,51*s,0x85000000);ui.rect(x,y,3*s,51*s,0xffffd36a);
        ui.line(x+14*s,y+12*s,x+19*s,y+7*s,s,0xffffd36a);ui.line(x+19*s,y+7*s,x+24*s,y+12*s,s,0xffffd36a);
        ui.line(x+24*s,y+12*s,x+19*s,y+17*s,s,0xffffd36a);ui.line(x+19*s,y+17*s,x+14*s,y+12*s,s,0xffffd36a);
        char title[80],progress[80];unsigned logs=deliveryHint.flags&255,wheat=(deliveryHint.flags>>8)&255;
        std::snprintf(title,sizeof title,"ENTREGA $%d  |  %dm",deliveryHint.kind==1?140:200,int(distance));
        if(deliveryHint.flags&(1u<<16))std::snprintf(progress,sizeof progress,"CONFIRMANDO PAGO...");
        else if(deliveryHint.kind==1)std::snprintf(progress,sizeof progress,"TRONCOS %u/16   TRIGO %u/8",logs,wheat);
        else std::snprintf(progress,sizeof progress,"TRONCOS %u/16  |  ENTREGA EN MERCADO",logs);
        float ratio=deliveryHint.kind==1?(logs+wheat)/24.f:logs/16.f;
        ui.text(x+32*s,y+5*s,title,13*s,0xffffd36a);ui.text(x+12*s,y+27*s,progress,10*s,0xfff0eee4);
        ui.rect(x+12*s,y+45*s,246*s,2*s,0xff444444);ui.rect(x+12*s,y+45*s,246*s*ratio,2*s,0xffffd36a);drawnDeliveryKind=deliveryHint.kind;
    }
    if(roadsidePhase&&now-roadsideStamp<1500){
        float x=vp.X+vp.Width*.5f,y=vp.Y+vp.Height*.71f;
        ui.rect(x-158*s,y,316*s,67*s,0xad101010);ui.rect(x-158*s,y,3*s,67*s,0xffffd36a);
        ui.wrench(x-128*s,y+29*s+(roadsidePhase==1?std::sin(now*.012f)*1.5f*s:0),s,0xffffd36a);
        char label[64];std::snprintf(label,sizeof label,roadsidePhase==1?"REPARANDO MOTOR  %u%%":roadsidePhase==2?"CONFIRMANDO REPARACION":"RECIBO EN REVISION",roadsideProgress);
        ui.text(x-99*s,y+9*s,label,11*s,0xffffd36a);ui.text(x-99*s,y+27*s,"3 HIERRO + 1 REDSTONE",10*s,0xfff0eee4);
        ui.rect(x-99*s,y+47*s,237*s,5*s,0xff444444);ui.rect(x-99*s,y+47*s,237*s*roadsideProgress/100.f,5*s,0xffffd36a);
        for(int i=1;i<5;i++)ui.rect(x-99*s+237*s*i/5,y+47*s,s,5*s,0xff161616);
        if(roadsidePhase==1){float spark=float(now%500)/500;for(int i=0;i<3;i++)ui.line(x-128*s,y+8*s,x-128*s+(i-1)*10*s*spark,y+8*s-9*s*spark,s,0x99ffe09a);}
    }
    if(ui.shapes.empty()&&ui.glyphs.empty())return;
    IDirect3DStateBlock9* saved{};if(FAILED(dev->CreateStateBlock(D3DSBT_ALL,&saved)))return;driveFont(dev);overlayState(dev);
    dev->SetTexture(0,nullptr);dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
    dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
    if(!ui.shapes.empty())dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,ui.shapes.size()/3,ui.shapes.data(),sizeof(OverlayVertex));
    if(driveFontTexture&&!ui.glyphs.empty()){overlayState(dev);dev->SetTexture(0,driveFontTexture);dev->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);dev->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
        dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,ui.glyphs.size()/3,ui.glyphs.data(),sizeof(OverlayVertex));}saved->Apply();saved->Release();
}
