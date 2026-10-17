#define GROVE_RENDERER_UNIT
#include "bridge.cpp"
#include <cassert>
int main(){
    grove::Armor model;
    std::vector<unsigned char> packet(8+sizeof(p::RenBatch)+3*sizeof(p::RenVertex));
    DWORD counts[]={1,3};std::memcpy(packet.data(),counts,8);
    p::RenBatch b{1,0,3,2<<8};std::memcpy(packet.data()+8,&b,sizeof b);
    p::RenVertex v[]={{-.3f,.75f,0,0,0,0xffffffff,0,0},{.3f,.75f,0,1,0,0xffffffff,0,0},{0,1.5f,0,0,1,0xffffffff,0,0}};
    std::memcpy(packet.data()+8+sizeof b,v,sizeof v);
    assert(model.frame(packet.data(),packet.size())&&model.vertices.size()==3);
    b.flags|=0x1000;std::memcpy(packet.data()+8,&b,sizeof b);
    assert(!model.frame(packet.data(),packet.size())&&model.vertices.size()==3); // no held-item contamination
    b.flags=2<<8;b.count=6;std::memcpy(packet.data()+8,&b,sizeof b);
    assert(!model.frame(packet.data(),packet.size())&&model.vertices.size()==3); // transactional rejection
    BYTE empty[8]{};assert(model.frame(empty,sizeof empty)&&model.vertices.empty()&&model.batches.empty());
}
