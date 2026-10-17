#include <cstdint>
#include <cstring>
#include <vector>
#include <cassert>
#include <cstdio>
#include "libertycraft_protocol.h"
namespace p=libertycraft::proto;
#include "RenderBatchRules.h"
int main(){
    std::vector<unsigned char> ring(256);
    auto put=[&](unsigned cursor,unsigned type,unsigned bytes,unsigned value){
        unsigned offset=cursor%256;std::memcpy(ring.data()+offset,&type,4);
        std::memcpy(ring.data()+offset+4,&bytes,4);if(bytes)std::memcpy(ring.data()+offset+8,&value,4);
    };
    put(16,p::kRenSection,16,0);put(40,grove::renderBatchEnd,4,1);
    assert(grove::completeRenderBatch(ring.data(),256,16,56,1));
    assert(!grove::completeRenderBatch(ring.data(),256,16,40,1));
    assert(!grove::completeRenderBatch(ring.data(),256,16,56,2));
    put(40,grove::renderBatchEnd,4,2);
    assert(!grove::completeRenderBatch(ring.data(),256,16,56,1));
    put(240,p::kRenPad,0,0);put(256,p::kRenSection,16,0);put(280,grove::renderBatchEnd,4,1);
    assert(grove::completeRenderBatch(ring.data(),256,240,296,1));
    assert(!grove::completeRenderBatch(ring.data(),256,240,296,33));
    put(256,grove::renderBatchBegin,4,1);
    assert(!grove::completeRenderBatch(ring.data(),256,240,296,1));
    std::puts("Render transactions: complete/partial/wrap/count/nesting PASS");
}
