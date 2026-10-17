#include "ExcavationCache.h"
#include <cassert>
#include <cstdio>
int main(){
    grove::ExcavationCache c(16);std::vector<std::uint8_t> a{1,2},b{1,3},mesh{4,5,6,7};
    c.put(a,mesh,true);auto p=c.find(a);assert(p&&p->changed&&p->mesh==mesh);
    assert(!c.find(b)); // Vertex/UV/normal, transform or box changed: recompute.
    c.put(b,{9},false);assert(c.find(b)&&!c.find(b)->changed);
    assert(c.find(a)); // Recently used survives pressure instead of the other entry.
    c.put({8,8},{1,2,3,4,5,6,7,8},true);assert(c.find(a)&&!c.find(b)&&c.memory()<=16);
    c.put({9},std::vector<std::uint8_t>(20),true);assert(!c.find({9})&&c.memory()<=16);
    c.clear();assert(!c.find(a)&&c.memory()==0&&c.size()==0);
    std::puts("{\"passed\":true,\"exact_input_identity\":true,\"cached_no_change\":true,\"bounded_lru\":true,\"oversized_rejected\":true}");
}
