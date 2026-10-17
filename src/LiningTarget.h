#pragma once
#include <array>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <algorithm>
#include <limits>
namespace grove {
// Ray-pick the actual published lining, which has no original GTA COL face
// underneath the landscape. Ordinary block/mob/water meshes are never indexed.
class LiningTarget {
public:
    using Key=std::array<int,3>;
    using Vec=std::array<double,3>;
    static constexpr unsigned marker=256,triangleLimit=50000;
    struct Hit {bool valid{};Vec point{};Key cell{};int face{};double distance{};};
private:
    struct Triangle {Vec a,b,c;int face;};
    std::map<Key,std::map<Key,std::vector<Triangle>>> cells;
    std::map<Key,std::set<Key>> members;
    unsigned triangles{};
    static Vec sub(Vec a,Vec b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
    static Vec cross(Vec a,Vec b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
    static double dot(Vec a,Vec b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
    static Vec normal(int face){Vec n{};n[face<2?1:face<4?2:0]=(face&1)?1:-1;return n;}
    static double intersect(const Triangle& t,Vec from,Vec ray){
        if(dot(normal(t.face),ray)>=-1e-8)return -1;
        auto e1=sub(t.b,t.a),e2=sub(t.c,t.a),p=cross(ray,e2);double determinant=dot(e1,p);
        if(std::abs(determinant)<1e-10)return -1;double inv=1/determinant;
        auto s=sub(from,t.a);double u=dot(s,p)*inv;if(u<-.00001||u>1.00001)return -1;
        auto q=cross(s,e1);double v=dot(ray,q)*inv;if(v<-.00001||u+v>1.00001)return -1;
        double distance=dot(e2,q)*inv;return distance>=0?distance:-1;
    }
public:
    unsigned size()const{return triangles;}
    void clear(){cells.clear();members.clear();triangles=0;}
    void erase(Key section){
        auto m=members.find(section);if(m==members.end())return;
        for(auto key:m->second){auto c=cells.find(key);if(c==cells.end())continue;auto group=c->second.find(section);
            if(group!=c->second.end()){triangles-=unsigned(group->second.size());c->second.erase(group);}
            if(c->second.empty())cells.erase(c);
        }members.erase(m);
    }
    template<class Vertices> bool replace(Key section,const Vertices& mesh){
        erase(section);std::map<Key,std::vector<Triangle>> pending;unsigned count=0;
        for(unsigned i=0;i+2<mesh.size();i+=3){
            const auto& a=mesh[i];const auto& b=mesh[i+1];const auto& c=mesh[i+2];
            if(!(a.flags&marker)||!(b.flags&marker)||!(c.flags&marker))continue;
            int face=int((a.flags>>4)&7)-1;if(face<0||face>5||((b.flags>>4)&7)!=unsigned(face+1)||((c.flags>>4)&7)!=unsigned(face+1))continue;
            Triangle t{{a.x,a.z,-a.y},{b.x,b.z,-b.y},{c.x,c.z,-c.y},face};
            bool valid=true;for(auto v:{t.a,t.b,t.c})for(double x:v)valid=valid&&std::isfinite(x)&&std::abs(x)<10000;
            if(!valid)continue;
            auto n=normal(face);Key key{};for(int j=0;j<3;++j)key[j]=int(std::floor((t.a[j]+t.b[j]+t.c[j])/3-n[j]*.02));
            if(++count+triangles>triangleLimit)return false; // Fail closed, never retain an old mesh target.
            pending[key].push_back(t);
        }
        for(auto& p:pending){members[section].insert(p.first);cells[p.first][section]=std::move(p.second);}triangles+=count;return true;
    }
    template<class Eligible> Hit ray(Vec from,Vec direction,double range,Eligible eligible)const{
        Hit best;double length=std::sqrt(dot(direction,direction));
        if(!std::isfinite(length)||length<.001||!std::isfinite(range)||range<=0||range>16)return best;
        for(double x:from)if(!std::isfinite(x)||std::abs(x)>10000)return best;
        for(auto& x:direction)x/=length;
        Key cell{},step{};Vec delta{},next{};
        for(int j=0;j<3;++j){cell[j]=int(std::floor(from[j]));step[j]=direction[j]>0?1:-1;
            delta[j]=direction[j]==0?std::numeric_limits<double>::infinity():std::abs(1/direction[j]);
            next[j]=direction[j]==0?delta[j]:(direction[j]>0?cell[j]+1-from[j]:from[j]-cell[j])*delta[j];}
        for(unsigned visited=0;visited<64;++visited){
            auto found=cells.find(cell);
            if(found!=cells.end()&&eligible(cell))for(const auto& group:found->second)for(const auto& triangle:group.second){
                double d=intersect(triangle,from,direction);
                if(d<0||d>range||(best.valid&&d>=best.distance))continue;
                best={true,{from[0]+direction[0]*d,from[1]+direction[1]*d,from[2]+direction[2]*d},cell,triangle.face,d};
            }
            int axis=next[0]<=next[1]&&next[0]<=next[2]?0:next[1]<=next[2]?1:2;
            if(next[axis]>range||(best.valid&&next[axis]>best.distance+.0001))break;
            cell[axis]+=step[axis];next[axis]+=delta[axis];
        }return best;
    }
};
}
