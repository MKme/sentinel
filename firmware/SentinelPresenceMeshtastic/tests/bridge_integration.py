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
#include <deque>
#include <algorithm>
uint32_t clockMs=0;
uint32_t millis(){return clockMs;}
constexpr int SERIAL_8N1=0, meshtastic_PortNum_PRIVATE_APP=256;
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
 uint32_t from=42;unsigned channel=0;bool want_ack=false;
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
class Bridge:public Base {public:
'''
tests=r'''
};
meshtastic_MeshPacket packet(const std::vector<uint8_t>&f) {
 meshtastic_MeshPacket p;p.decoded.payload.size=f.size()+4;
 memcpy(p.decoded.payload.bytes,"SMV1",4);memcpy(p.decoded.payload.bytes+4,f.data(),f.size());return p;
}
int main(){
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
 puts("PASS: actual bridge methods, directions, UART pins/baud, rate scheduling, private channel/radio gates, partial writes, duplicate and echo suppression");
}
'''
out=root/'.build/bridge-integration';out.mkdir(parents=True,exist_ok=True)
cpp=out/'bridge.cpp';cpp.write_text(prefix+methods+tests)
exe=out/'bridge.exe'
subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-I'+str(root/'tests'),str(cpp),'-o',str(exe)],check=True)
subprocess.run([str(exe)],check=True)
