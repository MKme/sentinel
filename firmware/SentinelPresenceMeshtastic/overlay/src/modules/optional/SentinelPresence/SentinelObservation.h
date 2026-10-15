// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "SentinelPosition.h"

namespace sentinelObservation {
// SOI1: magic, LE id:u32, confidence:u16 (0..10000), count:u16,
// ageMs:u16 (0..5000), labelLength:u8, upper-case ASCII label (1..24).
struct Input { uint32_t id=0; uint16_t confidence=0,count=0,ageMs=0; char label[25]={}; };
inline bool decode(const uint8_t *b,size_t n,Input &v) {
    if(n<16 || n>39 || memcmp(b,"SOI1",4)!=0 || b[14]<1 || b[14]>24 || n!=size_t(15+b[14]))return false;
    auto u16=[&](unsigned i){return uint16_t(b[i])|uint16_t(b[i+1])<<8;};
    v.id=uint32_t(u16(4))|uint32_t(u16(6))<<16;
    v.confidence=u16(8);v.count=u16(10);v.ageMs=u16(12);
    if(!v.id || v.confidence>10000 || !v.count || v.count>32767 || v.ageMs>5000)return false;
    for(unsigned i=0;i<b[14];++i) {
        char c=b[15+i];if(!((c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return false;
        v.label[i]=c;
    }
    v.label[b[14]]=0;return true;
}
struct Report { Input input; uint32_t at=0,utc=0; int32_t lat=INT32_MIN,lon=INT32_MIN; uint32_t fixAge=0; uint8_t location=0; };
inline Report capture(const Input &input,const sentinelPosition::Position &p,uint32_t now,uint32_t utc) {
    Report r;r.input=input;r.at=now;
    r.utc=sentinelPosition::epochValid(utc)?utc-input.ageMs/1000:0;
    // Position describes the sensor at reception, not a projected object location.
    if(p.hasPosition && p.fix) {
        r.lat=p.lat;r.lon=p.lon;
        const uint32_t positionAge=uint32_t(now-p.at),fixAge=uint32_t(now-p.fixAt);
        r.fixAge=(positionAge>fixAge?positionAge:fixAge)/1000;
        r.location=uint32_t(now-p.at)<15000 && uint32_t(now-p.fixAt)<15000?1:2;
    }
    return r;
}
struct Queue {
    Report entries[4];unsigned head=0,size=0;
    struct Seen { uint32_t id=0,at=0;char label[25]={}; } seen[16];unsigned next=0;
    uint32_t accepted=0,rejected=0,duplicates=0,dropped=0,sent=0,lastSent=0;
    bool push(const Report &r,uint32_t cooldown) {
        for(const auto &s:seen) if(s.id &&
            ((s.id==r.input.id && uint32_t(r.at-s.at)<60000) ||
             (!strcmp(s.label,r.input.label)&&uint32_t(r.at-s.at)<cooldown))) {++duplicates;return false;}
        if(size==4){++dropped;return false;}
        seen[next].id=r.input.id;seen[next].at=r.at;strcpy(seen[next].label,r.input.label);next=(next+1)%16;
        entries[(head+size)%4]=r;++size;++accepted;return true;
    }
    void pop(){if(size){head=(head+1)%4;--size;}}
    void expire(uint32_t now){while(size && uint32_t(now-entries[head].at)>=30000){pop();++dropped;}}
};
inline size_t encode(const Report &r,uint32_t now,uint32_t node,uint32_t packetId,char *out,size_t capacity) {
    uint8_t b[96]={};b[0]=1;b[1]=6;
    auto be=[&](unsigned i,uint32_t v){for(unsigned j=0;j<4;++j)b[i+j]=uint8_t(v>>(24-8*j));};
    be(2,r.utc/60);be(6,r.location?uint32_t(sentinelPosition::e5(r.lat)):uint32_t(INT32_MIN));
    be(10,r.location?uint32_t(sentinelPosition::e5(r.lon)):uint32_t(INT32_MIN));be(14,node);
    const uint32_t elapsed=uint32_t(now-r.at)/1000;
    const bool stale=r.location==2 || (r.location==1 && r.fixAge+elapsed>=15);
    b[18]=1|4|64|(r.location?0:8)|(stale?16:0)|(r.utc?0:32);
    char label[32];snprintf(label,sizeof(label),"OBS %s",r.input.label);unsigned len=strlen(label),n=20+len;b[19]=len;memcpy(b+20,label,len);
    auto sensor=[&](uint8_t type,uint16_t value){b[n++]=type;b[n++]=value>>8;b[n++]=value;};
    auto cap=[](uint32_t v){return uint16_t(v>32767?32767:v);};
    sensor(240,r.input.confidence);sensor(241,r.input.count);sensor(242,r.location?cap(r.fixAge+elapsed):uint16_t(-1));
    sensor(243,cap(r.input.ageMs/1000+elapsed));sensor(244,r.input.id>>16);sensor(245,r.input.id);
    constexpr char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    char encoded[132]={};unsigned z=0;
    for(unsigned i=0;i<n;i+=3){uint32_t v=uint32_t(b[i])<<16;if(i+1<n)v|=uint32_t(b[i+1])<<8;if(i+2<n)v|=b[i+2];
        encoded[z++]=alphabet[v>>18&63];encoded[z++]=alphabet[v>>12&63];if(i+1<n)encoded[z++]=alphabet[v>>6&63];if(i+2<n)encoded[z++]=alphabet[v&63];}
    int length=snprintf(out,capacity,"X1.11.C.%08lX.1/1.%s",(unsigned long)packetId,encoded);
    return length>0 && size_t(length)<capacity?size_t(length):0;
}
}
