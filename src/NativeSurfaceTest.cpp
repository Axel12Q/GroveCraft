#include "SATerrain.h"
#include <cassert>
#include <cstdio>
int main(){
    namespace p=libertycraft::proto;
    lc::col::Tri facade{};
    float wall[]={2244,22.9f,1263.7109375f,2248,22.9f,1263.7109375f,2244,26,1263.7109375f};
    assert(grove::exactFacade(wall,0,facade));
    assert(facade.kind==lc::col::kWall&&!(facade.flags&p::kTriTerrain));
    assert(std::memcmp(wall,facade.v,sizeof wall)==0);
    assert(!grove::exactFacade(wall,111,facade));
    float floor[]={0,2,0,1,2,0,0,2,1};assert(!grove::exactFacade(floor,0,facade));
    wall[0]=NAN;assert(!grove::exactFacade(wall,0,facade));
    // Negative coordinates and region boundaries must include all columns
    // consulted by a lining face, with the actual hit scheduled first.
    for(int x=-24;x<=24;++x)for(int z=-24;z<=24;++z){
        auto columns=grove::surfaceColumns(x,z);
        assert(columns.front()==(std::array<int,2>{int(std::floor(x/8.0)),int(std::floor(z/8.0))}));
        assert(columns.size()<=9);
        for(int dx=-4;dx<=4;dx++)for(int dz=-4;dz<=4;dz++){
            std::array<int,2> needed{int(std::floor((x+dx)/8.0)),int(std::floor((z+dz)/8.0))};
            assert(std::count(columns.begin(),columns.end(),needed)==1);
        }
    }
    // Real COL material at the failing below-ground hit (611.83,198.78,19.923).
    assert(grove::landscapeMaterial(111)==p::kDigDirt);
    for(unsigned m=111;m<=114;++m){
        assert(grove::landscapeMaterial(m)==p::kDigDirt);
        assert(grove::earthSample(128+grove::landscapeMaterial(m)));
    }
    assert(grove::landscapeMaterial(109)==p::kDigStone);
    assert(grove::landscapeMaterial(110)==p::kDigMud);
    // Do not reinterpret furnishings, floors, trees or unknowns as this ground.
    for(unsigned m:{42u,90u,108u,115u,158u,255u})assert(!grove::landscapeMaterial(m));
    std::puts("{\"passed\":true,\"real_cunteground13_material\":111,\"landscape_variants\":true,\"unrelated_materials_unchanged\":true}");
}
