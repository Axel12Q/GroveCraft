#pragma once
#include <cstdint>
#include <map>
#include <vector>
#include <algorithm>
namespace grove {
// CPU bytes only: no stale GPU handles, and exact input equality after hashing.
// The key includes current vertex attributes, world positions, declaration and
// excavation boxes, so streaming, lighting and adjacent cuts invalidate it.
class ExcavationCache {
public:
    struct Entry {std::vector<std::uint8_t> key,mesh;bool changed{};std::uint64_t used{};};
private:
    std::map<std::uint64_t,Entry> entries;
    std::size_t bytes{},budget;std::uint64_t clock{};
    static std::uint64_t hash(const std::vector<std::uint8_t>& key){
        std::uint64_t h=14695981039346656037ull;for(auto b:key){h^=b;h*=1099511628211ull;}return h;
    }
public:
    explicit ExcavationCache(std::size_t limit=64u*1024*1024):budget(limit){}
    const Entry* find(const std::vector<std::uint8_t>& key){
        auto i=entries.find(hash(key));if(i==entries.end()||i->second.key!=key)return nullptr;
        i->second.used=++clock;return &i->second;
    }
    void put(std::vector<std::uint8_t> key,std::vector<std::uint8_t> mesh,bool changed){
        auto size=key.size()+mesh.size();if(size>budget)return;
        auto h=hash(key);auto old=entries.find(h);
        if(old!=entries.end()){bytes-=old->second.key.size()+old->second.mesh.size();entries.erase(old);}
        while(!entries.empty()&&(bytes+size>budget||entries.size()>=128)){
            auto oldest=std::min_element(entries.begin(),entries.end(),[](const auto& a,const auto& b){return a.second.used<b.second.used;});
            bytes-=oldest->second.key.size()+oldest->second.mesh.size();entries.erase(oldest);
        }
        bytes+=size;entries.emplace(h,Entry{std::move(key),std::move(mesh),changed,++clock});
    }
    void clear(){entries.clear();bytes=0;}
    std::size_t memory()const{return bytes;}
    std::size_t size()const{return entries.size();}
};
}
