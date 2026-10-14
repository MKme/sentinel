#include "../overlay/src/modules/optional/SentinelPresence/SentinelMavlink.h"
#include "../overlay/src/modules/optional/SentinelPresence/SentinelCore.h"
#include <cassert>
#include <vector>
#include <cstdio>
using namespace sentinelMav;
// Independent bitwise CRC reference (no final XOR for MAVLink).
uint16_t reference(const std::vector<uint8_t> &v,uint8_t extra) {
    uint16_t crc=65535;
    auto add=[&](uint8_t b){crc^=b;for(int i=0;i<8;++i) crc=(crc>>1)^((crc&1)?0x8408:0);};
    for(size_t i=1;i<v.size();++i) add(v[i]);
    add(extra);return crc;
}
std::vector<uint8_t> frame(unsigned type,bool v2,bool signedFrame=false,bool truncated=false) {
    const auto &t=types[type];uint8_t length=truncated?1:(v2?t.maxLength:t.v1Length);
    std::vector<uint8_t> f=v2?std::vector<uint8_t>{0xfd,length,uint8_t(signedFrame),0,77,1,1,uint8_t(t.id),0,0}:
        std::vector<uint8_t>{0xfe,length,77,1,1,uint8_t(t.id)};
    for(unsigned i=0;i<length;++i) f.push_back(uint8_t(i+1));
    auto crc=reference(f,t.extra);f.push_back(crc&255);f.push_back(crc>>8);
    if(signedFrame) for(int i=0;i<13;++i) f.push_back(uint8_t(i));
    return f;
}
int main() {
    for(unsigned type=0;type<typeCount;++type) for(int mode=0;mode<4;++mode) {
        auto f=frame(type,mode!=0,mode==2,mode==3);
        assert(validate(f.data(),f.size())==int(type));
        Parser parser;Frame out;unsigned outputs=0;
        parser.feed(0x55,0,out);parser.feed(0x00,0,out);
        for(auto b:f) if(parser.feed(b,10,out)) ++outputs;
        assert(outputs==1 && out.size==f.size() && !memcmp(out.data,f.data(),f.size()));
        std::vector<uint8_t> mesh={'S','M','V','1'};mesh.insert(mesh.end(),f.begin(),f.end());
        Frame remote;assert(decode(mesh.data(),mesh.size(),remote));
        assert(remote.size==out.size && !memcmp(remote.data,out.data,out.size));
        for(size_t n=0;n<f.size();++n) assert(validate(f.data(),n)<0);
        auto bad=f;bad[mode?10:6]^=1;assert(validate(bad.data(),bad.size())<0);
        mesh[0]='X';assert(!decode(mesh.data(),mesh.size(),remote));
        Recent recent;assert(!recent.contains(out,0));recent.add(out,UINT32_MAX-10);
        assert(recent.contains(out,5));assert(!recent.contains(out,30000));
        // Timeout drops partial input, then complete frame recovers.
        Parser timeout;timeout.feed(f[0],0,out);timeout.feed(f[1],1,out);outputs=0;
        for(auto b:f) if(timeout.feed(b,500,out)) ++outputs;
        assert(outputs==1);
    }
    auto unknown=frame(0,true);unknown[7]=76; // COMMAND_LONG, never admitted
    assert(validate(unknown.data(),unknown.size())<0);
    auto flags=frame(0,true);flags[2]=2;assert(validate(flags.data(),flags.size())<0);
    auto bad=frame(0,false);bad.back()^=0xff;
    Parser recovery;Frame out;for(auto b:bad) recovery.feed(b,1,out);
    auto good=frame(0,false);bool got=false;for(auto b:good) got|=recovery.feed(b,500,out);assert(got);
    Parser fuzz;uint32_t x=1234567;
    for(unsigned i=0;i<100000;++i) {x=x*1664525+1013904223;fuzz.feed(x>>24,i/100,out);assert(fuzz.used<=280);}
    SentinelSettings legacy{};strcpy(legacy.label,"OLD-NODE");legacy.motionGpio=4;
    SentinelSettings migrated{};static_cast<SentinelSettingsV3&>(migrated)=legacy;
    assert(migrated.motionGpio==4 && !strcmp(migrated.label,"OLD-NODE"));
    assert(!migrated.mavSerialToMesh && !migrated.mavMeshToSerial && migrated.mavBaud==57600);
    assert(validMavBaud(57600) && !validMavBaud(0) && !validMavBaud(1000000));
    puts("PASS: MAVLink v1/v2/signed/truncated telemetry, independent CRC, mesh roundtrip, invalid/command rejection, timeout, dedup rollover, bounded fuzz, migration defaults");
    return 0;
}
