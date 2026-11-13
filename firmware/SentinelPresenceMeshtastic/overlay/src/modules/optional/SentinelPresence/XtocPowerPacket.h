// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MKME. Permission is hereby granted, free of charge, to any
// person obtaining a copy of this software and associated documentation files
// (the "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

namespace xtocPower {
// NaN means unavailable; zero is a real measured zero. SI quantities throughout.
struct Values {
    uint8_t channel=0;
    double currentA=NAN,voltageV=NAN,activeW=NAN,apparentVA=NAN,powerFactor=NAN;
    double batteryPct=NAN,energyWh=NAN,inputW=NAN,outputW=NAN;
    bool dc=false,assumedVoltage=false;
};
inline bool validUtc(uint32_t utc) {return utc>=1577836800U && utc<4102444800U;}
// Returns zero on invalid data or insufficient capacity. No allocation or third-party library.
inline size_t encode(const Values &v,uint32_t node,uint32_t utc,const char *label,const char *packetId,char *out,size_t capacity) {
    if(!out || !capacity)return 0;
    out[0]=0;
    if(!label || !packetId)return 0;
    const size_t labelLength=strlen(label),idLength=strlen(packetId);
    if(labelLength>24 || !idLength || idLength>48)return 0;
    for(size_t i=0;i<labelLength;++i)if(uint8_t(label[i])<32 || uint8_t(label[i])>126)return 0;
    for(size_t i=0;i<idLength;++i)if(!((packetId[i]>='0'&&packetId[i]<='9')||(packetId[i]>='A'&&packetId[i]<='Z')||(packetId[i]>='a'&&packetId[i]<='z')||packetId[i]=='_'||packetId[i]=='-'))return 0;
    if(v.assumedVoltage && (!isnan(v.activeW)||!isnan(v.inputW)||!isnan(v.outputW)))return 0;
    uint8_t b[100]={};unsigned n=19,count=0;bool valid=true,measured=false;
    auto be=[&](unsigned i,uint32_t value){for(unsigned j=0;j<4;++j)b[i+j]=uint8_t(value>>(24-8*j));};
    b[0]=1;be(2,validUtc(utc)?utc/60:0);be(6,0x80000000U);be(10,0x80000000U);be(14,node);
    b[18]=8|(labelLength?4:0)|(validUtc(utc)?0:32);
    if(labelLength){b[n++]=uint8_t(labelLength);memcpy(b+n,label,labelLength);n+=labelLength;}
    auto raw=[&](uint8_t type,int value){b[n++]=type;b[n++]=uint16_t(value)>>8;b[n++]=uint16_t(value);++count;};
    auto add=[&](uint8_t type,double value,double scale,int minimum,int maximum){
        if(isnan(value))return;
        if(!isfinite(value)||value*scale<minimum||value*scale>maximum){valid=false;return;}
        raw(type,int(floor(value*scale+.5)));measured=true;
    };
    auto watts=[&](uint8_t fine,uint8_t coarse,double value,bool sign){
        if(isnan(value))return;
        if(!isfinite(value)||(!sign&&value<0)){valid=false;return;}
        if(floor(value+.5)>32767 || floor(value+.5)<-32768)add(coarse,value,.1,sign?-32768:0,32767);
        else add(fine,value,1,sign?-32768:0,32767);
    };
    raw(80,1);raw(81,v.channel);
    add(82,v.currentA,100,-32768,32767);add(83,v.voltageV,100,0,32767);
    watts(84,85,v.activeW,true);add(86,v.apparentVA,1,0,32767);add(87,v.powerFactor,1000,0,1000);
    add(88,v.batteryPct,100,0,10000);add(89,v.energyWh,.1,0,32767);
    watts(92,94,v.inputW,false);watts(93,95,v.outputW,false);
    if(validUtc(utc))raw(90,utc%60);
    raw(91,(v.dc?1:0)|(v.assumedVoltage?2:0));b[1]=uint8_t(count);
    if(!valid||!measured)return 0;
    constexpr char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    char encoded[136]={};unsigned z=0;
    for(unsigned i=0;i<n;i+=3){uint32_t x=uint32_t(b[i])<<16;if(i+1<n)x|=uint32_t(b[i+1])<<8;if(i+2<n)x|=b[i+2];
        encoded[z++]=alphabet[x>>18&63];encoded[z++]=alphabet[x>>12&63];if(i+1<n)encoded[z++]=alphabet[x>>6&63];if(i+2<n)encoded[z++]=alphabet[x&63];}
    int length=snprintf(out,capacity,"X1.11.C.%s.1/1.%s",packetId,encoded);
    if(length<=0 || size_t(length)>=capacity){out[0]=0;return 0;}return size_t(length);
}
// Incremental ADC-window statistics, suitable for a cooperative firmware loop.
struct Window {
    unsigned count=0;double sum=0,squares=0;bool clipped=false;
    void add(double volts,bool rail=false){if(!isfinite(volts)){clipped=true;return;}++count;sum+=volts;squares+=volts*volts;clipped|=rail;}
    double mean()const{return count&&!clipped?sum/count:NAN;}
    double acRms()const{double m=mean();return count>=100&&!clipped?sqrt(fmax(0,squares/count-m*m)):NAN;}
};
}
