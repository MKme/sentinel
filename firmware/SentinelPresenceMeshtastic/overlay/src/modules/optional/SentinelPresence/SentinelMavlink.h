// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace sentinelMav {
// Telemetry only. CRC extras are MAVLink common/standard/minimal wire constants.
struct Type { uint32_t id; uint8_t extra, v1Length, maxLength; };
constexpr Type types[]={{0,50,9,9},{1,124,31,43},{24,24,30,52},{30,39,28,28},{33,104,28,28}};
constexpr unsigned typeCount=sizeof(types)/sizeof(types[0]);
// Flight mode has its own explicit common-dialect allowlist; legacy SMV1 stays telemetry-only.
constexpr Type flightTypes[]={{0,50,9,9},{1,124,31,31},{24,24,30,52},{30,39,28,28},{33,104,28,28},
    {39,254,37,38},{40,230,4,5},{42,28,2,6},{43,132,2,3},{44,221,4,5},{47,153,3,4},
    {51,196,4,5},{73,38,37,38},{76,152,33,33},{77,143,3,10},{74,20,20,20},{147,154,36,54},{242,104,52,60}};
constexpr unsigned flightTypeCount=sizeof(flightTypes)/sizeof(flightTypes[0]);
constexpr size_t maxFrame=96; // Signed HOME_POSITION is 85 bytes; no fragmentation.
inline uint16_t crcByte(uint16_t crc,uint8_t b) {
    uint8_t t=b^(crc&255); t^=uint8_t(t<<4);
    return (crc>>8)^(uint16_t(t)<<8)^(uint16_t(t)<<3)^(uint16_t(t)>>4);
}
inline int validate(const uint8_t *b,size_t n,bool flight=false) {
    const Type *table=flight?flightTypes:types;const unsigned count=flight?flightTypeCount:typeCount;
    if(n<8 || (b[0]!=0xfe && b[0]!=0xfd)) return -1;
    bool v2=b[0]==0xfd;
    if(v2 && (n<12 || (b[2]&~1))) return -1;
    size_t header=v2?10:6, body=header+b[1];
    if(n!=body+2+((v2&&(b[2]&1))?13:0) || n>maxFrame) return -1;
    uint32_t id=v2?(uint32_t(b[7])|(uint32_t(b[8])<<8)|(uint32_t(b[9])<<16)):b[5];
    if(!b[v2?5:3] || !b[v2?6:4]) return -1;
    for(unsigned i=0;i<count;++i) if(table[i].id==id) {
        if((!v2&&b[1]!=table[i].v1Length) || (v2&&(!b[1]||b[1]>table[i].maxLength))) return -1;
        uint16_t crc=0xffff;
        for(size_t j=1;j<body;++j) crc=crcByte(crc,b[j]);
        crc=crcByte(crc,table[i].extra);
        return crc==(uint16_t(b[body])|(uint16_t(b[body+1])<<8))?int(i):-1;
    }
    return -1;
}
struct Frame { uint8_t data[maxFrame]={}; uint16_t size=0; };
struct Parser {
    bool flight=false;
    uint8_t bytes[280]={}; size_t used=0; uint32_t last=0;
    // Rescan after an invalid frame; bounded storage also handles unsupported full-size frames.
    bool feed(uint8_t b,uint32_t now,Frame &out) {
        if(used && uint32_t(now-last)>250) used=0;
        last=now;
        if(used==sizeof(bytes)) used=0;
        bytes[used++]=b;
        while(used) {
            if(bytes[0]!=0xfe && bytes[0]!=0xfd) { discard(); continue; }
            if(used<3) return false;
            bool v2=bytes[0]==0xfd;
            if(v2&&(bytes[2]&~1)) {discard();continue;}
            size_t length=bytes[1]+(v2?12:8)+((v2&&(bytes[2]&1))?13:0);
            if(used<length) return false;
            if(validate(bytes,length,flight)>=0) {
                out.size=uint16_t(length); memcpy(out.data,bytes,length);
                used-=length; memmove(bytes,bytes+length,used); return true;
            }
            discard();
        }
        return false;
    }
    void discard(){--used;memmove(bytes,bytes+1,used);}
};
struct Recent {
    struct Entry { Frame frame; uint32_t at=0; } entries[16]; unsigned next=0;
    bool contains(const Frame &f,uint32_t now) const {
        for(const auto &e:entries) if(e.frame.size==f.size && f.size && uint32_t(now-e.at)<30000 &&
            memcmp(e.frame.data,f.data,f.size)==0) return true;
        return false;
    }
    void add(const Frame &f,uint32_t now) {entries[next].frame=f;entries[next].at=now;next=(next+1)%16;}
};
inline bool decode(const uint8_t *p,size_t n,Frame &out) {
    if(n<4 || n>maxFrame+4 || memcmp(p,"SMV1",4)!=0 || validate(p+4,n-4)<0) return false;
    out.size=uint16_t(n-4); memcpy(out.data,p+4,out.size);return true;
}
}
