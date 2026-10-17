#pragma once
namespace grove {
// A cached COM interface must outlive the caller's EndScene reference, including
// temporary movie devices. Keep one reference until replacement or Reset.
template<class Device> class DeviceLease {
    Device* value{};
public:
    DeviceLease()=default;
    DeviceLease(const DeviceLease&)=delete;
    DeviceLease& operator=(const DeviceLease&)=delete;
    ~DeviceLease(){reset();}
    Device* get()const{return value;}
    void reset(Device* next=nullptr){
        if(next==value)return;
        if(next)next->AddRef();
        auto* previous=value;value=next;
        if(previous)previous->Release();
    }
};
}
