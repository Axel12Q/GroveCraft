#define GROVE_RENDERER_UNIT
#include "bridge.cpp"
#include <cassert>
int main(){
    grove::ArmorFit fit;
    assert(fit.load(L"artifacts/armorfit-private/GroveCraft-armor-fit.bin"));
    std::array<D3DMATRIX,55> identity{};
    for(auto& m:identity){m._11=m._22=m._33=m._44=1;m._41=10;m._42=20;m._43=30;}
    for(unsigned part=1;part<=6;++part)for(float y=-.05f;y<2;y+=.037f){
        auto sample=fit.sample({.05f,y,.07f},part);float weight=0;
        for(auto s:sample.skin)weight+=s.weight;
        assert(std::abs(weight-1)<.001f&&std::isfinite(sample.rest.x+sample.rest.y+sample.rest.z));
        auto translated=grove::ArmorFit::posed(sample,identity);
        for(auto& m:identity){m._41-=10;m._42-=20;m._43-=30;}
        auto base=grove::ArmorFit::posed(sample,identity);
        assert(std::abs(translated.x-base.x-10)<.001f&&std::abs(translated.y-base.y-20)<.001f&&std::abs(translated.z-base.z-30)<.001f);
        for(auto& m:identity){m._41+=10;m._42+=20;m._43+=30;}
    }
    auto arm=fit.sample({-.4f,.85f,0},3);bool elbow=false;
    for(auto s:arm.skin)elbow|=s.bone==23;assert(elbow);
    auto joint=fit.sample({-.375f,1.06f,.05f},3);float lowerWeight=0;
    for(auto s:joint.skin)if(s.bone==23)lowerWeight=s.weight;
    assert(lowerWeight>.1f&&lowerWeight<.9f);
    auto before=grove::ArmorFit::posed(joint,identity);identity[23]._41+=1;
    auto after=grove::ArmorFit::posed(joint,identity);
    assert(std::abs(after.x-before.x-lowerWeight)<.001f); // Blend follows elbow, not one rigid whole sleeve.
    for(auto side:std::array<std::pair<unsigned,unsigned>,2>{{{3,21},{4,31}}}){
        auto shoulder=fit.sample({side.first==3?-.375f:.375f,1.43f,.05f},side.first);
        float clavicle=0;for(auto s:shoulder.skin)if(s.bone==side.second)clavicle=s.weight;
        assert(clavicle>.1f&&clavicle<.8f); // Shoulder seam must follow native clavicle too.
        auto before=grove::ArmorFit::posed(shoulder,identity);identity[side.second]._43+=.2f;
        auto after=grove::ArmorFit::posed(shoulder,identity);identity[side.second]._43-=.2f;
        assert(std::abs(after.z-before.z-clavicle*.2f)<.001f);
    }
    for(unsigned part:{5u,6u}){
        auto hip=fit.sample({part==5?-.12f:.12f,1.38f,.06f},part);float pelvis=0;
        for(auto s:hip.skin)if(s.bone==1)pelvis=s.weight;
        assert(pelvis>.2f&&pelvis<.9f);
        auto before=grove::ArmorFit::posed(hip,identity);identity[1]._42+=.2f;
        auto after=grove::ArmorFit::posed(hip,identity);identity[1]._42-=.2f;
        assert(std::abs(after.y-before.y-pelvis*.2f)<.001f);
    }
    assert(!fit.parse(reinterpret_cast<const unsigned char*>("GCAR"),4)&&fit.ready);
    assert(fit.sample({},0).skin.empty());
    FILE* file=_wfopen(L"artifacts/armorfit-private/GroveCraft-armor-fit.bin",L"rb");assert(file);
    std::vector<unsigned char> bytes(234764);assert(std::fread(bytes.data(),1,bytes.size(),file)==bytes.size());std::fclose(file);
    auto saved=fit.sample({0,1.6f,0},1).rest;
    unsigned badBone=999;std::memcpy(bytes.data()+20+24+12,&badBone,4);
    assert(!fit.parse(bytes.data(),bytes.size()));auto kept=fit.sample({0,1.6f,0},1).rest;
    assert(saved.x==kept.x&&saved.y==kept.y&&saved.z==kept.z);
}
