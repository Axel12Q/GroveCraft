#pragma once
#include <cstddef>
namespace grove {
// Durable repair receipt lives in the same snapshotted wallet directory as trade receipts.
class NativeRoadside {
    struct Receipt {unsigned magic=0x47575231,version=1,lo{},hi{},id{},phase{},pid{},vehicle{},address{},generation{};float before{},after{};int result{};unsigned checksum{};};
    std::wstring prefix;
    static unsigned sum(const Receipt& r){unsigned h=2166136261u;auto p=reinterpret_cast<const BYTE*>(&r);for(unsigned i=0;i<offsetof(Receipt,checksum);i++)h=(h^p[i])*16777619u;return h;}
    std::wstring path(unsigned lo,unsigned hi)const{wchar_t token[40];std::swprintf(token,40,L"repair-%06x%06x.dat",hi,lo);return prefix+token;}
    bool write(Receipt r){r.checksum=sum(r);auto p=path(r.lo,r.hi),tmp=p+L".tmp";HANDLE f=CreateFileW(tmp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(f==INVALID_HANDLE_VALUE)return false;DWORD n{};bool ok=WriteFile(f,&r,sizeof r,&n,nullptr)&&n==sizeof r&&FlushFileBuffers(f);CloseHandle(f);
        return ok&&MoveFileExW(tmp.c_str(),p.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);}
    bool load(unsigned lo,unsigned hi,Receipt& r){auto p=path(lo,hi);HANDLE f=CreateFileW(p.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
        if(f==INVALID_HANDLE_VALUE){if(GetLastError()!=ERROR_FILE_NOT_FOUND)return false;r={};r.lo=lo;r.hi=hi;return true;}
        DWORD n{},size=GetFileSize(f,nullptr);bool ok=ReadFile(f,&r,sizeof r,&n,nullptr)&&n==sizeof r&&size==sizeof r;CloseHandle(f);
        return ok&&r.magic==0x47575231&&r.version==1&&r.lo==lo&&r.hi==hi&&r.checksum==sum(r)&&r.phase<=2;}
public:
    unsigned lastId{},repairs{};int lastResult{};float before{},after{};
    void setPrefix(std::wstring p){prefix=std::move(p);}
    int request(NativeVehicles& cars,const gta::Snapshot& s,unsigned id,unsigned lo,unsigned hi,unsigned vehicle){
        if(prefix.empty()||!id||id>0x7ffffffe||lo>0xffffff||hi>0xffffff||(!lo&&!hi)||!vehicle)return -2;
        if(s.menu||s.mission||s.vehicle||!s.valid||s.health<=0)return -1;
        Receipt r;if(!load(lo,hi,r))return -2;lastId=id;
        if(id<r.id||id>r.id+1||(id==r.id&&vehicle!=r.vehicle))return lastResult=-2;
        if(id==r.id&&r.phase==2)return lastResult=r.result;
        std::uintptr_t target{};BYTE generation{};float health{};bool valid=cars.repairTarget(s,vehicle,target,generation,health);
        if(id==r.id&&r.phase==1){
            // Never apply a prepared effect to a recycled pool slot or a different process.
            if(!valid||r.pid!=GetCurrentProcessId()||r.address!=target||r.generation!=generation||(health!=r.before&&health!=r.after))return lastResult=-2;
        }else{
            r.id=id;r.vehicle=vehicle;r.result=0;
            if(!valid||health>=900){r.phase=2;return lastResult=write(r)?0:-1;}
            r.pid=GetCurrentProcessId();r.address=unsigned(target);r.generation=generation;r.before=health;r.after=std::min(900.f,health+250.f);r.phase=1;
            if(!write(r))return -1;
        }
        if(health==r.before)std::memcpy(reinterpret_cast<void*>(target),&r.after,4);
        r.phase=2;r.result=1;if(!write(r))return -1;
        before=r.before;after=r.after;++repairs;return lastResult=1;
    }
};
}
