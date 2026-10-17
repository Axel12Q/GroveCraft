#pragma once
namespace grove {
constexpr unsigned renderBatchBegin=0x47434231,renderBatchEnd=0x47434232;
// Validate a fully published bounded transaction before applying any packet.
// Padding at the ring wrap is transport, not one of its application packets.
inline bool completeRenderBatch(const unsigned char* ring,std::uint64_t capacity,
        std::uint64_t cursor,std::uint64_t head,unsigned count){
    if(!count||count>32||cursor>head||head-cursor>capacity)return false;
    auto start=cursor;
    for(unsigned i=0;i<=count;){
        if(cursor>=head||cursor-start>2*1024*1024)return false;
        auto offset=cursor%capacity,remain=capacity-offset;
        if(remain<8){cursor+=remain;continue;}
        unsigned type{},bytes{};std::memcpy(&type,ring+offset,4);std::memcpy(&bytes,ring+offset+4,4);
        if(type==p::kRenPad){cursor+=remain;continue;}
        auto total=(8ull+bytes+7)&~7ull;
        if(total>remain||total>head-cursor||total>capacity/2)return false;
        if(i==count){unsigned endCount{};
            if(type!=renderBatchEnd||bytes!=4)return false;
            std::memcpy(&endCount,ring+offset+8,4);return endCount==count;
        }
        if(type==renderBatchBegin||type==renderBatchEnd)return false;
        cursor+=total;++i;
    }return false;
}
}
