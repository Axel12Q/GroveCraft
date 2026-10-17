#pragma once
// Private Blender-authored CJ calibration. No game model or texture is embedded.
namespace grove {
struct ArmorInfluence {unsigned bone{};float weight{};gta::Vec local{};};
struct ArmorPoint {gta::Vec rest{};std::array<ArmorInfluence,4> skin{};};
struct ArmorSample {gta::Vec rest{};std::vector<ArmorInfluence> skin;};
class ArmorFit {
    struct Part {gta::Vec lo{},hi{};std::vector<ArmorPoint> points;};
    std::array<Part,6> parts;
    std::array<unsigned,3> dims{};
public:
    bool ready{};
    static bool allowed(unsigned k){return k==0||k==1||k==2||k==3||k==5||k==21||k==22||k==23||k==24||k==31||k==32||k==33||k==34||k==41||k==42||k==43||k==51||k==52||k==53;}
    bool parse(const unsigned char* data,std::size_t bytes){
        if(bytes<20||std::memcmp(data,"GCAR",4))return false;
        unsigned version{},d[3]{};std::memcpy(&version,data+4,4);std::memcpy(d,data+8,12);
        if(version!=1||d[0]!=5||d[1]!=17||d[2]!=5||bytes!=20+6*(24+425*92))return false;
        std::array<Part,6> candidate;std::size_t at=20;
        for(auto& part:candidate){
            std::memcpy(&part.lo,data+at,12);std::memcpy(&part.hi,data+at+12,12);at+=24;
            auto* lo=&part.lo.x;auto* hi=&part.hi.x;
            for(int i=0;i<3;++i)if(!std::isfinite(lo[i]+hi[i])||lo[i]>=hi[i]||std::abs(lo[i])+std::abs(hi[i])>8)return false;
            part.points.resize(425);
            for(auto& point:part.points){
                std::memcpy(&point.rest,data+at,12);at+=12;
                if(!std::isfinite(point.rest.x+point.rest.y+point.rest.z)||length(point.rest)>3)return false;
                float sum=0;
                for(auto& influence:point.skin){
                    std::memcpy(&influence.bone,data+at,4);std::memcpy(&influence.weight,data+at+4,4);
                    std::memcpy(&influence.local,data+at+8,12);at+=20;
                    if(!allowed(influence.bone)||!std::isfinite(influence.weight)||influence.weight<0||influence.weight>1
                        ||!std::isfinite(influence.local.x+influence.local.y+influence.local.z)||length(influence.local)>3)return false;
                    sum+=influence.weight;
                }if(std::abs(sum-1)>.001f)return false;
            }
        }
        parts.swap(candidate);dims={d[0],d[1],d[2]};ready=true;return true;
    }
    bool load(const wchar_t* path){
        FILE* file=_wfopen(path,L"rb");if(!file)return false;
        std::fseek(file,0,SEEK_END);long size=std::ftell(file);std::rewind(file);
        if(size<20||size>1024*1024){std::fclose(file);return false;}
        std::vector<unsigned char> data(size);auto read=std::fread(data.data(),1,data.size(),file);std::fclose(file);
        return read==data.size()&&parse(data.data(),data.size());
    }
    ArmorSample sample(gta::Vec v,unsigned part)const{
        ArmorSample out;if(!ready||part<1||part>6)return out;
        const auto& p=parts[part-1];float a[3]={v.x,v.y,v.z},lo[3]={p.lo.x,p.lo.y,p.lo.z},hi[3]={p.hi.x,p.hi.y,p.hi.z};
        unsigned index[3];float fraction[3];
        for(int i=0;i<3;++i){float t=std::clamp((a[i]-lo[i])/(hi[i]-lo[i]),0.f,1.f)*(dims[i]-1);
            index[i]=std::min(unsigned(t),dims[i]-2);fraction[i]=t-index[i];}
        std::array<gta::Vec,55> weighted{};std::array<float,55> weights{};
        for(unsigned corner=0;corner<8;++corner){unsigned xyz[3];float w=1;
            for(int i=0;i<3;++i){unsigned bit=(corner>>i)&1;xyz[i]=index[i]+bit;w*=bit?fraction[i]:1-fraction[i];}
            const auto& n=p.points[(xyz[2]*dims[1]+xyz[1])*dims[0]+xyz[0]];
            out.rest.x+=w*n.rest.x;out.rest.y+=w*n.rest.y;out.rest.z+=w*n.rest.z;
            for(const auto& s:n.skin){float b=w*s.weight;if(!b)continue;weights[s.bone]+=b;
                weighted[s.bone].x+=b*s.local.x;weighted[s.bone].y+=b*s.local.y;weighted[s.bone].z+=b*s.local.z;}
        }
        for(unsigned i=0;i<55;++i)if(weights[i]>.0000001f)out.skin.push_back({i,weights[i],weighted[i]});
        return out;
    }
    static gta::Vec posed(const ArmorSample& sample,const std::array<D3DMATRIX,55>& matrices){
        gta::Vec out{};for(const auto& s:sample.skin){const auto& m=matrices[s.bone];auto v=s.local;
            out.x+=v.x*m._11+v.y*m._21+v.z*m._31+s.weight*m._41;
            out.y+=v.x*m._12+v.y*m._22+v.z*m._32+s.weight*m._42;
            out.z+=v.x*m._13+v.y*m._23+v.z*m._33+s.weight*m._43;
        }return out;
    }
};
}
