#pragma once
// Cut native triangles against unit excavation boxes, interpolating the actual
// vertex declaration (positions, UVs, normals, colours). Original buffers stay
// untouched and DrawPrimitiveUP's stream reset is restored before returning.
namespace grove {
struct CutVertex {gta::Vec world;std::array<BYTE,128> data{};};
inline CutVertex cutLerp(const CutVertex& a,const CutVertex& b,float t,const std::vector<D3DVERTEXELEMENT9>& decl){
    CutVertex r=a;r.world={a.world.x+(b.world.x-a.world.x)*t,a.world.y+(b.world.y-a.world.y)*t,a.world.z+(b.world.z-a.world.z)*t};
    for(const auto& e:decl){unsigned n=e.Type<=D3DDECLTYPE_FLOAT4?e.Type+1:0;
        if(n)for(unsigned i=0;i<n;++i){float av{},bv{};std::memcpy(&av,a.data.data()+e.Offset+i*4,4);std::memcpy(&bv,b.data.data()+e.Offset+i*4,4);float v=av+(bv-av)*t;std::memcpy(r.data.data()+e.Offset+i*4,&v,4);}
        else if(e.Type==D3DDECLTYPE_D3DCOLOR||e.Type==D3DDECLTYPE_UBYTE4)for(int i=0;i<4;++i)r.data[e.Offset+i]=BYTE(std::clamp(float(a.data[e.Offset+i])+(float(b.data[e.Offset+i])-a.data[e.Offset+i])*t,0.f,255.f));
    }return r;
}
inline float cutCoordinate(const gta::Vec& v,int axis){return axis==0?v.x:axis==1?v.y:v.z;}
inline std::vector<CutVertex> cutHalf(const std::vector<CutVertex>& polygon,int axis,float boundary,bool greater,const std::vector<D3DVERTEXELEMENT9>& decl){
    std::vector<CutVertex> out;if(polygon.empty())return out;
    auto prev=polygon.back();float pd=cutCoordinate(prev.world,axis)-boundary;bool pin=greater?pd>=0:pd<=0;
    for(const auto& cur:polygon){float cd=cutCoordinate(cur.world,axis)-boundary;bool cin=greater?cd>=0:cd<=0;
        if(pin!=cin)out.push_back(cutLerp(prev,cur,pd/(pd-cd),decl));if(cin)out.push_back(cur);prev=cur;pd=cd;pin=cin;
    }return out;
}
inline std::vector<std::vector<CutVertex>> cutBox(const std::vector<CutVertex>& polygon,const NativeBox& box,const std::vector<D3DVERTEXELEMENT9>& decl,float padding=.005f){
    std::vector<std::vector<CutVertex>> out;auto inside=polygon;
    for(int axis=0;axis<3&&!inside.empty();++axis){float lo=cutCoordinate(box.min,axis)-padding,hi=cutCoordinate(box.max,axis)+padding;
        auto low=cutHalf(inside,axis,lo,false,decl);if(low.size()>=3)out.push_back(std::move(low));inside=cutHalf(inside,axis,lo,true,decl);
        auto high=cutHalf(inside,axis,hi,true,decl);if(high.size()>=3)out.push_back(std::move(high));inside=cutHalf(inside,axis,hi,false,decl);
    }return out;
}
// Cross-sections of the original bark, not Minecraft cubes or a new texture.
// Convex trunk rings close the exposed part of a cut; foliage above the low
// trunk band is excluded. UVs/colours remain from this exact native draw batch.
inline std::vector<CutVertex> woodCap(const std::vector<CutVertex>& source,int axis,float plane,
        const NativeExcavation& excavation,const std::vector<D3DVERTEXELEMENT9>& decl){
    std::vector<CutVertex> points;auto root=excavation.woodRoot;float r=excavation.woodRadius;
    for(unsigned i=0;i+2<source.size();i+=3){
        std::vector<CutVertex> poly{source[i],source[i+1],source[i+2]};
        poly=cutHalf(poly,2,root.z-1,true,decl);poly=cutHalf(poly,2,root.z+4,false,decl);
        poly=cutHalf(poly,0,root.x-r,true,decl);poly=cutHalf(poly,0,root.x+r,false,decl);
        poly=cutHalf(poly,1,root.y-r,true,decl);poly=cutHalf(poly,1,root.y+r,false,decl);
        if(poly.empty())continue;
        auto a=poly.back();float da=cutCoordinate(a.world,axis)-plane;
        for(auto b:poly){float db=cutCoordinate(b.world,axis)-plane;
            if(std::abs(da)<.0001f)points.push_back(a);
            if((da<0&&db>0)||(da>0&&db<0))points.push_back(cutLerp(a,b,da/(da-db),decl));
            a=b;da=db;
        }
        if(points.size()>2048)return {};
    }
    int u=(axis+1)%3,v=(axis+2)%3;
    auto coord=[&](const CutVertex& p,int a){return cutCoordinate(p.world,a);};
    std::sort(points.begin(),points.end(),[&](const auto& a,const auto& b){
        return coord(a,u)<coord(b,u)||(coord(a,u)==coord(b,u)&&coord(a,v)<coord(b,v));});
    points.erase(std::unique(points.begin(),points.end(),[&](const auto& a,const auto& b){
        return std::abs(coord(a,u)-coord(b,u))<.0001f&&std::abs(coord(a,v)-coord(b,v))<.0001f;}),points.end());
    if(points.size()<3)return {};
    auto cross=[&](const auto& a,const auto& b,const auto& c){
        return (coord(b,u)-coord(a,u))*(coord(c,v)-coord(a,v))-(coord(b,v)-coord(a,v))*(coord(c,u)-coord(a,u));};
    std::vector<CutVertex> hull;
    for(const auto& p:points){while(hull.size()>1&&cross(hull[hull.size()-2],hull.back(),p)<=.000001f)hull.pop_back();hull.push_back(p);}
    unsigned lower=unsigned(hull.size());
    for(int i=int(points.size())-2;i>=0;--i){while(hull.size()>lower&&cross(hull[hull.size()-2],hull.back(),points[i])<=.000001f)hull.pop_back();hull.push_back(points[i]);}
    if(!hull.empty())hull.pop_back();return hull.size()>=3?hull:std::vector<CutVertex>{};
}
inline void appendWoodCaps(std::vector<BYTE>& output,const std::vector<CutVertex>& source,
        const std::vector<NativeBox>& boxes,const NativeExcavation& excavation,
        const std::vector<D3DVERTEXELEMENT9>& decl,unsigned stride){
    if(!excavation.woodContext||boxes.size()>32)return;
    for(unsigned boxIndex=0;boxIndex<boxes.size();++boxIndex){const auto& box=boxes[boxIndex];
        if(box.max.z<excavation.woodRoot.z-1||box.min.z>excavation.woodRoot.z+4)continue;
        for(int axis=0;axis<3;++axis)for(int high=0;high<2;++high){
            float plane=cutCoordinate(high?box.max:box.min,axis)+(high?.005f:-.005f);
            auto poly=woodCap(source,axis,plane,excavation,decl);
            for(int other=0;other<3&&!poly.empty();++other)if(other!=axis){
                poly=cutHalf(poly,other,cutCoordinate(box.min,other)-.005f,true,decl);
                poly=cutHalf(poly,other,cutCoordinate(box.max,other)+.005f,false,decl);
            }
            if(poly.size()<3)continue;
            // Match the cavity wall normal in the original object's coordinates.
            for(auto& vertex:poly)for(const auto& e:decl)if(e.Usage==D3DDECLUSAGE_NORMAL&&e.Type==D3DDECLTYPE_FLOAT3){
                float sign=high?-1.f:1.f;auto& m=excavation.renderMatrix;
                float normal[]={sign*cutCoordinate(m.right,axis),sign*cutCoordinate(m.forward,axis),sign*cutCoordinate(m.up,axis)};
                std::memcpy(vertex.data.data()+e.Offset,normal,12);
            }
            std::vector<std::vector<CutVertex>> pieces{poly};
            for(unsigned j=0;j<boxes.size();++j)if(j!=boxIndex){std::vector<std::vector<CutVertex>> next;
                // Exclude coincident outer faces: subtracting their boundary as
                // well as the interior would open a seam between adjacent cuts.
                for(const auto& piece:pieces){auto parts=cutBox(piece,boxes[j],decl,.0048f);next.insert(next.end(),parts.begin(),parts.end());}
                pieces=std::move(next);if(pieces.empty())break;
            }
            for(const auto& piece:pieces)for(unsigned j=1;j+1<piece.size();++j){
                unsigned indices[]={0,high?j+1:j,high?j:j+1};
                for(auto index:indices)output.insert(output.end(),piece[index].data.begin(),piece[index].data.begin()+stride);
            }
        }
    }
}
inline bool cutNativeDraw(NativeExcavation& excavation,IDirect3DDevice9* dev,D3DPRIMITIVETYPE type,INT base,UINT minimum,UINT count,UINT start,UINT primitives,HRESULT& result){
    ++excavation.indexedCalls;if(excavation.rendering){++excavation.worldDraws;excavation.lastType=type;}
    if(!excavation.enabled||!excavation.rendering||(!excavation.cells&&!excavation.capture)||(type!=D3DPT_TRIANGLELIST&&type!=D3DPT_TRIANGLESTRIP)||primitives>30000||!count||count>65536)return false;
    struct Timing {double& total;LARGE_INTEGER begin{},frequency{};Timing(double& t):total(t){QueryPerformanceCounter(&begin);QueryPerformanceFrequency(&frequency);}~Timing(){LARGE_INTEGER end{};QueryPerformanceCounter(&end);total+=(end.QuadPart-begin.QuadPart)*1000000.0/frequency.QuadPart;}} timing(excavation.cutMicros);
    unsigned indexCount=type==D3DPT_TRIANGLESTRIP?primitives+2:primitives*3;
    excavation.stage=1;
    struct Stream {IDirect3DVertexBuffer9* vb{};UINT offset{},stride{};void* data{};};
    std::array<Stream,16> streams{};IDirect3DIndexBuffer9* ib{};IDirect3DVertexDeclaration9 *vd{},*packed{};UINT stride{};
    std::map<IDirect3DVertexBuffer9*,void*> locked;
    auto unlock=[&](){for(auto& item:locked)item.first->Unlock();locked.clear();};
    auto release=[&](){unlock();if(packed)packed->Release();if(vd)vd->Release();if(ib)ib->Release();for(auto& stream:streams)if(stream.vb)stream.vb->Release();};
    excavation.stage=2;
    HRESULT indicesResult=dev->GetIndices(&ib),declarationResult=dev->GetVertexDeclaration(&vd);
    excavation.lastDrawMin={0,0,float(indicesResult)};
    excavation.lastDrawMax={ib?1.f:0.f,vd?1.f:0.f,float(declarationResult)};
    if(FAILED(indicesResult)||!ib||FAILED(declarationResult)||!vd){release();return false;}
    D3DVERTEXELEMENT9 elements[MAXD3DDECLLENGTH+1]{};UINT elementCount=MAXD3DDECLLENGTH+1;
    if(FAILED(vd->GetDeclaration(elements,&elementCount))){release();return false;}
    std::vector<D3DVERTEXELEMENT9> decl,original;std::vector<unsigned> sizes;int position=-1;
    excavation.stage=3;
    for(unsigned i=0;i<elementCount&&elements[i].Stream!=0xff;++i){auto e=elements[i];
        unsigned size=e.Type<=D3DDECLTYPE_FLOAT4?(e.Type+1)*4:(e.Type==D3DDECLTYPE_D3DCOLOR||e.Type==D3DDECLTYPE_UBYTE4?4:0);
        if(e.Stream>=16||!size||stride+size>128){release();++excavation.rejectedDraws;return false;}
        auto& stream=streams[e.Stream];if(!stream.vb&&FAILED(dev->GetStreamSource(e.Stream,&stream.vb,&stream.offset,&stream.stride))){release();return false;}
        if(!stream.vb||!stream.stride||e.Offset+size>stream.stride){release();return false;}
        original.push_back(e);sizes.push_back(size);e.Stream=0;e.Offset=WORD(stride);
        if(e.Usage==D3DDECLUSAGE_POSITION&&e.UsageIndex==0&&e.Type==D3DDECLTYPE_FLOAT3)position=e.Offset;decl.push_back(e);stride+=size;
    }
    if(position<0){release();return false;}
    excavation.stage=4;
    D3DINDEXBUFFER_DESC idesc{};ib->GetDesc(&idesc);
    unsigned indexSize=idesc.Format==D3DFMT_INDEX16?2:idesc.Format==D3DFMT_INDEX32?4:0;
    if(!indexSize||base+int(minimum)<0||std::uint64_t(start+indexCount)*indexSize>idesc.Size){release();return false;}
    for(auto& stream:streams)if(stream.vb){D3DVERTEXBUFFER_DESC desc{};stream.vb->GetDesc(&desc);
        if(std::uint64_t(stream.offset)+std::uint64_t(base+minimum+count)*stream.stride>desc.Size){release();return false;}}
    void* indices{};
    excavation.stage=5;
    for(auto& stream:streams)if(stream.vb){auto it=locked.find(stream.vb);
        if(it==locked.end()){void* data{};if(FAILED(stream.vb->Lock(0,0,&data,0))){release();return false;}it=locked.emplace(stream.vb,data).first;}stream.data=it->second;}
    if(FAILED(ib->Lock(start*indexSize,indexCount*indexSize,&indices,0))){release();return false;}
    std::vector<CutVertex> source;source.reserve(primitives*3);gta::Vec lo{1e9f,1e9f,1e9f},hi{-1e9f,-1e9f,-1e9f};bool valid=true;
    excavation.stage=6;
    for(unsigned i=0;i<primitives*3;++i){unsigned at=i;
        if(type==D3DPT_TRIANGLESTRIP){unsigned triangle=i/3,corner=i%3;if(triangle&1&&corner<2)corner=1-corner;at=triangle+corner;}
        unsigned index=indexSize==2?static_cast<WORD*>(indices)[at]:static_cast<DWORD*>(indices)[at];
        if(index<minimum||index>=minimum+count){valid=false;break;}CutVertex v;
        for(unsigned attr=0;attr<decl.size();++attr){auto e=original[attr];auto& stream=streams[e.Stream];
            std::memcpy(v.data.data()+decl[attr].Offset,static_cast<BYTE*>(stream.data)+stream.offset+(base+index)*stream.stride+e.Offset,sizes[attr]);}
        gta::Vec local{};std::memcpy(&local,v.data.data()+position,12);v.world=NativeExcavation::transform(excavation.renderMatrix,local);
        if(!std::isfinite(v.world.x+v.world.y+v.world.z)){valid=false;break;}
        lo={std::min(lo.x,v.world.x),std::min(lo.y,v.world.y),std::min(lo.z,v.world.z)};hi={std::max(hi.x,v.world.x),std::max(hi.y,v.world.y),std::max(hi.z,v.world.z)};source.push_back(v);
    }
    ib->Unlock();unlock();if(!valid){release();return false;}auto boxes=excavation.boxes({lo.x-.005f,lo.y-.005f,lo.z-.005f},{hi.x+.005f,hi.y+.005f,hi.z+.005f});if(boxes.empty()&&!excavation.capture){++excavation.noBoxDraws;excavation.lastDrawMin=lo;excavation.lastDrawMax=hi;release();return false;}
    std::vector<BYTE> output;bool changed=false;
    static const bool cacheEnabled=[](){char value[8]{};return !(GetEnvironmentVariableA("GROVECRAFT_CUT_CACHE",value,sizeof value)&&value[0]=='0');}();
    std::vector<BYTE> cacheKey;
    bool useCache=cacheEnabled&&!excavation.capture;
    if(useCache){
        auto append=[&](const void* p,std::size_t n){auto b=static_cast<const BYTE*>(p);cacheKey.insert(cacheKey.end(),b,b+n);};
        append(&stride,sizeof stride);append(&excavation.woodContext,sizeof excavation.woodContext);
        if(excavation.woodContext){append(&excavation.woodRoot,sizeof excavation.woodRoot);append(&excavation.woodRadius,sizeof excavation.woodRadius);}
        unsigned declCount=unsigned(decl.size()),boxCount=unsigned(boxes.size());append(&declCount,sizeof declCount);append(decl.data(),decl.size()*sizeof(decl[0]));
        append(&boxCount,sizeof boxCount);for(const auto& b:boxes){append(&b.min,sizeof b.min);append(&b.max,sizeof b.max);}
        for(const auto& v:source){append(&v.world,sizeof v.world);append(v.data.data(),stride);}
    }
    const auto* cached=useCache?excavation.cutCache.find(cacheKey):nullptr;
    if(cached){++excavation.cutCacheHits;changed=cached->changed;output=cached->mesh;}
    else {
    if(useCache)++excavation.cutCacheMisses;
    excavation.stage=7;
    for(unsigned i=0;i<source.size();i+=3){std::vector<std::vector<CutVertex>> pieces{{source[i],source[i+1],source[i+2]}};
        for(auto box:boxes){std::vector<std::vector<CutVertex>> next;
            for(const auto& poly:pieces){gta::Vec pmin=poly[0].world,pmax=pmin;for(const auto& v:poly){pmin={std::min(pmin.x,v.world.x),std::min(pmin.y,v.world.y),std::min(pmin.z,v.world.z)};pmax={std::max(pmax.x,v.world.x),std::max(pmax.y,v.world.y),std::max(pmax.z,v.world.z)};}
                if(pmax.x<box.min.x-.005f||pmin.x>box.max.x+.005f||pmax.y<box.min.y-.005f||pmin.y>box.max.y+.005f||pmax.z<box.min.z-.005f||pmin.z>box.max.z+.005f){next.push_back(poly);continue;}
                auto parts=cutBox(poly,box,decl);next.insert(next.end(),parts.begin(),parts.end());changed=true;
            }pieces=std::move(next);if(pieces.size()>4096){release();++excavation.rejectedDraws;return false;}
        }
        for(const auto& poly:pieces)for(unsigned j=1;j+1<poly.size();++j)for(auto index:{0u,j,j+1})output.insert(output.end(),poly[index].data.begin(),poly[index].data.begin()+stride);
        if(output.size()>32*1024*1024){release();++excavation.rejectedDraws;return false;}
    }
    if(changed)appendWoodCaps(output,source,boxes,excavation,decl,stride);
    if(output.size()>32*1024*1024){release();++excavation.rejectedDraws;return false;}
    if(useCache)excavation.cutCache.put(std::move(cacheKey),output,changed);
    }
    if(excavation.capture)excavation.capture(dev,output,stride,decl);
    if(!changed){release();return false;}
    auto packedDecl=decl;packedDecl.push_back(D3DDECL_END());
    if(FAILED(dev->CreateVertexDeclaration(packedDecl.data(),&packed))){release();return false;}
    DWORD previousFvf{};dev->GetFVF(&previousFvf);
    dev->SetVertexDeclaration(packed);
    DWORD cull{};dev->GetRenderState(D3DRS_CULLMODE,&cull);
    if(excavation.woodContext)dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    result=output.empty()?D3D_OK:dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,UINT(output.size()/stride/3),output.data(),stride);
    if(excavation.woodContext)dev->SetRenderState(D3DRS_CULLMODE,cull);
    excavation.stage=8;
    dev->SetVertexDeclaration(vd);if(previousFvf)dev->SetFVF(previousFvf);
    for(unsigned i=0;i<streams.size();++i)if(streams[i].vb)dev->SetStreamSource(i,streams[i].vb,streams[i].offset,streams[i].stride);
    dev->SetIndices(ib);++excavation.clippedDraws;release();return true;
}
}
