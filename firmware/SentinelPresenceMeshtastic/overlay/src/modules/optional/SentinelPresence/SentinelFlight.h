// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "SentinelPosition.h"

namespace sentinelFlight {
using sentinelMav::Frame;
inline uint8_t byte(const Frame &f,unsigned offset) {
    return offset<f.data[1]?f.data[(f.data[0]==0xfd?10:6)+offset]:0;
}
inline uint32_t word(const Frame &f,unsigned offset) {
    return uint32_t(byte(f,offset))|(uint32_t(byte(f,offset+1))<<8)|(uint32_t(byte(f,offset+2))<<16)|(uint32_t(byte(f,offset+3))<<24);
}
inline int id(const Frame &f) {
    int t=sentinelMav::validate(f.data,f.size,true);
    return t<0?-1:int(sentinelMav::flightTypes[t].id);
}
inline bool identity(const Frame &f,uint8_t system,uint8_t component) {
    return f.data[f.data[0]==0xfd?5:3]==system && f.data[f.data[0]==0xfd?6:4]==component;
}
inline bool fresh(uint32_t stamp,uint32_t utc) {
    return sentinelPosition::epochValid(utc) && sentinelPosition::epochValid(stamp) &&
        int64_t(utc)-stamp>=-2 && int64_t(utc)-stamp<=5;
}
inline bool request(const Frame &f,uint8_t system,uint8_t component) {
    int message=id(f);unsigned target=0;
    // Only the existing XTOC supervised command/waypoint protocol. Endpoint verifies signatures.
    if(message<0 || f.data[0]!=0xfd || !(f.data[2]&1) || !identity(f,255,190))return false;
    switch(message) {
        case 0:return byte(f,4)==6 && byte(f,5)==8; // Paired GCS link heartbeat, not a flight action.
        case 76: {
            unsigned command=byte(f,28)|(unsigned(byte(f,29))<<8);
            if(command!=20 && command!=193)return false;
            target=30;break;
        }
        case 39:case 73:
            if((byte(f,30)|(unsigned(byte(f,31))<<8))!=16)return false;
            target=32;break;
        case 43:case 47:target=0;break;
        case 44:case 51:target=2;break;
        default:return false;
    }
    return byte(f,target)==system && byte(f,target+1)==component;
}
inline bool telemetry(int message) {
    return message==0 || message==1 || message==24 || message==30 || message==33 || message==42 ||
        message==74 || message==147 || message==242;
}
inline bool response(int message) {
    return message==39 || message==40 || message==44 || message==47 || message==51 || message==73 || message==77;
}
inline size_t encode(const Frame &f,uint32_t utc,uint8_t *out) {
    memcpy(out,"SMF1",4);
    for(unsigned i=0;i<4;++i)out[4+i]=uint8_t(utc>>(8*i));
    memcpy(out+8,f.data,f.size);return f.size+8;
}
inline bool decode(const uint8_t *data,size_t size,uint32_t utc,Frame &out) {
    if(size<16 || size>sentinelMav::maxFrame+8 || memcmp(data,"SMF1",4))return false;
    uint32_t stamp=uint32_t(data[4])|(uint32_t(data[5])<<8)|(uint32_t(data[6])<<16)|(uint32_t(data[7])<<24);
    if(!fresh(stamp,utc) || sentinelMav::validate(data+8,size-8,true)<0)return false;
    out.size=size-8;memcpy(out.data,data+8,out.size);return true;
}
template<unsigned N> struct Queue {
    Frame frames[N];uint32_t times[N]={};unsigned head=0,tail=0,offset=0;
    bool push(const Frame &f,uint32_t now) {
        unsigned next=(tail+1)%N;if(next==head)return false;
        frames[tail]=f;times[tail]=now;tail=next;return true;
    }
    bool empty() const {return head==tail;}
    void pop(){head=(head+1)%N;offset=0;}
};
}
