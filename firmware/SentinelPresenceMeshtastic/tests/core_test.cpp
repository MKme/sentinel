#include "../overlay/src/modules/optional/SentinelPresence/SentinelCore.h"
#include <cassert>
#include <cstdio>

int main() {
    SentinelSettings s={};
    std::strcpy(s.label,"SENTINEL-1");
    s.motionGpio=3;s.rssiMin=-82;s.channelFirst=1;s.channelLast=11;
    s.channelDwellMs=250;s.uniqueTtlSeconds=120;s.wifiCooldownSeconds=30;
    s.motionCooldownSeconds=10;s.combinedWindowSeconds=10;
    s.heartbeatSeconds=60;s.portalSeconds=120;
    assert(validSettings(s));
    SentinelSettingsV4 old{};memset(&old,0xff,sizeof(old));static_cast<SentinelSettingsV3&>(old)=s;
    SentinelSettings migrated{};static_cast<SentinelSettingsV4&>(migrated)=old;
    assert(migrated.sentinelSource==0 && migrated.sentinelSystem==1 && migrated.sentinelComponent==1 && migrated.sentinelIntervalSeconds==15);
    auto position=s;position.sentinelSource=2;assert(!validSettings(position));
    position.sentinelPeer=42;assert(validSettings(position));
    position.sentinelSystem=0;assert(!validSettings(position));position.sentinelSystem=1;
    position.sentinelIntervalSeconds=4;assert(!validSettings(position));
    position.sentinelIntervalSeconds=3601;assert(!validSettings(position));
    position.sentinelIntervalSeconds=15;position.sentinelSource=3;assert(!validSettings(position));
    for(unsigned pin=0;pin<49;++pin) {
        auto v=s;v.motionGpio=pin;
        assert(validSettings(v)==(pin==3||pin==4));
    }
    auto v=s;v.label[24]='x';assert(!validSettings(v));
    v=s;std::strcpy(v.label,"<script>");assert(!validSettings(v));
    v=s;v.channelFirst=12;assert(!validSettings(v));
    v=s;v.channelLast=13;assert(!validSettings(v));
    v=s;v.portalSeconds=0;assert(!validSettings(v));
    v=s;v.meshChannelIndex=8;assert(!validSettings(v));
    v=s;v.mavChannel=8;assert(!validSettings(v));
    v=s;v.mavBaud=12345;assert(!validSettings(v));
    v=s;v.mavIntervalMs=999;assert(!validSettings(v));
    v=s;v.mavIntervalMs=60001;assert(!validSettings(v));
    v=s;v.mavIntervalMs=1000;v.mavSerialToMesh=true;v.mavMeshToSerial=true;assert(validSettings(v));
    v=s;v.uniqueTtlSeconds=0;assert(!validSettings(v));
    v=s;v.rssiMin=0;assert(!validSettings(v));
    SourceCache cache;
    assert(cache.observe(0,10,100));
    assert(!cache.observe(0,20,100));
    assert(cache.observe(0,120,100));
    assert(cache.observe(1,130,100));
    assert(!cache.observe(0,140,100));
    SourceCache wrap;
    assert(wrap.observe(42,UINT32_MAX-20,100));
    assert(!wrap.observe(42,10,100));
    assert(wrap.observe(42,111,100));
    for(unsigned i=0;i<128;++i) cache.observe(i,1000+i,100);
    assert(!cache.observe(127,1130,100));
    uint8_t key[32]={0xd4,0xf1,0xbb,0x3a,0x20,0x29,0x07,0x59,0xf0,0xbc,0xff,0xab,0xcf,0x4e,0x69,1};
    for(unsigned i=0;i<256;++i) {key[15]=i;assert(publicKey(key,16));}
    assert(publicKey(key,0));
    key[0]^=1; assert(!publicKey(key,16));
    assert(!publicKey(key,32));
    assert(channelAllowed(true,true,true,false,key,32));
    assert(!channelAllowed(false,true,true,true,key,32));
    assert(!channelAllowed(true,false,true,true,key,32));
    assert(!channelAllowed(true,true,false,true,key,32));
    assert(!channelAllowed(true,true,true,true,key,-1));
    assert(!channelAllowed(true,true,true,true,key,15));
    assert(!channelAllowed(true,true,true,false,key,1));
    assert(channelAllowed(true,true,true,true,key,1));
    assert(!channelAllowed(true,true,true,false,key,0));
    std::puts("PASS: settings/pin boundaries, label safety, deduplication, rollover, cache eviction, public-key family");
}
