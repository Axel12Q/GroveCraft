#pragma once
// Animated vanilla entity meshes, including mobs, flames and block entities.
// Keep decoding transactional: a malformed frame cannot partially replace a valid one.
namespace grove {
class Scene {
    struct Texture { unsigned w{},h{}; std::vector<unsigned char> pixels; IDirect3DTexture9* gpu{}; };
    std::map<unsigned,Texture> textures;
    std::vector<Vertex> vertices;
    std::vector<p::RenBatch> batches;
    std::size_t textureBytes{};
    ULONGLONG stamp{};
public:
    unsigned rejected{};
    std::size_t vertexCount() const { return vertices.size(); }
    IDirect3DTexture9* textureGpu(unsigned id)const{auto it=textures.find(id);return it==textures.end()?nullptr:it->second.gpu;}
    void resetDevice(){for(auto& [id,t]:textures)if(t.gpu){t.gpu->Release();t.gpu=nullptr;}}
    void clear(){resetDevice();textures.clear();vertices.clear();batches.clear();textureBytes=0;stamp=0;}
    bool texture(const unsigned char* data,std::size_t bytes){
        if(bytes<sizeof(p::RenTexture))return false;
        p::RenTexture h;std::memcpy(&h,data,sizeof h);
        auto found=textures.find(h.id);auto old=found==textures.end()?0:found->second.pixels.size();
        auto size=4ull*h.width*h.height;
        if(!h.id||!h.width||!h.height||h.width>2048||h.height>2048||sizeof(h)+size!=bytes
            ||(found==textures.end()&&textures.size()>=256)||textureBytes-old+size>64ull*1024*1024)return false;
        auto& t=textures[h.id];if(t.gpu){t.gpu->Release();t.gpu=nullptr;}
        t.w=h.width;t.h=h.height;t.pixels.assign(data+sizeof h,data+bytes);textureBytes=textureBytes-old+size;return true;
    }
    bool frame(const unsigned char* data,std::size_t bytes){
        if(bytes<sizeof(p::RenScene))return false;
        p::RenScene h;std::memcpy(&h,data,sizeof h);
        if(!std::isfinite(h.originX)||!std::isfinite(h.originY)||!std::isfinite(h.originZ)
            ||std::abs(h.originX)>16000||std::abs(h.originY)>16000||std::abs(h.originZ)>16000
            ||h.batchCount>4096||h.vertexCount>300000||h.vertexCount%3
            ||sizeof(h)+std::uint64_t(h.batchCount)*sizeof(p::RenBatch)+std::uint64_t(h.vertexCount)*sizeof(p::RenVertex)!=bytes)return false;
        std::vector<p::RenBatch> next(h.batchCount);
        std::memcpy(next.data(),data+sizeof h,next.size()*sizeof(p::RenBatch));
        for(const auto& b:next)if(b.first>h.vertexCount||b.count>h.vertexCount-b.first||b.count%3||(b.flags&~1u))return false;
        std::vector<Vertex> mesh(h.vertexCount);
        const auto* start=data+sizeof h+next.size()*sizeof(p::RenBatch);
        for(unsigned i=0;i<h.vertexCount;++i){
            p::RenVertex v;std::memcpy(&v,start+i*sizeof v,sizeof v);
            if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||!std::isfinite(v.u)||!std::isfinite(v.v)
                ||std::abs(v.x)>256||std::abs(v.y)>256||std::abs(v.z)>256)return false;
            DWORD c=(v.color&0xff00ff00)|((v.color&0xff)<<16)|((v.color>>16)&0xff);
            mesh[i]={float(v.x+h.originX),float(-v.z-h.originZ),float(v.y+h.originY),c,v.u,v.v,v.light,v.flags};
        }
        vertices.swap(mesh);batches.swap(next);stamp=GetTickCount64();return true;
    }
    void ensureTextures(IDirect3DDevice9* dev){
        for(auto& [id,t]:textures)if(!t.gpu){
            if(FAILED(dev->CreateTexture(t.w,t.h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&t.gpu,nullptr)))continue;
            D3DLOCKED_RECT r{};if(FAILED(t.gpu->LockRect(0,&r,nullptr,0))){t.gpu->Release();t.gpu=nullptr;continue;}
            for(unsigned y=0;y<t.h;++y)for(unsigned x=0;x<t.w;++x){
                auto* dst=static_cast<unsigned char*>(r.pBits)+y*r.Pitch+x*4;auto* src=t.pixels.data()+(std::size_t(y)*t.w+x)*4;
                dst[0]=src[2];dst[1]=src[1];dst[2]=src[0];dst[3]=src[3];
            }
            t.gpu->UnlockRect(0);
        }
    }
    void draw(IDirect3DDevice9* dev,IDirect3DTexture9* blockAtlas){
        ensureTextures(dev);
        if(!stamp||GetTickCount64()-stamp>2000)return;
        auto lit=vertices;nativeLighting.shadeMesh(lit);
        for(unsigned pass=0;pass<2;++pass){
            dev->SetRenderState(D3DRS_ALPHABLENDENABLE,pass?TRUE:FALSE);
            dev->SetRenderState(D3DRS_ZWRITEENABLE,pass?FALSE:TRUE);
            dev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);dev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
            dev->SetRenderState(D3DRS_ALPHAREF,pass?1:128);
            for(const auto& b:batches)if((b.flags&1)==pass&&b.count){
                auto* tex=blockAtlas;
                if(b.texture){auto it=textures.find(b.texture);if(it==textures.end()||!it->second.gpu)continue;tex=it->second.gpu;}
                if(!tex)continue;dev->SetTexture(0,tex);
                dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,b.count/3,lit.data()+b.first,sizeof(Vertex));
            }
        }
        dev->SetTexture(0,blockAtlas);dev->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
        dev->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);dev->SetRenderState(D3DRS_ALPHAREF,128);
    }
};
}
