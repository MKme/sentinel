#pragma once
#include <cstdint>
#include <cstring>
struct SentinelSettingsV3 {
    char label[25];
    uint8_t motionGpio;
    int8_t rssiMin;
    uint8_t channelFirst;
    uint8_t channelLast;
    uint16_t channelDwellMs;
    uint16_t uniqueTtlSeconds;
    uint16_t wifiCooldownSeconds;
    uint16_t motionCooldownSeconds;
    uint16_t combinedWindowSeconds;
    uint16_t heartbeatSeconds;
    uint16_t portalSeconds;
    uint8_t meshChannelIndex;
    bool serialVerbose;
    bool meshOutput;
    bool allowPublicChannel;
    bool meshHeartbeat;
};

struct SentinelSettings : SentinelSettingsV3 {
    bool mavSerialToMesh=false, mavMeshToSerial=false;
    uint8_t mavChannel=0;
    uint32_t mavBaud=57600;
    uint16_t mavIntervalMs=5000;
};
inline bool validMavBaud(uint32_t n) { return n==9600 || n==19200 || n==38400 || n==57600 || n==115200; }
inline bool validSettings(const SentinelSettings &s) {
    if (!s.label[0] || s.label[24] != 0) return false;
    for (unsigned i=0;s.label[i];++i) {
        char c=s.label[i];
        if (!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c==' ')) return false;
    }
    return (s.motionGpio==3 || s.motionGpio==4) && s.rssiMin>=-100 && s.rssiMin<=-20 &&
        s.channelFirst>=1 && s.channelLast<=11 && s.channelFirst<=s.channelLast &&
        s.channelDwellMs>=100 && s.channelDwellMs<=2000 &&
        s.uniqueTtlSeconds>=10 && s.uniqueTtlSeconds<=3600 &&
        s.wifiCooldownSeconds>=5 && s.wifiCooldownSeconds<=3600 &&
        s.motionCooldownSeconds>=2 && s.motionCooldownSeconds<=3600 &&
        s.combinedWindowSeconds>=1 && s.combinedWindowSeconds<=300 &&
        s.heartbeatSeconds>=10 && s.heartbeatSeconds<=3600 &&
        s.portalSeconds>=15 && s.portalSeconds<=600 && s.meshChannelIndex<8 && s.mavChannel<8 && validMavBaud(s.mavBaud) &&
        s.mavIntervalMs>=1000 && s.mavIntervalMs<=60000;
}
inline bool publicKey(const uint8_t *key, int length) {
    const uint8_t prefix[15]={0xd4,0xf1,0xbb,0x3a,0x20,0x29,0x07,0x59,0xf0,0xbc,0xff,0xab,0xcf,0x4e,0x69};
    return (length>=0 && length<=1) || (length==16 && memcmp(key,prefix,15)==0);
}
inline bool channelAllowed(bool enabled,bool configured,bool tx,bool optIn,const uint8_t *key,int length) {
    if(!enabled || !configured || !tx || (length!=0 && length!=1 && length!=16 && length!=32)) return false;
    return optIn || !publicKey(key,length);
}
struct SourceCache {
    struct Entry { uint32_t hash=0, at=0; bool occupied=false; } entries[64];
    bool observe(uint32_t hash,uint32_t now,uint32_t ttl) {
        unsigned slot=0; uint32_t age=0;
        for(unsigned i=0;i<64;++i) {
            auto &e=entries[i];
            if(e.occupied && e.hash==hash) {
                bool fresh=uint32_t(now-e.at)>=ttl; e.at=now; return fresh;
            }
            if(!e.occupied) {slot=i;age=UINT32_MAX;}
            else if(uint32_t(now-e.at)>age) {slot=i;age=now-e.at;}
        }
        entries[slot]={hash,now,true}; return true;
    }
};
