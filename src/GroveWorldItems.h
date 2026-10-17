#pragma once
// Draw the guest's real spinning drops and progressive vanilla crack textures.
static unsigned visibleDrops,visibleCracks;
// The hand bone's modelling axes are not CJ's forward axes. Remember its rest
// rotation, then apply animation deltas to a grip expressed in CJ's frame.
static D3DMATRIX heldRest{};
static D3DMATRIX heldRestBody{};
static std::uintptr_t heldRestPed{};
static bool heldRestValid{};
static D3DMATRIX heldFrame(const D3DMATRIX& bone){
    D3DMATRIX frame=bone;
    struct Basis {gta::Vec right;DWORD a;gta::Vec forward;DWORD b;gta::Vec up;DWORD c;};
    Basis ped{};
    if(!playerSnapshot.matrix||!gta::read(playerSnapshot.matrix,ped))return frame;
    bool action=guestMining||guestEating||guestUsing||GetTickCount64()<attackUntil||placeAnimation;
    if(!heldRestValid||heldRestPed!=playerSnapshot.ped||!action){heldRest=bone;heldRestPed=playerSnapshot.ped;heldRestValid=true;
        heldRestBody._11=ped.right.x;heldRestBody._12=ped.right.y;heldRestBody._13=ped.right.z;
        heldRestBody._21=ped.forward.x;heldRestBody._22=ped.forward.y;heldRestBody._23=ped.forward.z;
        heldRestBody._31=ped.up.x;heldRestBody._32=ped.up.y;heldRestBody._33=ped.up.z;
    }
    auto expected=[&](gta::Vec saved){
        float x=saved.x*heldRestBody._11+saved.y*heldRestBody._12+saved.z*heldRestBody._13;
        float y=saved.x*heldRestBody._21+saved.y*heldRestBody._22+saved.z*heldRestBody._23;
        float z=saved.x*heldRestBody._31+saved.y*heldRestBody._32+saved.z*heldRestBody._33;
        return gta::Vec{x*ped.right.x+y*ped.forward.x+z*ped.up.x,x*ped.right.y+y*ped.forward.y+z*ped.up.y,x*ped.right.z+y*ped.forward.z+z*ped.up.z};
    };
    auto ex=expected({heldRest._11,heldRest._12,heldRest._13});
    auto ey=expected({heldRest._21,heldRest._22,heldRest._23});
    auto ez=expected({heldRest._31,heldRest._32,heldRest._33});
    auto rotate=[&](gta::Vec v){
        float x=v.x*ex.x+v.y*ex.y+v.z*ex.z;
        float y=v.x*ey.x+v.y*ey.y+v.z*ey.z;
        float z=v.x*ez.x+v.y*ez.y+v.z*ez.z;
        return gta::Vec{x*bone._11+y*bone._21+z*bone._31,x*bone._12+y*bone._22+z*bone._32,x*bone._13+y*bone._23+z*bone._33};
    };
    auto r=rotate(ped.right),f=rotate(ped.forward),u=rotate(ped.up);
    frame._11=r.x;frame._12=r.y;frame._13=r.z;
    frame._21=f.x;frame._22=f.y;frame._23=f.z;
    frame._31=u.x;frame._32=u.y;frame._33=u.z;
    return frame;
}
static void drawWorldItems(IDirect3DDevice9* dev){
    visibleDrops=visibleCracks=0;nativeActors.vehicles.cargoProps=0;if(!atlas||!connected())return;
    auto* source=reinterpret_cast<volatile p::WorldEntities*>(mem+p::kOffWorldEntities);
    auto seq=source->seq;if(seq&1)return;p::WorldEntities state{};std::memcpy(&state,const_cast<const p::WorldEntities*>(source),sizeof state);
    MemoryBarrier();if(source->seq!=seq||state.count>p::kMaxWorldEntities)return;
    std::vector<Vertex> drops,cracks,hands;
    for(unsigned n=0;n<state.count;++n){auto e=state.entities[n];
        bool cargo=e.kind==104||e.kind==105;
        if(cargo){gta::Vec origin{};float yaw{};
            if(!nativeActors.vehicles.cargoPoint(e.id,origin,yaw))continue;
            e.x=origin.x;e.y=origin.z;e.z=-origin.y;e.yaw=yaw;e.kind=e.kind==104?p::kWeBlock:p::kWeItem;++nativeActors.vehicles.cargoProps;
        }
        bool held=e.kind==100||e.kind==101||e.kind==103;D3DMATRIX hand{};
        if(held&&(!itemCount||guestMenu||!nativeInteraction.matrix(playerSnapshot,hand)))continue;
        if(held){
            // Keep Minecraft's item plane in CJ's frame, attached to his animated hand.
            if(e.kind==103||e.ext[0]==20||e.ext[0]==23){
                struct Basis {gta::Vec right;DWORD a;gta::Vec forward;DWORD b;gta::Vec up;DWORD c;};Basis body{};
                if(!playerSnapshot.matrix||!gta::read(playerSnapshot.matrix,body))continue;
                hand._11=body.right.x;hand._12=body.right.y;hand._13=body.right.z;
                hand._21=body.forward.x;hand._22=body.forward.y;hand._23=body.forward.z;
                hand._31=body.up.x;hand._32=body.up.y;hand._33=body.up.z;
            }else hand=heldFrame(hand);
        }
        if(!held&&e.kind!=p::kWeBlock&&e.kind!=p::kWeItem&&e.kind!=p::kWeCrack&&e.kind!=102)continue;
        if(!std::isfinite(e.x)||!std::isfinite(e.y)||!std::isfinite(e.z)||!std::isfinite(e.yaw)||!std::isfinite(e.scale)
            ||std::abs(e.x)>16000||std::abs(e.y)>4096||std::abs(e.z)>16000||e.scale<=0||e.scale>4)continue;
        bool valid=true;for(const auto& uv:e.uv)for(float a:uv)valid=valid&&std::isfinite(a)&&a>=0&&a<=1;if(!valid)continue;
        bool crack=e.kind==p::kWeCrack||e.kind==102;auto& out=held?hands:crack?cracks:drops;
        auto quad=[&](gta::Vec a,gta::Vec b,gta::Vec c,gta::Vec d,const float* uv,DWORD color){
            auto vertex=[&](gta::Vec v,float u,float w){return held?Vertex{v.x,v.y,v.z,color,u,w}:Vertex{v.x,-v.z,v.y,color,u,w};};
            Vertex v[]={vertex(a,uv[0],uv[1]),vertex(b,uv[2],uv[1]),vertex(c,uv[0],uv[3]),vertex(c,uv[0],uv[3]),vertex(b,uv[2],uv[1]),vertex(d,uv[2],uv[3])};out.insert(out.end(),std::begin(v),std::end(v));
        };
        float angle=e.yaw*.0174532925f,cs=std::cos(angle),sn=std::sin(angle);
        auto point=[&](float x,float y,float z){
            if(held){
                if(e.kind==103){
                    y+=.10f;
                }else if(e.kind==101&&e.ext[0]==20){
                    float along=(x+y)*.70710678f,across=(y-x)*.70710678f;
                    x=.025f;z+=along;y=across+.08f;
                }else if(e.kind==101&&e.ext[0]==23){
                    z+=y*.25f;y*=.97f;
                }else if(e.kind==101&&(itemType==10||itemType==6||itemType==15||e.ext[0]==22)){
                    // Minecraft's bottom-left grip to top-right blade points
                    // ahead of CJ, with a small upward pitch and visible broadside.
                    float along=(x+y)*.70710678f,across=(x-y)*.70710678f;
                    x=across*.8f;z+=across*.6f+along*.34f;y=along*.94f+e.scale*.35f;
                }else if(e.kind==101){
                    z+=y*.8f;y*=.6f;
                }
                y+=.12f;return gta::Vec{hand._41+x*hand._11+y*hand._21+z*hand._31,
                hand._42+x*hand._12+y*hand._22+z*hand._32,hand._43+x*hand._13+y*hand._23+z*hand._33};}
            return gta::Vec{e.x+x*cs-z*sn,e.y+y,e.z+x*sn+z*cs};};
        if(e.kind==103){
            // Vanilla plate proportions: 12 wide, 22 tall, 1 deep.
            float w=e.scale*.5f,h=e.scale*22/24,t=e.scale/24;
            quad(point(-w,t,h),point(w,t,h),point(-w,t,-h),point(w,t,-h),e.uv[0],0xffffffff);
            quad(point(w,-t,h),point(-w,-t,h),point(w,-t,-h),point(-w,-t,-h),e.uv[1],0xffffffff);
            quad(point(-w,-t,h),point(-w,t,h),point(-w,-t,-h),point(-w,t,-h),e.uv[2],0xffffffff);
            quad(point(w,t,h),point(w,-t,h),point(w,t,-h),point(w,-t,-h),e.uv[2],0xffffffff);
            quad(point(-w,-t,h),point(w,-t,h),point(-w,t,h),point(w,t,h),e.uv[2],0xffffffff);
            quad(point(-w,t,-h),point(w,t,-h),point(-w,-t,-h),point(w,-t,-h),e.uv[2],0xffffffff);
            // Vanilla rear handle (2 x 6 x 6) reaches back to CJ's palm.
            float uPixel=(e.uv[0][2]-e.uv[0][0])/12,vPixel=(e.uv[0][3]-e.uv[0][1])/22;
            float handleUv[]={e.uv[0][0]+31*uPixel,e.uv[0][1]+5*vPixel,
                e.uv[0][0]+33*uPixel,e.uv[0][1]+11*vPixel};
            float hw=e.scale/12,hh=e.scale/4,back=-e.scale*.46f;
            quad(point(-hw,back,hh),point(hw,back,hh),point(-hw,back,-hh),point(hw,back,-hh),handleUv,0xffffffff);
            quad(point(-hw,-t,hh),point(-hw,back,hh),point(-hw,-t,-hh),point(-hw,back,-hh),handleUv,0xffffffff);
            quad(point(hw,back,hh),point(hw,-t,hh),point(hw,back,-hh),point(hw,-t,-hh),handleUv,0xffffffff);
            quad(point(-hw,back,hh),point(hw,back,hh),point(-hw,-t,hh),point(hw,-t,hh),handleUv,0xffffffff);
            quad(point(-hw,-t,-hh),point(hw,-t,-hh),point(-hw,back,-hh),point(hw,back,-hh),handleUv,0xffffffff);
            continue;
        }
        if(e.kind==102){
            float nx=e.ext[0],ny=e.ext[1],nz=e.ext[2];
            if(!std::isfinite(nx+ny+nz)||nx*nx+ny*ny+nz*nz<.5f||nx*nx+ny*ny+nz*nz>1.5f)continue;
            int axis=std::abs(ny)>=std::abs(nx)&&std::abs(ny)>=std::abs(nz)?1:std::abs(nx)>=std::abs(nz)?0:2;
            auto surface=[&](float a,float b){gta::Vec v{};
                if(axis==1){v.x=std::floor(e.x)+a-e.x;v.z=std::floor(e.z)+b-e.z;v.y=-(nx*v.x+nz*v.z)/ny;}
                else if(axis==0){v.y=std::floor(e.y)+a-e.y;v.z=std::floor(e.z)+b-e.z;v.x=-(ny*v.y+nz*v.z)/nx;}
                else{v.x=std::floor(e.x)+a-e.x;v.y=std::floor(e.y)+b-e.y;v.z=-(nx*v.x+ny*v.y)/nz;}
                return gta::Vec{e.x+v.x+nx*.008f,e.y+v.y+ny*.008f,e.z+v.z+nz*.008f};};
            quad(surface(0,0),surface(1,0),surface(0,1),surface(1,1),e.uv[0],0xffffffff);++visibleCracks;continue;
        }
        if(e.kind==p::kWeItem||e.kind==101){float h=e.scale*.5f;quad(point(-h,h,0),point(h,h,0),point(-h,-h,0),point(h,-h,0),e.uv[0],0xffffffff);if(!held)++visibleDrops;continue;}
        float x0=-e.scale*.5f,x1=-x0,y0=x0,y1=x1,z0=x0,z1=x1;
        if(crack){if(!std::isfinite(e.ext[0])||!std::isfinite(e.ext[1])||!std::isfinite(e.ext[2])||e.ext[0]<=0||e.ext[1]<=0||e.ext[2]<=0||e.ext[0]>16||e.ext[1]>16||e.ext[2]>16)continue;
            x0=y0=z0=0;x1=e.ext[0];y1=e.ext[1];z1=e.ext[2];++visibleCracks;
        }else if(!held&&!cargo)++visibleDrops;
        const float* side=e.uv[0];const float* top=e.uv[crack?0:1];const float* bottom=e.uv[crack?0:2];
        DWORD tint=e.tint?((e.tint&0xff00ff00)|((e.tint&0xff)<<16)|((e.tint>>16)&0xff)):0xffffffff;
        quad(point(x0,y1,z0),point(x1,y1,z0),point(x0,y0,z0),point(x1,y0,z0),side,0xffffffff);
        quad(point(x1,y1,z1),point(x0,y1,z1),point(x1,y0,z1),point(x0,y0,z1),side,0xffffffff);
        quad(point(x0,y1,z1),point(x0,y1,z0),point(x0,y0,z1),point(x0,y0,z0),side,0xffffffff);
        quad(point(x1,y1,z0),point(x1,y1,z1),point(x1,y0,z0),point(x1,y0,z1),side,0xffffffff);
        quad(point(x0,y1,z1),point(x1,y1,z1),point(x0,y1,z0),point(x1,y1,z0),top,tint);
        quad(point(x0,y0,z0),point(x1,y0,z0),point(x0,y0,z1),point(x1,y0,z1),bottom,0xffffffff);
    }
    // Solid held pixels must populate depth: mobs/armor draw later and would
    // otherwise overwrite the grip even when physically behind CJ's item.
    dev->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
    dev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);dev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    dev->SetRenderState(D3DRS_ALPHAREF,128);
    nativeLighting.shadeMesh(hands);nativeLighting.shadeMesh(drops);
    if(!hands.empty())dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,hands.size()/3,hands.data(),sizeof(Vertex));
    dev->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
    dev->SetRenderState(D3DRS_ALPHAREF,10);
    if(!drops.empty())dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,drops.size()/3,drops.data(),sizeof(Vertex));
    // Vanilla destroy-stage textures use white as unchanged and dark pixels as fractures.
    // Multiply the existing block, rather than painting the white background over it.
    dev->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
    dev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ZERO);dev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_SRCCOLOR);
    if(!cracks.empty())dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,cracks.size()/3,cracks.data(),sizeof(Vertex));
}
