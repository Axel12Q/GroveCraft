#pragma once
#include <filesystem>
#include <fstream>
#include <atomic>
#include <wincrypt.h>
#include <objbase.h>
#include "GroveReleasePaths.h"
namespace grove {
// Paired save metadata is outside GTA's binary save format. Immutable Minecraft
// snapshots and receipts are committed only after the original SaveSlot succeeds.
class NativeCampaign {
public:
    struct Pair {unsigned magic{0x47575031},version{1};char campaign[33]{},snapshot[33]{};unsigned char hash[32]{};};
    std::filesystem::path root;std::string campaign,work,request,action,snapshot;
    std::mutex controlMutex;
    std::atomic<bool> blocked{false};unsigned revision{};bool supported{},enabled{};
    bool testMode{};
    void testPrefix(){
        if(!testMode)return;auto prefix=(root.parent_path()/L"pair-test"/L"GTASAsf").string();
        if(prefix.size()+1>=104||!gta::accessible(reinterpret_cast<void*>(prefixGlobal),MAX_PATH,true))throw std::runtime_error("Unsafe test save prefix");
        std::filesystem::create_directories(root.parent_path()/L"pair-test");std::memcpy(reinterpret_cast<void*>(prefixGlobal),prefix.c_str(),prefix.size()+1);
    }
    std::uintptr_t saveCode{},loadCode{},resetCode{},prefixGlobal{},loadNameGlobal{};
    static std::string uuid(){GUID g{};if(FAILED(CoCreateGuid(&g)))return {};char b[33]{};
        auto* p=reinterpret_cast<unsigned char*>(&g);for(int i=0;i<16;i++)std::sprintf(b+i*2,"%02x",p[i]);return b;}
    static bool id(const std::string& s){return s.size()==32&&s.find_first_not_of("0123456789abcdef")==std::string::npos;}
    static std::wstring widen(const char* s){int n=MultiByteToWideChar(CP_ACP,0,s,-1,nullptr,0);if(n<1)return {};std::wstring out(n,L'\0');
        MultiByteToWideChar(CP_ACP,0,s,-1,out.data(),n);out.resize(n-1);return out;}
    static bool atomic(const std::filesystem::path& p,const void* bytes,DWORD size){
        auto temp=p;temp+=L".tmp";HANDLE h=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(h==INVALID_HANDLE_VALUE)return false;DWORD n{};bool ok=WriteFile(h,bytes,size,&n,nullptr)&&n==size&&FlushFileBuffers(h);CloseHandle(h);
        if(!ok)return false;
        for(int i=0;i<40;i++){if(MoveFileExW(temp.c_str(),p.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return true;
            DWORD error=GetLastError();if(error!=ERROR_SHARING_VIOLATION&&error!=ERROR_ACCESS_DENIED)return false;Sleep(5);}
        return false;}
    static bool hash(const std::filesystem::path& p,unsigned char (&out)[32]){
        HCRYPTPROV provider{};HCRYPTHASH digest{};if(!CryptAcquireContextW(&provider,nullptr,nullptr,PROV_RSA_AES,CRYPT_VERIFYCONTEXT))return false;
        bool ok=CryptCreateHash(provider,CALG_SHA_256,0,0,&digest)!=0;std::ifstream in(p,std::ios::binary);ok=ok&&bool(in);
        char b[65536];while(ok&&in){in.read(b,sizeof b);auto n=in.gcount();if(n)ok=CryptHashData(digest,reinterpret_cast<BYTE*>(b),DWORD(n),0)!=0;}
        DWORD n=32;ok=ok&&in.eof()&&CryptGetHashParam(digest,HP_HASHVAL,out,&n,0)&&n==32;
        if(digest)CryptDestroyHash(digest);CryptReleaseContext(provider,0);return ok;}
    static std::string hex(const unsigned char* p){char s[65]{};for(int i=0;i<32;i++)std::sprintf(s+i*2,"%02x",p[i]);return s;}
    static bool readPair(const std::filesystem::path& p,Pair& v){std::ifstream in(p,std::ios::binary);in.read(reinterpret_cast<char*>(&v),sizeof v);
        return in.gcount()==sizeof v&&in.peek()==EOF&&v.magic==0x47575031&&v.version==1&&v.campaign[32]==0&&v.snapshot[32]==0&&id(v.campaign)&&id(v.snapshot);}
    static std::filesystem::path companion(std::filesystem::path p,const wchar_t* suffix){p+=suffix;return p;}
    std::filesystem::path wallet()const{return root/L"wallets"/work;}
    std::wstring walletPrefix()const{return (wallet()/L"GroveCraft-wallet-").wstring();}
    static bool receiptName(const std::string& s){return s.size()==34&&s.substr(0,18)=="GroveCraft-wallet-"&&s.substr(30)==".dat"
        &&s.substr(18,12).find_first_not_of("0123456789abcdef")==std::string::npos;}
    bool verifiedWallet(const std::filesystem::path& directory){
        std::ifstream in(directory/L"wallet.index");if(!in)return false;std::set<std::string> names;std::string digest,name;
        while(in>>digest){if(!(in>>name)||digest.size()!=64||!receiptName(name)||!names.insert(name).second)return false;
            unsigned char bytes[32]{};if(!hash(directory/L"wallet"/name,bytes)||hex(bytes)!=digest)return false;}
        if(!in.eof()||!std::filesystem::is_directory(directory/L"wallet"))return false;
        for(const auto& f:std::filesystem::directory_iterator(directory/L"wallet"))if(!f.is_regular_file()||!names.count(f.path().filename().string()))return false;
        return true;
    }
    std::string ack()const{std::ifstream in(root/L"acks"/(request+".txt"));std::string s;std::getline(in,s);return s;}
    bool publish(const std::string& verb,const std::string& snap="-"){
        std::lock_guard<std::mutex> guard(controlMutex);
        action=verb;snapshot=snap;request=uuid();if(!id(request))return false;blocked=true;
        std::string s="GroveCraftCampaign1\n"+std::to_string(GetCurrentProcessId())+"\n"+action+"\n"+campaign+"\n"+work+"\n"+snapshot+"\n"+request+"\n";
        return atomic(root/L"control.txt",s.data(),DWORD(s.size()));}
    void newGame(){if(!enabled||!supported)return;campaign=uuid();work=uuid();++revision;
        std::filesystem::create_directories(wallet());publish("open");}
    void ensure(){if(enabled&&(campaign.empty()||action=="wait"))newGame();}
    void poll(){std::lock_guard<std::mutex> guard(controlMutex);if(enabled&&supported&&blocked&&(action=="open"||action=="restore")&&ack()=="ready")blocked=false;}
    std::filesystem::path savePath(int slot)const{char name[MAX_PATH]{};if(slot<0||slot>7||!gta::accessible(reinterpret_cast<void*>(prefixGlobal),sizeof name))return {};
        std::memcpy(name,reinterpret_cast<void*>(prefixGlobal),sizeof name-1);if(!*name)return {};
        return std::filesystem::path(widen(name)+std::to_wstring(slot+1)+L".b");}
    std::filesystem::path loadPath()const{char name[105]{};if(!gta::accessible(reinterpret_cast<void*>(loadNameGlobal),104))return {};
        std::memcpy(name,reinterpret_cast<void*>(loadNameGlobal),104);return std::filesystem::path(widen(name));}
    bool recover(const std::filesystem::path& file){
        auto pending=companion(file,L".grovecraft.pending");if(!std::filesystem::exists(pending))return true;
        Pair plan{},done{};unsigned char actual[32]{};if(!readPair(pending,plan))return false;
        auto side=companion(file,L".grovecraft");
        if(readPair(side,done)&&std::string(done.snapshot)==plan.snapshot&&hash(file,actual)&&!std::memcmp(actual,done.hash,32)){
            std::filesystem::remove(pending);return true;}
        auto backup=root/L"transactions"/plan.snapshot;
        if(std::filesystem::exists(backup/L"previous.b")){
            std::filesystem::copy_file(backup/L"previous.b",file,std::filesystem::copy_options::overwrite_existing);
            if(std::filesystem::exists(backup/L"previous.pair"))std::filesystem::copy_file(backup/L"previous.pair",side,std::filesystem::copy_options::overwrite_existing);
            else if(std::filesystem::exists(side))std::filesystem::remove(side);
        }else if(std::filesystem::exists(backup/L"no-previous")){
            if(std::filesystem::exists(file))std::filesystem::remove(file);
            if(std::filesystem::exists(side))std::filesystem::remove(side);
        }else return false;
        std::filesystem::remove(pending);return true;
    }
    bool prepareSave(const std::filesystem::path& file,Pair& plan,DWORD timeout=30000){
        if(blocked||!id(campaign)||!id(work)||file.empty()||!recover(file))return false;
        std::string snap=uuid();if(!id(snap)||!publish("save",snap))return false;
        auto until=GetTickCount64()+timeout;std::string reply;
        while(GetTickCount64()<until){reply=ack();if(!reply.empty())break;Sleep(20);}
        if(reply!="saved")return false;
        auto directory=root/L"snapshots"/snap;
        if(!std::filesystem::exists(directory/L"world"/L"level.dat")||!std::filesystem::exists(directory/L"files.sha256"))return false;
        std::filesystem::create_directories(directory/L"wallet");
        std::string walletIndex;
        for(const auto& f:std::filesystem::directory_iterator(wallet()))if(f.is_regular_file()&&f.path().extension()==L".dat"){
            auto name=f.path().filename().string();if(!receiptName(name))return false;
            auto target=directory/L"wallet"/f.path().filename();std::filesystem::copy_file(f.path(),target);
            unsigned char bytes[32]{};if(!hash(target,bytes))return false;walletIndex+=hex(bytes)+" "+name+"\n";
        }
        if(!atomic(directory/L"wallet.index",walletIndex.data(),DWORD(walletIndex.size()))||!verifiedWallet(directory))return false;
        auto backup=root/L"transactions"/snap;std::filesystem::create_directories(backup);
        if(std::filesystem::exists(file))std::filesystem::copy_file(file,backup/L"previous.b");
        else{std::ofstream marker(backup/L"no-previous");marker<<"1\n";}
        auto side=companion(file,L".grovecraft");if(std::filesystem::exists(side))std::filesystem::copy_file(side,backup/L"previous.pair");
        std::strcpy(plan.campaign,campaign.c_str());std::strcpy(plan.snapshot,snap.c_str());
        return atomic(companion(file,L".grovecraft.pending"),&plan,sizeof plan);
    }
    bool commit(const std::filesystem::path& file,Pair& plan){
        if(!hash(file,plan.hash))return false;
        if(!atomic(companion(file,L".grovecraft"),&plan,sizeof plan))return false;
        // A copied save can be located by its exact bytes even without its sidecar.
        atomic(root/L"saves"/(hex(plan.hash)+".pair"),&plan,sizeof plan);
        std::filesystem::remove(companion(file,L".grovecraft.pending"));return true;
    }
    void resume(){publish("idle");blocked=false;}
    bool verifySnapshot(const Pair& pair,DWORD timeout=30000){
        if(!publish("verify",pair.snapshot))return false;auto until=GetTickCount64()+timeout;
        while(GetTickCount64()<until){auto reply=ack();if(!reply.empty())return reply=="verified";Sleep(20);}return false;
    }
    bool beforeLoad(const std::filesystem::path& file,Pair& pair,bool& linked){
        linked=false;if(file.empty()||!recover(file))return false;unsigned char actual[32]{};if(!hash(file,actual))return false;
        auto side=companion(file,L".grovecraft");
        if(std::filesystem::exists(side)){if(!readPair(side,pair)||std::memcmp(actual,pair.hash,32))return false;linked=true;}
        else{auto index=root/L"saves"/(hex(actual)+".pair");if(std::filesystem::exists(index)){
            if(!readPair(index,pair)||std::memcmp(actual,pair.hash,32))return false;linked=true;}}
        if(linked&&(!std::filesystem::exists(root/L"snapshots"/pair.snapshot/L"files.sha256")||!verifiedWallet(root/L"snapshots"/pair.snapshot)))return false;
        return true;
    }
    void loaded(const Pair& pair,bool linked){
        campaign=linked?pair.campaign:uuid();work=uuid();++revision;std::filesystem::create_directories(wallet());
        if(linked){auto directory=root/L"snapshots"/pair.snapshot/L"wallet";
            if(!std::filesystem::exists(directory))throw std::runtime_error("Missing paired wallet snapshot");
            for(const auto& f:std::filesystem::directory_iterator(directory))if(f.is_regular_file()&&f.path().extension()==L".dat")
                std::filesystem::copy_file(f.path(),wallet()/f.path().filename());}
        publish(linked?"restore":"open",linked?pair.snapshot:"-");
    }
    bool init(){
        wchar_t path[32768]{};DWORD n=GetEnvironmentVariableW(L"GROVECRAFT_CAMPAIGN_ROOT",path,32768);
        if(n>=32768)return false;
        if(n)root=std::filesystem::path(path);
        else {
            wchar_t fixture[256]{};
            if(GetEnvironmentVariableW(L"GROVECRAFT_TEST_WORLD",fixture,256))return false;
            auto home=releaseHome();if(home.empty())return false;root=home/L"campaigns";
        }
        if(!root.is_absolute())return false;
        gta::OriginalImage image;if(!image.load())return false;
        auto save=image.scan("55 8B EC 0F BE 45 08 50 E8 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00 E8 ? ? ? ? E8 ? ? ? ? 6A 00 E8 ? ? ? ? 83 C4 08 F6 D8 1A C0 24 FE 04 02");
        auto load=image.scan("55 8B EC 8B 45 08 81 EC 7C 02 00 00 53 C6 00 00 33 DB C6 05 ? ? ? ? 00 89 1D ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8");
        auto reset=image.scan("53 56 57 B9 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 50 B9 ? ? ? ? E8 ? ? ? ?");
        auto prefix=image.scan("55 8B EC 8B 45 08 68 ? ? ? ? 50 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 10 5D C3");
        if(save.size()!=1||load.size()!=1||reset.size()!=1||prefix.size()!=1)return false;
        saveCode=image.code(save[0]);loadCode=image.code(load[0]);resetCode=image.code(reset[0]);DWORD p{},l{};
        if(!gta::read(prefix[0]+18,p)||!gta::read(load[0]+42,l))return false;prefixGlobal=image.global(p);loadNameGlobal=image.global(l);
        wchar_t acceptance[4]{};testMode=GetEnvironmentVariableW(L"GROVECRAFT_CAMPAIGN_ACCEPTANCE",acceptance,4)==1&&acceptance[0]==L'1';
        if(testMode&&root.filename()!=L"pair-test-data")return false;
        std::filesystem::create_directories(root/L"acks");std::filesystem::create_directories(root/L"snapshots");std::filesystem::create_directories(root/L"saves");
        // Main-menu connection must not open the old global Lab world.
        campaign=uuid();work=uuid();publish("wait");enabled=supported=true;return true;
    }
};
}
