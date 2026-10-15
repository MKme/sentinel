#include "../overlay/src/modules/optional/SentinelPresence/SentinelObservation.h"
#include <cassert>
#include <iostream>
int main() {
    using namespace sentinelObservation;
    uint8_t data[]={ 'S','O','I','1',0x78,0x56,0x34,0x12,0xf0,0x23,1,0,0,0,4,'F','I','R','E' };
    Input input;assert(decode(data,sizeof(data),input));assert(input.confidence==9200 && input.id==0x12345678);
    auto bad=data[8];data[8]=255;data[9]=255;assert(!decode(data,sizeof(data),input));data[8]=bad;data[9]=0x23;
    assert(!decode(data,sizeof(data)-1,input));data[15]='<';assert(!decode(data,sizeof(data),input));data[15]='F';assert(decode(data,sizeof(data),input));
    sentinelPosition::Position p;char out[180];
    auto none=capture(input,p,10000,0);assert(none.location==0 && !none.utc);
    assert(encode(none,10000,0x10203040,1,out,sizeof(out)));std::cout<<out<<'\n';
    p.hasPosition=true;p.fix=true;p.lat=450000000;p.lon=-800000000;p.at=p.fixAt=10000;
    auto fresh=capture(input,p,11000,1792062000);assert(fresh.location==1);
    assert(encode(fresh,11000,0x10203040,2,out,sizeof(out)));std::cout<<out<<'\n';
    auto stale=capture(input,p,27000,1792062016);assert(stale.location==2);
    assert(encode(stale,27000,0x10203040,3,out,sizeof(out)));std::cout<<out<<'\n';
    // A fresh snapshot that waits in the queue becomes stale at transmission.
    assert(encode(fresh,28000,0x10203040,4,out,sizeof(out)));std::cout<<out<<'\n';
    p.fix=false;assert(capture(input,p,28000,1792062017).location==0);
    Queue q;assert(q.push(none,5000));assert(!q.push(none,5000));assert(q.duplicates==1);
    for(unsigned i=1;i<4;++i){auto r=none;r.input.id+=i;r.input.label[0]='A'+i;assert(q.push(r,5000));}
    auto overflow=none;overflow.input.id=99;strcpy(overflow.input.label,"SMOKE");assert(!q.push(overflow,5000));assert(q.dropped==1);
    q.expire(40000);assert(q.size==0 && q.dropped==5);
    Queue longCooldown;assert(longCooldown.push(none,120000));longCooldown.pop();
    auto repeated=none;repeated.input.id++;repeated.at+=90000;
    assert(!longCooldown.push(repeated,120000));
    repeated.at=none.at+120000;assert(longCooldown.push(repeated,120000));
    p.fix=true;p.at=27000;p.fixAt=10000;
    auto oldFix=capture(input,p,28000,1792062017);
    assert(oldFix.location==2 && oldFix.fixAge==18);
    // Wraparound expiry remains bounded.
    Queue wrap;auto r=none;r.at=UINT32_MAX-100;assert(wrap.push(r,5000));wrap.expire(30000);assert(!wrap.size);
    assert(!encode(none,10000,1,1,out,8));
}
