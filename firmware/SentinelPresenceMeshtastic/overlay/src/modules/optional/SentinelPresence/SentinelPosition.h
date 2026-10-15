// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "SentinelMavlink.h"
#include <cstdio>

namespace sentinelPosition {
inline bool epochValid(uint32_t seconds) {return seconds>=1577836800U && seconds<4102444800U;}
inline bool parseNode(const char *s,uint32_t &node) {
    if(strlen(s)!=8) return false;
    node=0;
    for(unsigned i=0;i<8;++i) {
        char c=s[i];int n=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;
        if(n<0)return false;node=(node<<4)|uint32_t(n);
    }
    return true;
}
inline int32_t e5(int32_t e7) {int64_t n=int64_t(e7)+50;return n>=0?n/100:-((-n+99)/100);}
struct Position {
    bool valid=false,fix=false,hasPosition=false;uint32_t owner=0,at=0,fixAt=0,epoch=0,fixEpoch=0;
    int32_t lat=0,lon=0;
    bool observe(const sentinelMav::Frame &f,uint32_t node,uint8_t system,uint8_t component,uint32_t now,uint32_t utc) {
        int type=sentinelMav::validate(f.data,f.size);
        if(type<0)return false;
        bool v2=f.data[0]==0xfd;unsigned header=v2?10:6;
        if(f.data[v2?5:3]!=system || f.data[v2?6:4]!=component)return false;
        uint32_t id=sentinelMav::types[type].id;
        if(id!=24 && id!=33)return false;
        auto byte=[&](unsigned i)->uint8_t{return i<f.data[1]?f.data[header+i]:0;};
        auto u32=[&](unsigned i)->uint32_t{return uint32_t(byte(i))|(uint32_t(byte(i+1))<<8)|(uint32_t(byte(i+2))<<16)|(uint32_t(byte(i+3))<<24);};
        uint32_t stamp=epochValid(utc)?utc:0;
        if(id==24) {
            // A new bad fix invalidates the previous sample immediately.
            owner=node;valid=false;fix=false;hasPosition=false;
            if(byte(28)<3 || byte(28)>8)return false;
            uint64_t micros=uint64_t(u32(0))|(uint64_t(u32(4))<<32);
            uint64_t seconds=micros/1000000ULL;
            if(!stamp && seconds<=UINT32_MAX && epochValid(uint32_t(seconds)))stamp=uint32_t(seconds);
        } else {
            if(!fix || owner!=node || uint32_t(now-fixAt)>=15000)return false;
            if(!stamp && epochValid(fixEpoch))stamp=fixEpoch+uint32_t(now-fixAt)/1000;
        }
        int32_t latitude=int32_t(u32(id==24?8:4)),longitude=int32_t(u32(id==24?12:8));
        if(latitude < -900000000 || latitude > 900000000 || longitude < -1800000000 || longitude > 1800000000) {valid=false;fix=false;return false;}
        if(id==24) {fix=true;fixAt=now;fixEpoch=stamp;}
        lat=latitude;lon=longitude;at=now;epoch=stamp;owner=node;hasPosition=true;valid=epochValid(stamp);
        return valid;
    }
    bool fresh(uint32_t now) const {return valid && fix && uint32_t(now-at)<15000 && uint32_t(now-fixAt)<15000;}
};
// Existing X Suite Sentinel v1: position-only report, label flag, zero sensor readings.
inline size_t wrapper(const Position &p,const char *label,uint32_t packetId,char *out,size_t capacity) {
    if(!p.valid || !epochValid(p.epoch))return 0;
    uint8_t bytes[52]={};size_t n=19;
    bytes[0]=1;bytes[1]=0;
    auto be=[&](unsigned at,uint32_t v){for(unsigned i=0;i<4;++i)bytes[at+i]=uint8_t(v>>(24-8*i));};
    be(2,p.epoch/60);be(6,uint32_t(e5(p.lat)));be(10,uint32_t(e5(p.lon)));be(14,p.owner);
    size_t length=strlen(label);if(length>32)length=32;
    if(length){bytes[18]=4;bytes[n++]=uint8_t(length);memcpy(bytes+n,label,length);n+=length;}
    constexpr char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    char encoded[72]={};size_t z=0;
    for(size_t i=0;i<n;i+=3) {
        uint32_t v=uint32_t(bytes[i])<<16;if(i+1<n)v|=uint32_t(bytes[i+1])<<8;if(i+2<n)v|=bytes[i+2];
        encoded[z++]=alphabet[(v>>18)&63];encoded[z++]=alphabet[(v>>12)&63];
        if(i+1<n)encoded[z++]=alphabet[(v>>6)&63];if(i+2<n)encoded[z++]=alphabet[v&63];
    }
    int count=snprintf(out,capacity,"X1.11.C.%08lX.1/1.%s",(unsigned long)packetId,encoded);
    return count>0 && size_t(count)<capacity?size_t(count):0;
}
}
