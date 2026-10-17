#pragma once
// Vanilla equipment meshes, posed on the native player's current skeletal joints.
// No stock model, animation, clothing texture or native armour value is changed.
namespace grove {
inline gta::Vec sub(gta::Vec a,gta::Vec b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline gta::Vec cross(gta::Vec a,gta::Vec b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline float length(gta::Vec a){return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);}
inline gta::Vec unit(gta::Vec a){float n=length(a);return n>.001f?gta::Vec{a.x/n,a.y/n,a.z/n}:gta::Vec{};}
}
#include "ArmorFit.h"
namespace grove {
class Armor {
public:
    ArmorFit fit;
    struct FittedVertex {ArmorSample sample;p::RenVertex source;};
    std::vector<std::vector<FittedVertex>> fitted;
    std::vector<p::RenVertex> vertices;
    std::vector<p::RenBatch> batches;
    unsigned rejected{},drawn{};
    std::array<float,6> poseBounds{};
    unsigned missingBone{};
    ULONGLONG stamp{};
    void clear(){vertices.clear();batches.clear();fitted.clear();stamp=0;}
    void prepare(){
        fitted.clear();if(!fit.ready)return;fitted.resize(batches.size());
        for(std::size_t bi=0;bi<batches.size();++bi){auto b=batches[bi];unsigned part=(b.flags>>8)&15;
            unsigned divisions=part==1?1:4;
            // A malformed oversized guest model must not amplify the draw budget.
            if(vertices.size()>6000)divisions=1;
            auto& out=fitted[bi];out.reserve(b.count*divisions*divisions);
            for(unsigned n=b.first;n<b.first+b.count;n+=3){auto a=vertices[n],c=vertices[n+1],d=vertices[n+2];
                auto emit=[&](unsigned x,unsigned y){float u=float(x)/divisions,v=float(y)/divisions,w=1-u-v;
                    p::RenVertex p=a;p.x=w*a.x+u*c.x+v*d.x;p.y=w*a.y+u*c.y+v*d.y;p.z=w*a.z+u*c.z+v*d.z;
                    p.u=w*a.u+u*c.u+v*d.u;p.v=w*a.v+u*c.v+v*d.v;
                    out.push_back({fit.sample({p.x,p.y,p.z},part),p});};
                for(unsigned y=0;y<divisions;++y)for(unsigned x=0;x<divisions-y;++x){
                    emit(x,y);emit(x+1,y);emit(x,y+1);
                    if(x+y+1<divisions){emit(x+1,y);emit(x+1,y+1);emit(x,y+1);}
                }
            }
        }
    }
    bool frame(const unsigned char* data,std::size_t bytes){
        if(bytes<8)return false;DWORD nb{},nv{};std::memcpy(&nb,data,4);std::memcpy(&nv,data+4,4);
        if(nb>128||nv>80000||nv%3||8ull+nb*sizeof(p::RenBatch)+nv*sizeof(p::RenVertex)!=bytes)return false;
        std::vector<p::RenBatch> b(nb);std::memcpy(b.data(),data+8,nb*sizeof(p::RenBatch));
        unsigned covered=0;
        for(auto v:b){unsigned part=(v.flags>>8)&15;
            if(part<1||part>6||(v.flags&~0xf01u)||v.first!=covered||v.first>nv||v.count>nv-v.first||v.count%3)return false;
            covered+=v.count;}
        if(covered!=nv)return false;
        std::vector<p::RenVertex> v(nv);std::memcpy(v.data(),data+8+nb*sizeof(p::RenBatch),nv*sizeof(p::RenVertex));
        for(auto a:v)if(!std::isfinite(a.x+a.y+a.z+a.u+a.v)||std::abs(a.x)>4||std::abs(a.y)>4||std::abs(a.z)>4)return false;
        vertices.swap(v);batches.swap(b);prepare();stamp=GetTickCount64();return true;
    }
    // Body coordinates from the standing vanilla model, with no held items or skin.
    gta::Vec rest(const p::RenVertex& v,unsigned part)const{
        if(fit.ready)return fit.sample({v.x,v.y,v.z},part).rest;
        float z=v.y<.75f?-1.04f+v.y*1.3866667f:v.y<1.5f?(v.y-.75f)*.6666667f:.5f+(v.y-1.5f)*.54f;
        if(part==3||part==4){
            // CJ's merged rest pose spreads his arms, unlike the vanilla doll.
            float side=part==3?1.f:-1.f,drop=(1.375f-v.y)*.8f;
            return {v.z*.85f,side*(.23f+drop*.45f)-(v.x-(part==3?-.375f:.375f))*.8f,.45f-drop*.90f};
        }
        return {v.z*(part==1?.6f:.85f),-v.x*(part==1?.6f:.9f),z};
    }
    bool pose(unsigned part,const gta::Snapshot& s,NativeInteraction& bones,gta::Vec& origin,gta::Vec& right,gta::Vec& forward,gta::Vec& down,float& stretch){
        struct Basis {gta::Vec right;DWORD a;gta::Vec forward;DWORD b;gta::Vec up;DWORD c;};Basis body{};
        if(!gta::read(s.matrix,body))return false;
        D3DMATRIX a{},b{};int root[]={0,5,2,22,32,41,51},tip[]={0,5,4,24,34,43,53};
        if(!bones.boneMatrix(s,root[part],a)||!bones.boneMatrix(s,tip[part],b))return false;
        origin={a._41,a._42,a._43};right=body.right;forward=body.forward;down={-body.up.x,-body.up.y,-body.up.z};stretch=1;
        if(part==1){
            origin.z+=.04f;stretch=.6f;
            gta::Vec axes[]={{a._11,a._12,a._13},{a._21,a._22,a._23},{a._31,a._32,a._33}};
            auto dot=[](gta::Vec x,gta::Vec y){return x.x*y.x+x.y*y.y+x.z*y.z;};
            int up=0;for(int i=1;i<3;++i)if(std::abs(dot(axes[i],body.up))>std::abs(dot(axes[up],body.up)))up=i;
            auto u=unit(axes[up]);if(dot(u,body.up)<0)u={-u.x,-u.y,-u.z};
            int front=(up+1)%3;int other=(up+2)%3;if(std::abs(dot(axes[other],body.forward))>std::abs(dot(axes[front],body.forward)))front=other;
            forward=unit(axes[front]);if(dot(forward,body.forward)<0)forward={-forward.x,-forward.y,-forward.z};
            down={-u.x,-u.y,-u.z};right=unit(cross(down,forward));forward=unit(cross(right,down));return true;
        }
        gta::Vec target{b._41,b._42,b._43};auto delta=sub(target,origin);float n=length(delta);if(n<.12f||n>1.5f)return false;
        down=unit(delta);if(part==2)down={-down.x,-down.y,-down.z};
        right=unit(cross(down,forward));forward=unit(cross(right,down));
        stretch=n/(part==2?.75f:part>=5?.75f:.75f);return true;
    }
    void draw(IDirect3DDevice9* dev,Scene& scene,const gta::Snapshot& s,NativeInteraction& bones){
        drawn=0;if(!stamp||GetTickCount64()-stamp>3000||!s.valid||s.menu)return;
        if(fit.ready){
            poseBounds={1e9f,1e9f,1e9f,-1e9f,-1e9f,-1e9f};missingBone=0;
            std::array<D3DMATRIX,55> matrices{};std::array<bool,55> used{},valid{};
            for(const auto& batch:fitted)for(const auto& v:batch)for(const auto& skin:v.sample.skin)used[skin.bone]=true;
            for(unsigned i=0;i<55;++i)if(used[i])valid[i]=bones.boneMatrix(s,i,matrices[i]);
            for(std::size_t bi=0;bi<fitted.size();++bi){auto* texture=scene.textureGpu(batches[bi].texture);if(!texture)continue;
                bool complete=true;for(const auto& v:fitted[bi])for(const auto& skin:v.sample.skin)complete&=valid[skin.bone];
                if(!complete){for(const auto& v:fitted[bi])for(const auto& skin:v.sample.skin)if(!valid[skin.bone])missingBone=skin.bone;continue;}
                std::vector<Vertex> mesh;mesh.reserve(fitted[bi].size());
                for(const auto& v:fitted[bi]){auto pos=ArmorFit::posed(v.sample,matrices);auto source=v.source;
                    float p[]={pos.x,pos.y,pos.z};for(int i=0;i<3;++i){poseBounds[i]=std::min(poseBounds[i],p[i]);poseBounds[i+3]=std::max(poseBounds[i+3],p[i]);}
                    DWORD color=(source.color&0xff00ff00)|((source.color&0xff)<<16)|((source.color>>16)&0xff);
                    mesh.push_back({pos.x,pos.y,pos.z,nativeLighting.shade(color,pos,source.light,source.flags),source.u,source.v});}
                dev->SetTexture(0,texture);dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);dev->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
                dev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);dev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
                dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,mesh.size()/3,mesh.data(),sizeof(Vertex));drawn+=mesh.size();
            }return;
        }
        for(const auto& batch:batches){auto* texture=scene.textureGpu(batch.texture);if(!texture)continue;
            unsigned part=(batch.flags>>8)&15;gta::Vec origin{},r{},f{},d{};float stretch{};
            if(!pose(part,s,bones,origin,r,f,d,stretch))continue;
            float x0=part==3?-.375f:part==4?.375f:part==5?-.125f:part==6?.125f:0;
            float y0=part==1?1.5f:part==2?.75f:part<5?1.375f:.75f;
            std::vector<Vertex> mesh;mesh.reserve(batch.count);
            for(unsigned n=batch.first;n<batch.first+batch.count;++n){auto v=vertices[n];
                float x=-(v.x-x0)*(part==1?.6f:1.12f),y=(y0-v.y)*stretch,z=v.z*(part==1?.6f:1.f);
                DWORD c=(v.color&0xff00ff00)|((v.color&0xff)<<16)|((v.color>>16)&0xff);
                gta::Vec at{origin.x+x*r.x+y*d.x+z*f.x,origin.y+x*r.y+y*d.y+z*f.y,origin.z+x*r.z+y*d.z+z*f.z};
                mesh.push_back({at.x,at.y,at.z,nativeLighting.shade(c,at,v.light,v.flags),v.u,v.v});}
            dev->SetTexture(0,texture);dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);dev->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
            // Crack rendering uses multiplicative blending even with no scene entities.
            dev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);dev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
            dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,mesh.size()/3,mesh.data(),sizeof(Vertex));drawn+=mesh.size();
        }
    }
};
}
