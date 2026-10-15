"""Render the exact firmware portal C++ with explicitly simulated channel state."""
from pathlib import Path
import subprocess

root=Path(__file__).resolve().parents[1]
overlay=root/'overlay/src/modules/optional/SentinelPresence'
source=(overlay/'SentinelPresence.cpp').read_text()
defaults=source[source.index('void loadFactoryDefaults()'):source.index('void loadSettings()')]
page=source[source.index('    String portalPage()'):source.index('    void startPortal()')]
saved=source[source.index('    String savedPage()'):source.index('    void stopPortalAndStartScanner()')]
shim=r'''
#include <iostream>
#include <string>
#include <type_traits>
#include <cstring>
#include "SentinelCore.h"
#include "SentinelPresenceConfig.h"
#define F(x) x
class String: public std::string {
public:
 using std::string::string;
 String()=default;
 String(const std::string &s):std::string(s){}
 template<class T,typename std::enable_if<std::is_integral<T>::value,int>::type=0>
 String(T n):std::string(std::to_string(n)){}
 void replace(const char *from,const char *to){size_t p=0;while((p=find(from,p))!=npos){std::string::replace(p,strlen(from),to);p+=strlen(to);}}
};
void strlcpy(char *to,const char *from,size_t size){std::strncpy(to,from,size-1);to[size-1]=0;}
SentinelSettings settings;
bool observationEnabled=true;uint16_t observationInterval=15;
constexpr int meshtastic_Channel_Role_DISABLED=0;
struct Channel {int role=1;};
struct Channels {
 unsigned getNumChannels(){return 2;}
 Channel getByIndex(unsigned){return {};}
 const char *getName(unsigned i){return i==0?"LongFast":"Field Team";}
} channels;
String checked(bool b){return b?" checked":"";}
'''
out=root/'.build/portal-preview';out.mkdir(parents=True,exist_ok=True)
cpp=out/'portal.cpp'
cpp.write_text(shim+defaults+page+saved+'''\nint main(int argc,char**){loadFactoryDefaults();
 std::strcpy(settings.label,"SENTINEL-04");settings.meshChannelIndex=1;
 settings.meshHeartbeat=true; std::cout<<(argc>1?savedPage():portalPage());}\n''')
subprocess.run(['g++','-std=c++17','-I'+str(overlay),str(cpp),'-o',str(out/'portal.exe')],check=True)
with (out/'index.html').open('wb') as f:subprocess.run([str(out/'portal.exe')],stdout=f,check=True)
with (out/'saved.html').open('wb') as f:subprocess.run([str(out/'portal.exe'),'saved'],stdout=f,check=True)
print(out/'index.html')
