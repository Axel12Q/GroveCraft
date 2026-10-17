#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <vector>
#include <mutex>
#include <thread>
#include <cassert>
#include <cmath>
#include "GameBindings.h"
#include "NativeCampaign.h"
namespace fs=std::filesystem;
static void write(const fs::path& p,const char* text){fs::create_directories(p.parent_path());std::ofstream(p,std::ios::binary)<<text;}
static std::string read(const fs::path& p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
static bool prepare(grove::NativeCampaign& c,const fs::path& file,grove::NativeCampaign::Pair& pair){
    std::thread guest([&]{
        for(int i=0;i<200;i++){std::ifstream in(c.root/L"control.txt");std::vector<std::string> lines;std::string line;
            while(std::getline(in,line))lines.push_back(line);
            in.close();
            if(lines.size()==7&&lines[2]=="save"){
                auto directory=c.root/L"snapshots"/lines[5];write(directory/L"world"/L"level.dat","synthetic Minecraft save");
                write(directory/L"files.sha256","synthetic acknowledgment fixture, not a Java snapshot");
                write(c.root/L"acks"/(lines[6]+".txt"),"saved\n");return;
            }Sleep(5);
        }
    });bool ok=c.prepareSave(file,pair,3000);guest.join();return ok;
}
static bool verify(grove::NativeCampaign& c,const grove::NativeCampaign::Pair& pair,const char* reply){
    std::thread guest([&]{
        for(int i=0;i<200;i++){std::ifstream in(c.root/L"control.txt");std::vector<std::string> lines;std::string line;
            while(std::getline(in,line))lines.push_back(line);in.close();
            if(lines.size()==7&&lines[2]=="verify"){
                assert(lines[5]==pair.snapshot);write(c.root/L"acks"/(lines[6]+".txt"),reply);return;
            }Sleep(5);
        }
    });bool ok=c.verifySnapshot(pair,3000);guest.join();c.resume();return ok;
}
int main(){
    grove::NativeCampaign c;c.root=fs::absolute(fs::path("artifacts")/("campaign-native-unit-"+grove::NativeCampaign::uuid()));
    fs::create_directories(c.root/L"acks");fs::create_directories(c.root/L"saves");c.enabled=true;
    c.blocked=true;c.newGame();assert(c.campaign.empty()&&c.blocked); // Failed hooks cannot start/unblock a world.
    c.supported=true;c.newGame();auto first=c.campaign;c.newGame();assert(c.campaign!=first);
    c.blocked=false;auto file=c.root/L"GTASAsf8.b";write(file,"old GTA");write(c.wallet()/L"GroveCraft-wallet-123456789abc.dat","receipt at old save");
    grove::NativeCampaign::Pair p;assert(prepare(c,file,p));write(file,"new GTA");assert(c.commit(file,p));c.resume();
    grove::NativeCampaign::Pair loaded;bool linked=false;assert(c.beforeLoad(file,loaded,linked)&&linked);assert(std::string(loaded.campaign)==c.campaign);
    assert(verify(c,loaded,"verified\n"));assert(!verify(c,loaded,"error invalid-snapshot\n"));
    assert(!c.verifySnapshot(loaded,1));c.resume();assert(read(file)=="new GTA");
    auto copy=c.root/L"GTASAsf7.b";fs::copy_file(file,copy);assert(c.beforeLoad(copy,loaded,linked)&&linked);
    write(file,"corrupt or externally replaced GTA");assert(!c.beforeLoad(file,loaded,linked));write(file,"new GTA");
    grove::NativeCampaign::Pair interrupted;assert(prepare(c,file,interrupted));write(file,"uncommitted native overwrite");
    assert(c.recover(file));assert(read(file)=="new GTA");assert(c.beforeLoad(file,loaded,linked)&&linked);c.resume();
    auto absent=c.root/L"GTASAsf6.b";grove::NativeCampaign::Pair initial;assert(prepare(c,absent,initial));write(absent,"partial native save");assert(c.recover(absent));assert(!fs::exists(absent));c.resume();
    auto snapshot=c.root/L"snapshots"/p.snapshot;write(snapshot/L"wallet"/L"GroveCraft-wallet-123456789abc.dat","tampered receipt");assert(!c.beforeLoad(file,loaded,linked));
    c.resume();grove::NativeCampaign::Pair timeout;assert(!c.prepareSave(copy,timeout,1));assert(read(copy)=="new GTA");
    assert(!grove::NativeCampaign::id("../old-world"));std::puts("PASS campaign identity, failed-hook guard, paired commit, load preflight success/error/timeout, copied slot, corruption, interrupted rollback, absent slot, wallet integrity and timeout; synthetic native/guest IO");
}
