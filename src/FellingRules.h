#pragma once
namespace grove {
// Rotate the original geometry around its severed base, with deterministic fall direction.
inline gta::Vec fallenPoint(gta::Vec p,gta::Vec pivot,float dx,float dy,float angle){
    float x=p.x-pivot.x,y=p.y-pivot.y,z=p.z-pivot.z;
    float along=x*dx+y*dy,side=-x*dy+y*dx,c=std::cos(angle),s=std::sin(angle);
    float a=along*c+z*s,h=z*c-along*s;
    return {pivot.x+a*dx-side*dy,pivot.y+a*dy+side*dx,pivot.z+h};
}
template<class Dug> bool severedBase(gta::Vec base,Dug dug,float radius=.2f){
    // Native trunks straddle the voxel grid. Requiring all four samples left an
    // unselectable bark sliver after cutting the base. Any cut through the small
    // root footprint severs it; upper trunk cuts never enter this footprint.
    // Broad stock trunks can have their selectable bark well outside the model
    // origin. Sample the bounded root footprint instead of only its centre cell.
    radius=std::clamp(radius,.2f,1.6f);
    for(float x=-radius;x<=radius+.001f;x+=radius/4)
        for(float y=-radius;y<=radius+.001f;y+=radius/4)
            if(x*x+y*y<=radius*radius+.001f&&dug({base.x+x,base.y+y,base.z+.5f}))return true;
    return false;
}
}
