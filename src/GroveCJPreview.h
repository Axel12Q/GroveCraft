#pragma once
#include "CJPreviewSkin.h"
// Read CJ's live merged RenderWare geometry and materials. It automatically follows
// his current clothes/body shape; no DFF/TXD files are extracted or redistributed.
static void drawCJPreview(IDirect3DDevice9* dev){
    previewTriangles=previewArmorTriangles=0;
    if(!cjPreview||!guestMenu||GetTickCount64()-previewStamp>600||!playerSnapshot.valid)return;
    struct Triangle {OverlayVertex v[3];IDirect3DTexture9* texture{};float depth{};};
    std::vector<Triangle> triangles;float minZ=1e9f,maxZ=-1e9f;
    cjPreviewSkin.prepare();
    DWORD clump{},link{};if(!gta::read(playerSnapshot.ped+0x18,clump)||!gta::read(clump+8,link))return;
    auto head=clump+8;
    for(unsigned atomicCount=0;link!=head&&atomicCount<16;++atomicCount){
        if(link<64)return;DWORD geom{},next{};if(!gta::read(link,next)||!gta::read(link-64+24,geom))return;link=next;
        int nt{},nv{},nm{};DWORD morph{},verts{},uv{},faces{},materials{};
        if(!gta::read(geom+16,nt)||!gta::read(geom+20,nv)||nt<1||nt>20000||nv<1||nv>12000
            ||!gta::read(geom+92,morph)||!gta::read(morph+20,verts)||!gta::read(geom+52,uv)
            ||!gta::read(geom+44,faces)||!gta::read(geom+32,materials)||!gta::read(geom+36,nm)||nm<1||nm>32)return;
        std::vector<IDirect3DTexture9*> textures(nm);
        for(int m=0;m<nm;++m){DWORD material{},texture{},raster{},gpu{},table{};
            if(gta::read(materials+m*4,material)&&gta::read(material,texture)&&gta::read(texture,raster)
                &&gta::read(raster+0x34,gpu)&&gta::read(gpu,table)&&gta::accessible(reinterpret_cast<void*>(table),22*4)){
                // The x86 D3D9 raster extension's first member is its texture; verified
                // from this owned Steam build and CJ's live torso/head/legs/feet rasters.
                textures[m]=reinterpret_cast<IDirect3DTexture9*>(gpu);
            }
        }
        if(!gta::accessible(reinterpret_cast<void*>(verts),nv*12)||!gta::accessible(reinterpret_cast<void*>(uv),nv*8)||!gta::accessible(reinterpret_cast<void*>(faces),nt*8))return;
        std::vector<gta::Vec> posed(nv);for(int i=0;i<nv;++i){gta::Vec rest{};std::memcpy(&rest,reinterpret_cast<void*>(verts+i*12),12);posed[i]=cjPreviewSkin.pose(rest);}
        for(int n=0;n<nt;++n){unsigned short ids[4]{};std::memcpy(ids,reinterpret_cast<void*>(faces+n*8),8);if(ids[3]>=nm)return;
            Triangle tri;tri.texture=textures[ids[3]];if(!tri.texture)continue;
            for(int k=0;k<3;++k){gta::Vec p{};float tex[2]{};
                if(ids[k]>=nv)return;p=posed[ids[k]];std::memcpy(tex,reinterpret_cast<void*>(uv+ids[k]*8),8);
                if(!std::isfinite(p.x+p.y+p.z+tex[0]+tex[1])||std::abs(p.x)+std::abs(p.y)+std::abs(p.z)>5)return;
                minZ=std::min(minZ,p.z);maxZ=std::max(maxZ,p.z);
                tri.v[k]={p.x,p.y,p.z,1,0xffffffff,tex[0],tex[1]};
            }triangles.push_back(tri);
        }
    }
    if(triangles.empty()||maxZ-minZ<1||maxZ-minZ>3)return;
    previewTriangles=triangles.size();
    std::array<D3DMATRIX,55> armorPose{};std::array<bool,55> poseValid{};
    for(unsigned i=0;i<55;++i)poseValid[i]=cjPreviewSkin.matrix(i,armorPose[i]);
    for(std::size_t bi=0;bi<armor.batches.size();++bi){const auto& b=armor.batches[bi];auto* texture=scene.textureGpu(b.texture);if(!texture)continue;
        bool fitted=armor.fit.ready&&bi<armor.fitted.size();unsigned count=fitted?armor.fitted[bi].size():b.count;
        for(unsigned n=0;n<count;n+=3){Triangle t;t.texture=texture;
            for(unsigned k=0;k<3;++k){auto v=fitted?armor.fitted[bi][n+k].source:armor.vertices[b.first+n+k];
                auto p=fitted?armor.fitted[bi][n+k].sample.rest:armor.rest(v,(b.flags>>8)&15);
                if(fitted){const auto& sample=armor.fitted[bi][n+k].sample;bool complete=true;
                    for(const auto& influence:sample.skin)complete&=influence.bone<55&&poseValid[influence.bone];
                    if(complete)p=cjPreviewSkin.local(grove::ArmorFit::posed(sample,armorPose));}
                DWORD c=(v.color&0xff00ff00)|((v.color&0xff)<<16)|((v.color>>16)&0xff);
                t.v[k]={p.x,p.y,p.z,1,c,v.u,v.v};}
            triangles.push_back(t);++previewArmorTriangles;
        }
    }
    float cx=previewLeft+previewWidth*.5f,cy=previewTop+previewHeight*.5f;
    float yaw=std::atan((guiCursorX-cx)/80.f)*.40f,pitch=std::atan((guiCursorY-cy)/100.f)*.15f;
    float size=std::min(previewHeight*.88f/(maxZ-minZ),previewWidth/.95f);
    for(auto& tri:triangles){
        gta::Vec points[3];for(int k=0;k<3;++k){auto& v=tri.v[k];float turn=yaw+(v.z>.40f?yaw*.85f:0);
            float x=v.x*std::cos(turn)-v.y*std::sin(turn),y=v.x*std::sin(turn)+v.y*std::cos(turn);
            float z=(v.z-minZ),depth=x*std::cos(pitch)+z*std::sin(pitch);z=z*std::cos(pitch)-x*std::sin(pitch);
            points[k]={x,y,z};tri.depth+=depth;
            v.x=cx-y*size;v.y=previewTop+previewHeight*.95f-z*size;v.z=std::clamp(.5f-depth*.1f,.05f,.95f);v.rhw=1;
        }
        auto normal=grove::unit(grove::cross(grove::sub(points[1],points[0]),grove::sub(points[2],points[0])));
        DWORD light=DWORD(180+75*std::abs(normal.x));
        for(auto& v:tri.v){DWORD color=v.color;v.color=(color&0xff000000)|((((color>>16)&255)*light/255)<<16)|((((color>>8)&255)*light/255)<<8)|((color&255)*light/255);}
    }
    std::stable_sort(triangles.begin(),triangles.end(),[](const auto& a,const auto& b){return std::less<IDirect3DTexture9*>{}(a.texture,b.texture);});
    RECT clip{LONG(previewLeft),LONG(previewTop),LONG(previewLeft+previewWidth),LONG(previewTop+previewHeight)};
    dev->SetScissorRect(&clip);dev->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);
    overlayState(dev);dev->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);dev->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
    D3DRECT depthRect{clip.left,clip.top,clip.right,clip.bottom};
    dev->Clear(1,&depthRect,D3DCLEAR_ZBUFFER,0,1,0);dev->SetRenderState(D3DRS_ZENABLE,TRUE);dev->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
    dev->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);dev->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
    dev->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);dev->SetRenderState(D3DRS_ALPHAREF,128);dev->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATEREQUAL);
    std::vector<OverlayVertex> run;IDirect3DTexture9* texture{};
    auto flush=[&](){if(run.empty())return;dev->SetTexture(0,texture);dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,run.size()/3,run.data(),sizeof(OverlayVertex));run.clear();};
    for(auto& t:triangles){if(texture!=t.texture){flush();texture=t.texture;}run.insert(run.end(),std::begin(t.v),std::end(t.v));}flush();
    dev->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);
}
