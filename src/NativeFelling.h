#pragma once
#include "NativeVegetation.h"
#include "FellingRules.h"
namespace grove {
// Copy only the rendered instance, retaining D3D textures by COM ownership. Stock model/COL
// memory stays untouched. Nearby dug snapshots recreate the fall after reload, without IDs
// tied to reused entity addresses. Trunk collision is an intentionally conservative voxel mesh.
class NativeFelling {
    struct Vertex {float x,y,z;DWORD colour;float u,v;};
    struct Batch {IDirect3DBaseTexture9* texture{};std::vector<Vertex> vertices;bool fixed{};
        Batch()=default;Batch(const Batch&)=delete;Batch& operator=(const Batch&)=delete;
        Batch(Batch&& b)noexcept:texture(b.texture),vertices(std::move(b.vertices)),fixed(b.fixed){b.texture=nullptr;}
        ~Batch(){if(texture)texture->Release();}
    };
    using Key=std::array<int,5>;
    struct Tree {gta::Vec pivot{},origin{};NativeExcavation::Matrix original{};const NativeCol* col{};
        float dx{1},dy{},angle{},height{},radius{.35f},rest{1.5707963f};ULONGLONG stamp{},seen{},updated{};
        bool active{},failed{},spent{};ULONGLONG report{};unsigned draws{},captured{},vertexCount{};std::vector<Batch> batches;
    };
    std::map<Key,Tree> trees;Tree* collecting{};unsigned area{};
    std::set<Key> harvested;
    static std::vector<Vertex> half(const std::vector<Vertex>& poly,float z,bool above){
        std::vector<Vertex> out;if(poly.empty())return out;auto a=poly.back();bool ai=above?a.z>=z:a.z<=z;
        for(auto b:poly){bool bi=above?b.z>=z:b.z<=z;if(ai!=bi){float f=(z-a.z)/(b.z-a.z);Vertex v=a;
            v.x=a.x+(b.x-a.x)*f;v.y=a.y+(b.y-a.y)*f;v.z=z;v.u=a.u+(b.u-a.u)*f;v.v=a.v+(b.v-a.v)*f;
            for(unsigned s=0;s<32;s+=8){unsigned c=unsigned(float((a.colour>>s)&255)+(int((b.colour>>s)&255)-int((a.colour>>s)&255))*f);v.colour=(v.colour&~(255u<<s))|(c<<s);}out.push_back(v);}
            if(bi)out.push_back(b);a=b;ai=bi;
        }return out;
    }
    static Key key(unsigned world,short model,gta::Vec pos){return {int(world),model,int(std::lround(pos.x*16)),int(std::lround(pos.y*16)),int(std::lround(pos.z*16))};}
public:
    bool collectingMesh()const{return collecting!=nullptr;}
    unsigned falls{},captureFailures{},hidden{},vertices{};
    unsigned textured{},uvAttributes{};gta::Vec lastMin{1e9f,1e9f,1e9f},lastMax{-1e9f,-1e9f,-1e9f},drawTranslation{};
    bool remember(unsigned world,short model,gta::Vec pos){
        if(!treeDigMaterial(model)||world>255||!std::isfinite(pos.x+pos.y+pos.z))return false;
        auto k=key(world,model,pos);if(harvested.size()>=8192&&!harvested.count(k))return false;
        bool fresh=harvested.insert(k).second;auto it=trees.find(k);
        if(it!=trees.end()&&!it->second.spent){auto& t=it->second;t.spent=true;
            for(auto& b:t.batches)vertices-=unsigned(b.vertices.size());t.batches.clear();}
        return fresh;
    }
    bool regrow(unsigned world,short model,gta::Vec pos){
        auto k=key(world,model,pos);if(!harvested.erase(k))return false;
        auto it=trees.find(k);if(it!=trees.end()){
            for(auto& b:it->second.batches)vertices-=unsigned(b.vertices.size());trees.erase(it);
        }return true;
    }
    bool begin(void* entity,const NativeExcavation::Matrix& matrix,NativeExcavation& dig,bool allowed){
        collecting=nullptr;if(!allowed||!dig.enabled)return false;
        short model{};NativeCol col{};const NativeCol* identity{};
        if(!dig.model(entity,model,col,identity)||!treeDigMaterial(model))return false;
        auto k=key(dig.world,model,matrix.pos);auto it=trees.find(k);auto now=GetTickCount64();
        if(harvested.count(k)){
            if(it==trees.end()){it=trees.try_emplace(k).first;auto& t=it->second;t.origin=matrix.pos;t.col=identity;t.active=t.spent=true;}
            ++hidden;it->second.seen=now;return true;
        }
        if(it!=trees.end()){
            auto& t=it->second;t.seen=now;
            if(t.active){++hidden;return true;}
            // Streaming/LOD can give an incomplete batch for one frame; retry instead
            // of permanently leaving a severed tree floating.
            if(t.failed){if(now-t.updated<1000)return false;t.failed=false;t.updated=now;}
        }
        gta::Vec root=NativeExcavation::transform(matrix,{0,0,std::max(col.min.z,0.f)});
        float rootRadius=std::clamp(std::min(col.max.x-col.min.x,col.max.y-col.min.y)*.25f,.35f,1.6f);
        if(!severedBase(root,[&](gta::Vec p){return dig.woodCut(p);},rootRadius)){root.z+=1;
            if(!severedBase(root,[&](gta::Vec p){return dig.woodCut(p);},rootRadius))return false;
        }
        if(it==trees.end()){
            if(trees.size()>=24){++captureFailures;return false;}
            it=trees.try_emplace(k).first;auto& t=it->second;t.pivot=root;t.pivot.z=std::floor(root.z+.5f)+1;t.origin=matrix.pos;t.original=matrix;t.col=identity;t.seen=now;
            unsigned hash=unsigned(k[2])*73856093u^unsigned(k[3])*19349663u;
            float heading=float(hash%6283)/1000;t.dx=std::cos(heading);t.dy=std::sin(heading);
            t.radius=std::clamp(std::min(col.max.x-col.min.x,col.max.y-col.min.y)*.25f,.25f,.65f);
        }
        auto& t=it->second;t.batches.clear();t.captured=t.draws=t.vertexCount=0;t.height=0;collecting=&t;return false;
    }
    void noteDraw(){if(collecting)++collecting->draws;}
    void capture(IDirect3DDevice9* dev,const std::vector<BYTE>& data,unsigned stride,const std::vector<D3DVERTEXELEMENT9>& decl){
        if(!collecting)return;auto& t=*collecting;++t.captured;
        int pos=-1,uv=-1,colour=-1;
        for(auto e:decl){if(e.Usage==D3DDECLUSAGE_POSITION&&e.Type==D3DDECLTYPE_FLOAT3)pos=e.Offset;
            if(e.Usage==D3DDECLUSAGE_TEXCOORD&&e.UsageIndex==0&&e.Type==D3DDECLTYPE_FLOAT2)uv=e.Offset;
            if(e.Usage==D3DDECLUSAGE_COLOR&&e.Type==D3DDECLTYPE_D3DCOLOR)colour=e.Offset;}
        if(pos<0||!stride||data.size()%stride||vertices+t.vertexCount+data.size()/stride>300000){t.failed=true;return;}
        if(data.empty())return;Batch batch;dev->GetTexture(0,&batch.texture);
        if(batch.texture)++textured;if(uv>=0)++uvAttributes;D3DMATRIX actual{};dev->GetTransform(D3DTS_WORLD,&actual);drawTranslation={actual._41,actual._42,actual._43};
        for(std::size_t i=0;i<data.size();i+=stride){gta::Vec p{};float tex[2]{};DWORD c=0xffffffff;
            std::memcpy(&p,data.data()+i+pos,12);p=NativeExcavation::transform(t.original,p);
            if(uv>=0)std::memcpy(tex,data.data()+i+uv,8);if(colour>=0)std::memcpy(&c,data.data()+i+colour,4);
            batch.vertices.push_back({p.x,p.y,p.z,c,tex[0],tex[1]});t.height=std::max(t.height,p.z-t.pivot.z);
            // ExcavationRender supplies closed cut faces with this batch's native
            // bark texture. They follow the same split/rotation as the trunk.
            lastMin={std::min(lastMin.x,p.x),std::min(lastMin.y,p.y),std::min(lastMin.z,p.z)};lastMax={std::max(lastMax.x,p.x),std::max(lastMax.y,p.y),std::max(lastMax.z,p.z)};
        }
        Batch upper,stump;upper.texture=batch.texture;stump.texture=batch.texture;stump.fixed=true;
        if(batch.texture){batch.texture->AddRef();batch.texture->AddRef();}
        for(unsigned i=0;i+2<batch.vertices.size();i+=3){std::vector<Vertex> poly{batch.vertices[i],batch.vertices[i+1],batch.vertices[i+2]};
            for(auto* dest:{&upper,&stump}){auto clipped=half(poly,t.pivot.z,!dest->fixed);for(unsigned j=1;j+1<clipped.size();++j)for(auto k:{0u,j,j+1})dest->vertices.push_back(clipped[k]);}
        }
        t.vertexCount+=unsigned(upper.vertices.size()+stump.vertices.size());
        if(vertices+t.vertexCount>300000){t.failed=true;return;}
        if(!upper.vertices.empty())t.batches.push_back(std::move(upper));if(!stump.vertices.empty())t.batches.push_back(std::move(stump));
    }
    void end(){
        if(!collecting)return;auto& t=*collecting;
        if(t.failed||!t.draws||t.draws!=t.captured||t.batches.empty()||t.height<1||t.height>45){
            t.failed=true;t.updated=GetTickCount64();t.batches.clear();++captureFailures;
        }else{
            // Stop when the crown first reaches the local ground plane, rather than burying it.
            for(int step=1;step<=90;++step){float candidate=step*.0174532925f;bool contact=false;
                for(auto& b:t.batches)if(!b.fixed)for(auto v:b.vertices)
                    if(fallenPoint({v.x,v.y,v.z},t.pivot,t.dx,t.dy,candidate).z<t.pivot.z-1.05f){contact=true;break;}
                if(contact){t.rest=(step-1)*.0174532925f;break;}
            }
            // Foliage planes are flexible visual cards, not a rigid support hull.
            t.rest=std::max(t.rest,1.2217305f);
            t.active=true;t.stamp=t.updated=GetTickCount64();++falls;for(auto& b:t.batches)vertices+=unsigned(b.vertices.size());
        }
        collecting=nullptr;
    }
    bool suppresses(const NativeExcavation::Matrix& matrix,const NativeCol& col)const{
        for(auto& [k,t]:trees)if(k[0]==int(area)&&t.active&&t.col==&col
            &&std::abs(matrix.pos.x-t.origin.x)<.02f&&std::abs(matrix.pos.y-t.origin.y)<.02f&&std::abs(matrix.pos.z-t.origin.z)<.02f)return true;
        return false;
    }
    void resetDevice(){trees.clear();harvested.clear();collecting=nullptr;vertices=0;}
    template<class Report> void tick(const gta::Snapshot& player,unsigned world,bool enabled,Report report){
        area=world;auto now=GetTickCount64();
        for(auto it=trees.begin();it!=trees.end();){auto& t=it->second;
            if(!enabled||it->first[0]!=int(world)||std::hypot(t.origin.x-player.position.x,t.origin.y-player.position.y)>144){
                for(auto& b:t.batches)vertices-=unsigned(b.vertices.size());it=trees.erase(it);continue;
            }
            if((player.menu||now-t.updated>1000)&&t.active)t.stamp+=now-t.updated;t.updated=now;
            if(t.active&&!t.spent&&!player.menu){float f=std::clamp(float(now-t.stamp)/2200.f,0.f,1.f);t.angle=f*f*(3-2*f)*t.rest;
                if(f>=1&&now-t.report>500&&std::hypot(t.origin.x-player.position.x,t.origin.y-player.position.y)<30){
                    report(it->first[1],t.origin,std::clamp(int(std::ceil(t.height*.5f)),2,16),t.dx,t.dy,t.height*.85f*std::sin(t.angle));t.report=now;
                }
            }
            ++it;
        }
    }
    void collision(std::map<SectionKey,SectionBits>& sections)const{
        for(auto& [k,t]:trees)if(k[0]==int(area)&&t.active&&!t.spent){
            for(float h=1;h<t.height*.85f;h+=.4f){auto p=fallenPoint({t.pivot.x,t.pivot.y,t.pivot.z+h},t.pivot,t.dx,t.dy,t.angle);
                // Match the excavation voxel grid. No solid canopy; trunk only.
                for(float dx:{-t.radius,t.radius})for(float dy:{-t.radius,t.radius}){
                    int x=int(std::floor(p.x+dx)),y=int(std::floor(p.z)),z=int(std::floor(-p.y+dy));
                    auto& bits=sections[{x>>4,y>>4,z>>4}];unsigned bit=(x&15)+16*(z&15)+256*(y&15);bits[bit/8]|=1u<<(bit%8);
                }
            }
        }
    }
    void draw(IDirect3DDevice9* dev){
        IDirect3DStateBlock9* state{};if(FAILED(dev->CreateStateBlock(D3DSBT_ALL,&state)))return;
        D3DMATRIX world{};world._11=world._22=world._33=world._44=1;dev->SetTransform(D3DTS_WORLD,&world);
        dev->SetVertexShader(nullptr);dev->SetPixelShader(nullptr);dev->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1);
        dev->SetRenderState(D3DRS_ZENABLE,TRUE);dev->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
        dev->SetRenderState(D3DRS_LIGHTING,FALSE);dev->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
        nativeWeather.apply(dev);dev->SetRenderState(D3DRS_STENCILENABLE,FALSE);dev->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);dev->SetRenderState(D3DRS_COLORWRITEENABLE,15);
        dev->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);dev->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATER);dev->SetRenderState(D3DRS_ALPHAREF,100);
        dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);dev->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
        dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
        dev->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);dev->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
        dev->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP);dev->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP);
        dev->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
        for(auto& [k,t]:trees)if(k[0]==int(area)&&t.active&&!t.spent)for(auto& b:t.batches){
            std::vector<Vertex> moved=b.vertices;if(!b.fixed)for(auto& v:moved){auto p=fallenPoint({v.x,v.y,v.z},t.pivot,t.dx,t.dy,t.angle);v.x=p.x;v.y=p.y;v.z=p.z;}
            dev->SetTexture(0,b.texture);dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,UINT(moved.size()/3),moved.data(),sizeof(Vertex));
        }
        state->Apply();state->Release();
    }
};
}
