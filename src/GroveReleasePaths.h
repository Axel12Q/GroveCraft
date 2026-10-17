#pragma once
#include <filesystem>
#include <fstream>
namespace grove {
inline std::filesystem::path releaseHome(){
    wchar_t path[32768]{};
    auto count=GetEnvironmentVariableW(L"LOCALAPPDATA",path,32768);
    if(!count||count>=32768)return {};
    auto home=std::filesystem::path(path)/L"GroveCraft";
    if(!home.is_absolute())return {};
    std::ifstream marker(home/L"installed-v1.txt");std::string line;
    if(!std::getline(marker,line))return {};
    if(!line.empty()&&line.back()=='\r')line.pop_back();
    if(line!="GroveCraftRelease1")return {};
    return home;
}
}
