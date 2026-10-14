"""Exercise the actual firmware bridge methods with bounded UART and mesh fakes."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
source=(root/'overlay/src/modules/optional/SentinelPresence/SentinelPresence.cpp').read_text()
methods=source[source.index('    HardwareSerial mavUart'):source.index('    void initialize()')]
prefix=r'''
#define main parserTestsMain
#include "mavlink_test.cpp"
#undef main
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

#include "../overlay/src/modules/optional/SentinelPresence/SentinelPosition.h"
#include "../overlay/src/modules/optional/SentinelPresence/SentinelFlight.h"
#include <deque>
#include <algorithm>
uint32_t clockMs=0;
uint32_t millis(){return clockMs;}
constexpr int SERIAL_8N1=0, meshtastic_PortNum_PRIVATE_APP=256,meshtastic_PortNum_TEXT_MESSAGE_APP=1,RTCQualityDevice=1;
uint32_t getValidTime(int){return 1791972000;}
constexpr int meshtastic_Channel_Role_DISABLED=0,meshtastic_Channel_Role_SECONDARY=2;
constexpr int meshtastic_Config_LoRaConfig_RegionCode_UNSET=0,RX_SRC_LOCAL=0;
struct Log {template<class... A> void printf(const char*,A...) {}} Serial;
struct HardwareSerial {
 std::deque<uint8_t> input;std::vector<uint8_t> output;
 explicit HardwareSerial(int n){assert(n==2);}
 void setRxBufferSize(int n){assert(n==2048);}
 void begin(unsigned baud,int,int rx,int tx){assert(baud==57600&&rx==44&&tx==43);}
 size_t available(){return input.size();}
 uint8_t read(){auto b=input.front();input.pop_front();return b;}
 size_t availableForWrite(){return 3;}
 size_t write(const uint8_t*p,size_t n){assert(n<=3);output.insert(output.end(),p,p+n);return n;}
};
struct meshtastic_MeshPacket {
 uint32_t id=12345678,from=42,to=99;unsigned channel=0,hop_limit=0;bool want_ack=false,via_mqtt=false;
 struct {int portnum=256;struct {uint8_t bytes[237]={};size_t size=0;}payload;}decoded;
};
struct Key {unsigned size=32;uint8_t bytes[32]={7};};
struct Channel {int role=1;bool has_settings=true;struct {Key psk;}settings;};
struct Channels {Channel ch;unsigned getNumChannels(){return 1;}unsigned getPrimaryIndex(){return 0;}Channel&getByIndex(unsigned){return ch;}}channels;
struct {struct {int region=1;bool tx_enabled=true;}lora;}config;
struct Node {unsigned getNodeNum(){return 99;}}node;Node*nodeDB=&node;
struct Service {std::vector<meshtastic_MeshPacket> sent;void sendToMesh(meshtastic_MeshPacket*p,int,bool){sent.push_back(*p);delete p;}}mesh;Service*service=&mesh;
enum class ProcessMessage {CONTINUE};
struct Base {virtual bool wantPacket(const meshtastic_MeshPacket*)=0;virtual ProcessMessage handleReceived(const meshtastic_MeshPacket&)=0;meshtastic_MeshPacket*allocDataPacket(){return new meshtastic_MeshPacket;}};
SentinelSettings settings{};
class Bridge:public Base {public: bool initialized=true,portalActive=false;
'''
tests=r'''
};
meshtastic_MeshPacket packet(const std::vector<uint8_t>&f) {
 meshtastic_MeshPacket p;p.decoded.payload.size=f.size()+4;
 memcpy(p.decoded.payload.bytes,"SMV1",4);memcpy(p.decoded.payload.bytes+4,f.data(),f.size());return p;
}
int main(int argc,char **argv){
 if(argc==3) {
   settings.flightEnabled=true;settings.flightPeer=42;clockMs=10000;
   Bridge air;air.pollMavlink(clockMs);meshtastic_MeshPacket p;
   const char *hex=argv[2];size_t n=strlen(hex)/2;assert(n<=sizeof(p.decoded.payload.bytes));
   for(size_t i=0;i<n;++i){unsigned b=0;assert(sscanf(hex+2*i,"%2x",&b)==1);p.decoded.payload.bytes[i]=b;}
   p.decoded.payload.size=n;
   if(!strcmp(argv[1],"up")) {
     air.handleReceived(p);
     for(unsigned i=0;i<80;++i)air.pollMavlink(clockMs);
     for(auto b:air.mavUart.output)printf("%02x",b);
   } else {
     for(size_t i=0;i<n;++i)air.mavUart.input.push_back(p.decoded.payload.bytes[i]);
     air.pollMavlink(clockMs);
     for(auto &sent:mesh.sent)for(size_t i=0;i<sent.decoded.payload.size;++i)printf("%02x",sent.decoded.payload.bytes[i]);
   }
   puts("");return 0;
 }

 parserTestsMain();
 Bridge off;off.pollMavlink(10000);assert(!off.mavStarted);
 settings.mavSerialToMesh=true;settings.mavMeshToSerial=true;
 Bridge bridge;clockMs=10000;bridge.pollMavlink(clockMs);assert(bridge.mavStarted);
 auto a=frame(0,true),b=frame(1,false);
 for(auto c:a)bridge.mavUart.input.push_back(c);
 for(auto c:b)bridge.mavUart.input.push_back(c);
 bridge.pollMavlink(clockMs);assert(mesh.sent.size()==1);
 assert(mesh.sent[0].decoded.payload.size==a.size()+4);
 assert(!memcmp(mesh.sent[0].decoded.payload.bytes+4,a.data(),a.size()));
 bridge.pollMavlink(10001);assert(mesh.sent.size()==1);
 clockMs=15000;bridge.pollMavlink(clockMs);assert(mesh.sent.size()==2);
 assert(!memcmp(mesh.sent[1].decoded.payload.bytes+4,b.data(),b.size()));
 // Fresh receiver reproduces bytes despite partial UART writes.
 Bridge rx;rx.pollMavlink(clockMs);auto p=packet(a);
 p.channel=1;rx.handleReceived(p);assert(rx.mavWrite==0);p.channel=0;
 p.from=99;rx.handleReceived(p);assert(rx.mavWrite==0);p.from=42;
 settings.mavMeshToSerial=false;rx.handleReceived(p);assert(rx.mavWrite==0);settings.mavMeshToSerial=true;
 channels.ch.settings.psk.size=1;rx.handleReceived(p);assert(rx.mavWrite==0);
 channels.ch.settings.psk.size=32;rx.handleReceived(p);rx.handleReceived(p);assert(rx.mavWrite==1);
 for(int i=0;i<40;++i)rx.pollMavlink(clockMs);
 assert(rx.mavUart.output==a&&rx.mavSerialFrames==1);
 // Echo from attached consumer must not re-enter mesh.
 for(auto c:a)rx.mavUart.input.push_back(c);
 rx.pollMavlink(clockMs);assert(mesh.sent.size()==2);
 // Public channel blocks transmit, independently of sensor opt-in.
 Bridge blocked;blocked.pollMavlink(clockMs);settings.allowPublicChannel=true;
 channels.ch.settings.psk.size=1;
 for(auto c:b)blocked.mavUart.input.push_back(c);
 blocked.pollMavlink(clockMs);assert(mesh.sent.size()==2);
 channels.ch.settings.psk.size=32;config.lora.tx_enabled=false;
 blocked.pollMavlink(clockMs);assert(mesh.sent.size()==2);config.lora.tx_enabled=true;
 // Malformed/private-port noise never reaches UART.
 Bridge malformed;malformed.pollMavlink(clockMs);p.decoded.payload.bytes[4+10]^=1;
 malformed.handleReceived(p);malformed.pollMavlink(clockMs);assert(malformed.mavUart.output.empty());
 // Seven-frame bounded receive queue: reject overflow without overwriting pending bytes.
 Bridge full;full.pollMavlink(clockMs);
 for(unsigned seq=0;seq<8;++seq) {
   auto f=frame(0,true);f[4]=seq;f.resize(19);auto crc=reference(f,50);f.push_back(crc&255);f.push_back(crc>>8);
   full.handleReceived(packet(f));
 }
 assert(full.mavWrite==7 && full.mavDropped==1);
 // Never release an old serial sample after a prolonged radio block.
 Bridge stale;stale.pollMavlink(clockMs);config.lora.tx_enabled=false;
 for(auto c:b)stale.mavUart.input.push_back(c);
 stale.pollMavlink(clockMs);config.lora.tx_enabled=true;
 stale.pollMavlink(clockMs+30000);assert(mesh.sent.size()==2 && stale.mavDropped==1);
 // Conversion works without either raw bridge switch.
 settings=SentinelSettings{};settings.sentinelSource=1;mesh.sent.clear();clockMs=50000;
 Bridge pos;auto gps=location();
 for(unsigned i=0;i<gps.size;++i)pos.mavUart.input.push_back(gps.data[i]);
 pos.pollMavlink(clockMs);pos.pollSentinelPosition(clockMs);
 assert(pos.mavStarted && mesh.sent.size()==1 && mesh.sent[0].decoded.portnum==1);
 assert(!memcmp(mesh.sent[0].decoded.payload.bytes,"X1.11.C.",8));
 assert(pos.mavPosition.owner==99);
 pos.pollSentinelPosition(clockMs+1000);assert(mesh.sent.size()==1);
 pos.pollSentinelPosition(clockMs+15000);assert(mesh.sent.size()==1);
 settings.sentinelSource=2;settings.sentinelPeer=42;Bridge remote;
 std::vector<uint8_t> bytes(gps.data,gps.data+gps.size);auto incoming=packet(bytes);
 incoming.from=43;remote.handleReceived(incoming);assert(!remote.mavPosition.valid);
 incoming.from=42;remote.handleReceived(incoming);assert(remote.mavPosition.owner==42);
 remote.pollMavlink(clockMs);assert(!remote.mavStarted);
 channels.ch.settings.psk.size=1;remote.pollSentinelPosition(clockMs);assert(mesh.sent.size()==1);
 channels.ch.settings.psk.size=32;remote.pollSentinelPosition(clockMs);assert(mesh.sent.size()==2);
 clockMs+=14000;remote.handleReceived(incoming);assert(remote.mavPosition.at==50000);
 remote.pollSentinelPosition(clockMs+1000);assert(mesh.sent.size()==2);
 settings.sentinelSource=0;remote.pollSentinelPosition(clockMs);assert(mesh.sent.size()==2);
 settings=SentinelSettings{};settings.flightEnabled=true;settings.flightPeer=42;clockMs=10000;mesh.sent.clear();
 Bridge air;air.pollMavlink(clockMs);
 std::vector<uint8_t> cmd(10+33,0);cmd[0]=0xfd;cmd[1]=33;cmd[2]=1;cmd[5]=255;cmd[6]=190;cmd[7]=76;
 cmd[10+28]=20;cmd[10+30]=1;cmd[10+31]=1;
 auto crc=reference(cmd,152);cmd.push_back(crc&255);cmd.push_back(crc>>8);cmd.resize(cmd.size()+13,0x55);
 Frame request;request.size=cmd.size();memcpy(request.data,cmd.data(),cmd.size());
 assert(sentinelMav::validate(request.data,request.size,true)>=0); // request is flight-only
 assert(sentinelMav::validate(request.data,request.size)<0);
 meshtastic_MeshPacket flight;flight.decoded.payload.size=sentinelFlight::encode(request,1791972000,flight.decoded.payload.bytes);
 flight.from=43;air.handleReceived(flight);assert(air.flightUart.empty());flight.from=42;
 flight.to=0xffffffff;air.handleReceived(flight);assert(air.flightUart.empty());flight.to=99;
 flight.via_mqtt=true;air.handleReceived(flight);assert(air.flightUart.empty());flight.via_mqtt=false;
 channels.ch.settings.psk.size=1;air.handleReceived(flight);assert(air.flightUart.empty());channels.ch.settings.psk.size=32;
 flight.decoded.payload.size=sentinelFlight::encode(request,1791971990,flight.decoded.payload.bytes);
 air.handleReceived(flight);assert(air.flightUart.empty());
 flight.decoded.payload.size=sentinelFlight::encode(request,1791972000,flight.decoded.payload.bytes);
 air.handleReceived(flight);air.handleReceived(flight);assert(air.flightUart.tail==1);
 for(unsigned i=0;i<80;++i)air.pollMavlink(clockMs);
 assert(air.mavUart.output==cmd);
 // Supported controller telemetry returns as paired unicast SMF1.
 auto hb=frame(0,true);for(auto c:hb)air.mavUart.input.push_back(c);air.pollMavlink(clockMs);
 assert(mesh.sent.size()==1 && mesh.sent[0].to==42 && mesh.sent[0].hop_limit==0 && !mesh.sent[0].want_ack);
 assert(!memcmp(mesh.sent[0].decoded.payload.bytes,"SMF1",4));
 puts("PASS: flight paired sender/destination/channel, MQTT rejection, expiry, duplicate suppression, byte-exact UART and downlink");
 puts("PASS: serial/mesh position conversion, selected peer, private output, rate, stale/duplicate suppression, disabled source");
 puts("PASS: actual bridge methods, directions, UART pins/baud, rate scheduling, private channel/radio gates, partial writes, duplicate and echo suppression");
}
'''
out=root/'.build/bridge-integration';out.mkdir(parents=True,exist_ok=True)
cpp=out/'bridge.cpp';cpp.write_text(prefix+methods+tests)
exe=out/'bridge.exe'
subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-I'+str(root/'tests'),str(cpp),'-o',str(exe)],check=True)
subprocess.run([str(exe)],check=True)
