#pragma once
#include <cmath>
#include <algorithm>
namespace grove {
struct Aim {bool valid{},hit{};int x{},y{},z{},face{1},px{},py{},pz{};double distance{};};
inline bool nativePrecedes(double distance,const Aim& voxel){
    return std::isfinite(distance)&&distance>=0&&(!voxel.hit||distance+.01<voxel.distance);
}
// Minecraft Direction ordinal: down/up/north/south/west/east.
template<class Solid> Aim aim(Point camera,Point direction,Point feet,Solid solid){
    Aim a;double length=std::sqrt(direction.x*direction.x+direction.y*direction.y+direction.z*direction.z);
    if(!std::isfinite(length)||length<0.5)return a;
    direction.x/=length;direction.y/=length;direction.z/=length;
    int oldX=std::floor(camera.x),oldY=std::floor(camera.y),oldZ=std::floor(camera.z);
    for(double distance=.04;distance<12;distance+=.04){
        int x=std::floor(camera.x+direction.x*distance),y=std::floor(camera.y+direction.y*distance),z=std::floor(camera.z+direction.z*distance);
        if(solid(x,y,z)){
            a.hit=true;a.x=x;a.y=y;a.z=z;
            // Compare native geometry with the block's entry face, not its
            // centre: terrain behind a visible block must not steal mining.
            double entryDistance=0;
            double origin[]={camera.x,camera.y,camera.z},dir[]={direction.x,direction.y,direction.z};
            int cell[]={x,y,z};
            for(int axis=0;axis<3;++axis)if(std::abs(dir[axis])>1e-12){
                double lo=(cell[axis]-origin[axis])/dir[axis],hi=(cell[axis]+1-origin[axis])/dir[axis];
                if(lo>hi)std::swap(lo,hi);entryDistance=std::max(entryDistance,lo);
            }
            a.distance=entryDistance;
            a.face=x!=oldX?(x>oldX?4:5):y!=oldY?(y>oldY?0:1):z!=oldZ?(z>oldZ?2:3):1;
            break;
        }
        oldX=x;oldY=y;oldZ=z;
    }
    if(!a.hit){
        // Empty GTA ground is not in Minecraft's world: intersect the local foot-height plane.
        double ground=std::ceil(feet.y-.0001);
        double distance=direction.y<-.02?(ground-camera.y)/direction.y:-1;
        Point point{feet.x+direction.x*3,feet.y,feet.z+direction.z*3};
        if(distance>0&&distance<12){point.x=camera.x+direction.x*distance;point.z=camera.z+direction.z*distance;}
        a.x=std::floor(point.x);a.y=ground;a.z=std::floor(point.z);a.face=1;
    }
    a.px=a.x;a.py=a.y;a.pz=a.z;
    if(a.hit){if(a.face<2)a.py+=a.face==0?-1:1;else if(a.face<4)a.pz+=a.face==2?-1:1;else a.px+=a.face==4?-1:1;}
    double dx=a.x+.5-feet.x,dy=a.y+.5-(feet.y+1),dz=a.z+.5-feet.z;
    a.valid=dx*dx+dy*dy+dz*dz<=36&&a.y>=-1000&&a.y<=1000;
    return a;
}
}
