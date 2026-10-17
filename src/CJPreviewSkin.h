#pragma once
// Private calibration from the user's own CJ reference. No reference assets in source.
class CJPreviewSkin {
    struct Influence{unsigned bone;float weight;float inverse[16];};
    struct Point{gta::Vec rest;std::vector<Influence> skin;};
    std::vector<Point> points;std::map<std::array<int,3>,std::size_t> nearest;
    std::map<unsigned,D3DMATRIX> matrices;bool attempted{};
public:
    void prepare(){
        if(!attempted){attempted=true;wchar_t path[32768];
            if(GetEnvironmentVariableW(L"GROVECRAFT_CJ_SKIN",path,32768)){
                FILE* f=_wfopen(path,L"rb");if(f){char magic[4];unsigned n{};
                    if(std::fread(magic,1,4,f)==4&&!std::memcmp(magic,"GCSK",4)&&std::fread(&n,4,1,f)==1&&n<=20000){
                        bool valid=true;for(unsigned i=0;i<n&&valid;++i){Point p;unsigned count{};
                            valid=std::fread(&p.rest,12,1,f)==1&&std::fread(&count,4,1,f)==1&&count>0&&count<=4;
                            for(unsigned j=0;j<count&&valid;++j){Influence v;valid=std::fread(&v,sizeof v,1,f)==1&&v.bone<512&&std::isfinite(v.weight)&&v.weight>0&&v.weight<=1;
                                if(valid)p.skin.push_back(v);}
                            if(valid)points.push_back(std::move(p));}
                        if(!valid)points.clear();
                    }std::fclose(f);}
            }
        }
        matrices.clear();if(points.empty())return;
        std::set<unsigned> used;for(const auto& p:points)for(const auto& v:p.skin)used.insert(v.bone);
        for(unsigned bone:used){D3DMATRIX m{};if(nativeInteraction.boneMatrix(playerSnapshot,bone,m))matrices.emplace(bone,m);}
    }
    bool matrix(unsigned bone,D3DMATRIX& out)const{auto i=matrices.find(bone);if(i==matrices.end())return false;out=i->second;return true;}
    gta::Vec local(gta::Vec world)const{
        auto v=grove::sub(world,playerSnapshot.position);gta::Vec right{},forward{};
        if(playerSnapshot.matrix&&gta::read(playerSnapshot.matrix,right)&&gta::read(playerSnapshot.matrix+0x10,forward))
            return {v.x*forward.x+v.y*forward.y,v.x*right.x+v.y*right.y,v.z};
        return v;
    }
    gta::Vec pose(gta::Vec rest){
        if(points.empty())return rest;
        std::array<int,3> key{int(std::lround(rest.x*10000)),int(std::lround(rest.y*10000)),int(std::lround(rest.z*10000))};
        auto found=nearest.find(key);
        if(found==nearest.end()){
            float best=1e9f;std::size_t at=0;
            for(std::size_t i=0;i<points.size();++i){auto d=grove::sub(rest,points[i].rest);float q=d.x*d.x+d.y*d.y+d.z*d.z;if(q<best){best=q;at=i;}}
            if(best>.04f)return rest;found=nearest.emplace(key,at).first;
        }
        gta::Vec out{};float total=0;
        for(const auto& v:points[found->second].skin){auto i=matrices.find(v.bone);if(i==matrices.end())return rest;
            const auto* a=v.inverse;gta::Vec p{rest.x*a[0]+rest.y*a[4]+rest.z*a[8]+a[12],rest.x*a[1]+rest.y*a[5]+rest.z*a[9]+a[13],rest.x*a[2]+rest.y*a[6]+rest.z*a[10]+a[14]};
            const auto& m=i->second;out.x+=v.weight*(p.x*m._11+p.y*m._21+p.z*m._31+m._41);
            out.y+=v.weight*(p.x*m._12+p.y*m._22+p.z*m._32+m._42);out.z+=v.weight*(p.x*m._13+p.y*m._23+p.z*m._33+m._43);total+=v.weight;
        }
        return total>.99f?local(out):rest;
    }
};
static CJPreviewSkin cjPreviewSkin;
