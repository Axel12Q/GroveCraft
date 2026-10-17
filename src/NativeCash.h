#pragma once
#include <cstddef>
// Durable, sequential receipts; never replay an acknowledged payment on reconnect.
// GTA's money still follows GTA's save slot. This journal is not a replacement savegame.
namespace grove {
class NativeCash {
    struct Receipt {unsigned magic=0x47574331,version=1,lo{},hi{},id{},phase{};int before{},after{},amount{},result{};unsigned checksum{};};
    std::wstring prefix;
    static unsigned sum(const Receipt& r){unsigned h=2166136261u;auto p=reinterpret_cast<const unsigned char*>(&r);
        for(unsigned n=0;n<offsetof(Receipt,checksum);++n)h=(h^p[n])*16777619u;return h;}
    std::wstring path(unsigned lo,unsigned hi)const{wchar_t token[32];std::swprintf(token,32,L"%06x%06x.dat",hi,lo);return prefix+token;}
    bool write(const Receipt& value)const{
        Receipt r=value;r.checksum=sum(r);auto file=path(r.lo,r.hi),temp=file+L".tmp";
        HANDLE h=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(h==INVALID_HANDLE_VALUE)return false;DWORD written{};
        bool ok=WriteFile(h,&r,sizeof r,&written,nullptr)&&written==sizeof r&&FlushFileBuffers(h);CloseHandle(h);
        return ok&&MoveFileExW(temp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    }
    bool load(unsigned lo,unsigned hi,Receipt& r)const{
        auto file=path(lo,hi);HANDLE h=CreateFileW(file.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
        if(h==INVALID_HANDLE_VALUE){if(GetLastError()!=ERROR_FILE_NOT_FOUND)return false;r={};r.lo=lo;r.hi=hi;return true;}
        DWORD got{},size=GetFileSize(h,nullptr);bool ok=ReadFile(h,&r,sizeof r,&got,nullptr)&&got==sizeof r&&size==sizeof r;CloseHandle(h);
        return ok&&r.magic==0x47574331&&r.version==1&&r.lo==lo&&r.hi==hi&&r.checksum==sum(r)&&r.phase<=2;
    }
public:
    unsigned lastId{};int lastAmount{},lastResult{},cash=-1;
    void setPrefix(std::wstring value){prefix=std::move(value);}
    // 1 committed, 0 insufficient/cap, -2 stale/corrupt/ambiguous, -1 retry safely.
    int request(const gta::Bindings& b,const gta::Snapshot& s,unsigned id,int amount,unsigned lo,unsigned hi){
        if(prefix.empty()||!id||id>0x7fffffffu||(!lo&&!hi)||lo>0xffffff||hi>0xffffff||!amount||amount<-5000||amount>5000)return -2;
        if(!b.money(s,cash)||s.menu||s.mission||s.vehicle||s.health<=0||s.state<=0||s.state>=20)return -1;
        Receipt r;if(!load(lo,hi,r))return -2;
        lastId=id;lastAmount=amount;
        if(id<r.id||id>r.id+1||(id==r.id&&amount!=r.amount))return lastResult=-2;
        if(id==r.id&&r.phase==2)return lastResult=r.result;
        if(id==r.id&&r.phase==1){
            // A prepared journal is recoverable only at an exact old/new balance.
            if(cash!=r.before&&cash!=r.after)return lastResult=-2;
            if(cash==r.before&&!b.changeMoney(s,r.before,r.after))return lastResult=-1;
            r.phase=2;r.result=1;if(!write(r))return lastResult=-1;
            b.money(s,cash);return lastResult=1;
        }
        r.id=id;r.before=cash;r.amount=amount;
        long long after=static_cast<long long>(cash)+amount;
        if(after<0||after>99999999){r.after=cash;r.phase=2;r.result=0;return lastResult=write(r)?0:-1;}
        r.after=int(after);r.phase=1;r.result=1;
        if(!write(r))return lastResult=-1;
        if(!b.changeMoney(s,r.before,r.after))return lastResult=-1;
        r.phase=2;if(!write(r))return lastResult=-1;
        b.money(s,cash);return lastResult=1;
    }
};
}
