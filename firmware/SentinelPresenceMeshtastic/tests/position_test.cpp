#define main parserTestsMain
#include "mavlink_test.cpp"
#undef main
#include "../overlay/src/modules/optional/SentinelPresence/SentinelPosition.h"
void put(std::vector<uint8_t>& f,unsigned at,uint64_t value,unsigned bytes) {
    for(unsigned i=0;i<bytes;++i)f[10+at+i]=uint8_t(value>>(8*i));
}
Frame location(bool gps=true,uint8_t fix=3,uint64_t time=1791972000000000ULL,int32_t lat=-338651430,int32_t lon=1512099000) {
    unsigned type=gps?2:4;auto f=frame(type,true);
    if(gps) {put(f,0,time,8);put(f,28,fix,1);}
    put(f,gps?8:4,uint32_t(lat),4);put(f,gps?12:8,uint32_t(lon),4);
    f.resize(10+f[1]);auto crc=reference(f,types[type].extra);f.push_back(crc&255);f.push_back(crc>>8);
    Frame result;result.size=f.size();memcpy(result.data,f.data(),f.size());return result;
}
int main() {
    using namespace sentinelPosition;
    Position p;auto gps=location(),global=location(false);
    assert(!p.observe(global,0x10203040,1,1,100,0));
    assert(!p.observe(gps,0x10203040,2,1,100,0));
    assert(!p.observe(gps,0x10203040,1,2,100,0));
    assert(p.observe(gps,0x10203040,1,1,100,0));
    assert(p.fresh(101) && !p.fresh(15100));
    assert(!p.observe(global,0x9999,1,1,200,0));
    assert(p.observe(global,0x10203040,1,1,1100,0) && p.epoch==1791972001);
    char output[120];assert(wrapper(p,"WRAITH-01",0xA1B2C3D4,output,sizeof(output)));
    puts(output); // Consumed by the actual XTOC decoder contract test.
    char small[8];assert(!wrapper(p,"WRAITH-01",1,small,sizeof(small)));
    assert(!p.observe(location(true,2),0x10203040,1,1,2000,0) && !p.fresh(2000));
    assert(!p.observe(global,0x10203040,1,1,2100,0));
    assert(!p.observe(location(true,3,1000000),0x10203040,1,1,2200,0)); // Boot-relative time is not UTC.
    assert(p.observe(location(true,3,1000000),0x10203040,1,1,2300,1791972000));
    assert(!p.observe(location(true,3,1791972000000000ULL,900000001),0x10203040,1,1,2400,0));
    assert(!p.fresh(2400));
    assert(p.observe(location(true,3,1791972000000000ULL,0,0),0x10203040,1,1,UINT32_MAX-100,0));
    assert(p.fresh(100)); // Legitimate 0/0 with an actual fix is accepted.
    assert(e5(-150)==-1 && e5(-151)==-2 && e5(150)==2);
    uint32_t node=0;assert(parseNode("10203040",node)&&node==0x10203040);
    assert(!parseNode("!10203040",node)&&!parseNode("not-hex!",node));
    puts("PASS: position extraction, selected identity, 3D fix loss, stale/unknown-time suppression, UTC fallback, coordinate bounds, rollover, Sentinel encoding");
    return 0;
}
