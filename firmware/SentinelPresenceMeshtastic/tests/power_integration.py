"""Compile and exercise the actual optional driver with deterministic ADC/radio fixtures."""
from pathlib import Path
import runpy,subprocess
root=Path(__file__).resolve().parents[1]
base=runpy.run_path(str(root/'tests/ground_integration.py'))
source=(root/'overlay/src/modules/optional/SentinelPresence/SentinelPresence.cpp').read_text()
driver=source[source.index('    // Optional cooperative power sensing'):source.index('    void pollMotion')]
prefix=base['prefix'].replace('class Bridge:public Base', '''
#include "../overlay/src/modules/optional/SentinelPresence/XtocPowerPacket.h"
#define SENTINEL_POWER_ENABLED 1
#define SENTINEL_POWER_AMPS_PER_ADC_VOLT 10.0
#define SENTINEL_POWER_VOLTS_PER_ADC_VOLT 10.0
#define SENTINEL_POWER_MEASURE_VOLTAGE true
#define SENTINEL_POWER_DC true
#define SENTINEL_POWER_CURRENT_ZERO_V 1.0
#define SENTINEL_POWER_NOMINAL_AC_VOLTS 0
#define SENTINEL_POWER_CHANNEL 2
#define SENTINEL_POWER_CURRENT_GPIO 1
#define SENTINEL_POWER_VOLTAGE_GPIO 2
#define SENTINEL_POWER_INTERVAL_MS 60000UL
constexpr int ADC_11db=1;
bool clipped=false;
void analogReadResolution(int){}void analogSetPinAttenuation(int,int){}
int analogRead(int){return clipped?4095:2048;}
int analogReadMilliVolts(int pin){return pin==1?1500:2400;}
struct Pool {void release(meshtastic_MeshPacket *p){delete p;}}packetPool;
class Bridge:public Base''')
tests='''
};
int main(){Bridge b;settings.meshOutput=true;
 for(unsigned i=1000;i<=1200;++i)b.pollPower(i);
 assert(mesh.sent.size()==1);auto &p=mesh.sent[0];
 assert(p.decoded.portnum==1 && p.channel==0 && !p.want_ack);
 assert(!memcmp(p.decoded.payload.bytes,"X1.11.C.",8));
 for(unsigned i=1201;i<61000;++i)b.pollPower(i);
 assert(mesh.sent.size()==1);
 clipped=true;for(unsigned i=61000;i<=61200;++i)b.pollPower(i);assert(mesh.sent.size()==1);
 clipped=false;channels.ch.settings.psk.size=1;
 for(unsigned i=121000;i<=121200;++i)b.pollPower(i);assert(mesh.sent.size()==1);
 channels.ch.settings.psk.size=32;config.lora.tx_enabled=false;
 for(unsigned i=181000;i<=181200;++i)b.pollPower(i);assert(mesh.sent.size()==1);
 config.lora.tx_enabled=true;settings.meshOutput=false;
 for(unsigned i=241000;i<=241200;++i)b.pollPower(i);assert(mesh.sent.size()==1);
 puts("PASS optional actual power driver: calibrated DC packet, cooperative window, rate, clipping, private channel and TX/mesh output gates");}
'''
out=root/'.build/power-integration';out.mkdir(parents=True,exist_ok=True)
cpp=out/'power.cpp';cpp.write_text(prefix+base['methods']+driver+tests)
exe=out/'power.exe'
subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-I'+str(root/'tests'),str(cpp),'-o',str(exe)],check=True)
subprocess.run([str(exe)],check=True)
