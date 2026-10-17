#pragma once
namespace grove {
// Reject a section only if all eight corners lie outside one homogeneous clip plane.
// Crossing the near plane remains visible; never divide by w or cull behind-camera corners alone.
inline bool visibleSection(const D3DMATRIX& m,gta::Vec lo,gta::Vec hi){
    unsigned common=63;
    for(int i=0;i<8;++i){float x=i&1?hi.x:lo.x,y=i&2?hi.y:lo.y,z=i&4?hi.z:lo.z;
        float a=x*m._11+y*m._21+z*m._31+m._41,b=x*m._12+y*m._22+z*m._32+m._42;
        float c=x*m._13+y*m._23+z*m._33+m._43,w=x*m._14+y*m._24+z*m._34+m._44;
        if(!std::isfinite(a+b+c+w))return true;
        unsigned out=(a<-w?1:0)|(a>w?2:0)|(b<-w?4:0)|(b>w?8:0)|(c<0?16:0)|(c>w?32:0);common&=out;
    }return common==0;
}
inline D3DMATRIX viewProjection(const D3DMATRIX& a,const D3DMATRIX& b){
    D3DMATRIX out{};for(int i=0;i<4;++i)for(int j=0;j<4;++j)for(int k=0;k<4;++k)out.m[i][j]+=a.m[i][k]*b.m[k][j];return out;
}
}
