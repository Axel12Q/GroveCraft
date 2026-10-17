#include "StructureMapSnapshot.h"
#include <cassert>
#include <limits>
#include <cstdio>
int main(){
    grove::StructureMapSnapshot s;
    assert(!s.accept(0,0,0,0,2,71,0));
    assert(!s.accept(1,120,25,-340,0,71,1));assert(!s.accept(2,-90,18,77,0,71,2));
    assert(s.accept(65,0,0,0,2,71,0));assert(s.count==2&&s.points[0].x==120&&s.points[0].y==340&&s.points[0].z==25);
    // An incomplete next batch preserves the last complete publication.
    s.accept(0,0,0,0,2,72,0);s.accept(1,1,2,3,0,72,3);
    assert(!s.accept(65,0,0,0,2,72,0));assert(s.count==2&&s.commits==1);
    // Mixed-world token, duplicates and invalid geometry never commit.
    s.accept(0,0,0,0,1,73,0);s.accept(1,1,2,3,0,72,3);
    assert(!s.accept(65,0,0,0,1,73,0));
    s.accept(0,0,0,0,1,74,0);s.accept(1,1,2,3,0,74,3);s.accept(1,1,2,3,0,74,3);
    assert(!s.accept(65,0,0,0,1,74,0));
    s.accept(0,0,0,0,1,75,0);s.accept(1,std::numeric_limits<float>::quiet_NaN(),2,3,0,75,1);
    assert(!s.accept(65,0,0,0,1,75,0));assert(s.count==2);
    s.accept(0,0,0,0,65,76,0);assert(!s.accept(65,0,0,0,65,76,0));
    s.accept(0,0,0,0,0,77,0);assert(s.accept(65,0,0,0,0,77,0));assert(!s.count);
    std::puts("StructureMapSnapshotTest PASS");
}
