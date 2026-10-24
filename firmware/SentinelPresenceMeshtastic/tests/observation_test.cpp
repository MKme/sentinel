#include "../overlay/src/modules/optional/SentinelPresence/SentinelObservation.h"
#include <cassert>
#include <iostream>
int main() {
    using namespace sentinelObservation;
    uint8_t data[]={ 'S','O','I','1',0x78,0x56,0x34,0x12,0xf0,0x23,1,0,0,0,4,'F','I','R','E' };
    Input input;assert(decode(data,sizeof(data),input));assert(input.confidence==9200 && input.id==0x12345678);
    auto bad=data[8];data[8]=255;data[9]=255;assert(!decode(data,sizeof(data),input));data[8]=bad;data[9]=0x23;
    assert(!decode(data,sizeof(data)-1,input));data[15]='<';assert(!decode(data,sizeof(data),input));data[15]='F';assert(decode(data,sizeof(data),input));
    char out[180];
    auto none=capture(input,10000,0);assert(!none.utc);
    assert(encode(none,10000,0x10203040,1,out,sizeof(out)));std::cout<<out<<'\n';
    auto timed=capture(input,11000,1792062000);
    assert(timed.utc==1792062000);
    assert(encode(timed,11000,0x10203040,2,out,sizeof(out)));std::cout<<out<<'\n';
    Queue q;assert(q.push(none,5000));assert(!q.push(none,5000));assert(q.duplicates==1);
    for(unsigned i=1;i<4;++i){auto r=none;r.input.id+=i;r.input.label[0]='A'+i;assert(q.push(r,5000));}
    auto overflow=none;overflow.input.id=99;strcpy(overflow.input.label,"SMOKE");assert(!q.push(overflow,5000));assert(q.dropped==1);
    q.expire(40000);assert(q.size==0 && q.dropped==5);
    Queue longCooldown;assert(longCooldown.push(none,120000));longCooldown.pop();
    auto repeated=none;repeated.input.id++;repeated.at+=90000;
    assert(!longCooldown.push(repeated,120000));
    repeated.at=none.at+120000;assert(longCooldown.push(repeated,120000));
    // Wraparound expiry remains bounded.
    Queue wrap;auto r=none;r.at=UINT32_MAX-100;assert(wrap.push(r,5000));wrap.expire(30000);assert(!wrap.size);
    assert(!encode(none,10000,1,1,out,8));
}
