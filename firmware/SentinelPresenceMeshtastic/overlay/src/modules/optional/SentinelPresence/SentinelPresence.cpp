// SPDX-License-Identifier: GPL-3.0-or-later
#include "SentinelPresence.h"
#include "configuration.h"

#if defined(ARCH_ESP32) && defined(SEEED_XIAO_S3)

#include "SentinelPresenceConfig.h"
#include "SentinelCore.h"
#include "SentinelMavlink.h"
#include "Channels.h"
#include "MeshService.h"
#include "NodeDB.h"
#include "SinglePortModule.h"
#include "configuration.h"
#include "concurrency/OSThread.h"
#include "main.h"

#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_event.h>
#include <esp_wifi.h>

namespace {

constexpr uint8_t EVENT_QUEUE_SIZE = 32;


struct WifiEvent {
    uint32_t atMs;
    uint32_t sourceHash;
    int8_t rssi;
    uint8_t channel;
    uint8_t frameType;
    uint8_t frameSubtype;
};

volatile WifiEvent eventQueue[EVENT_QUEUE_SIZE];
volatile uint8_t eventWrite = 0;
volatile uint8_t eventRead = 0;
volatile uint32_t wifiPacketsSeen = 0;
volatile uint32_t queueDrops = 0;
portMUX_TYPE eventMux = portMUX_INITIALIZER_UNLOCKED;

SentinelSettings settings = {};
WebServer portalServer(80);
DNSServer portalDns;

uint32_t clampU32(long value, uint32_t low, uint32_t high)
{
    if (value < static_cast<long>(low))
        return low;
    if (value > static_cast<long>(high))
        return high;
    return static_cast<uint32_t>(value);
}

void loadFactoryDefaults()
{
    settings = SentinelSettings{};
    strlcpy(settings.label, SENTINEL_DEFAULT_NODE_LABEL, sizeof(settings.label));
    settings.motionGpio = SENTINEL_DEFAULT_MOTION_GPIO;
    settings.rssiMin = SENTINEL_DEFAULT_WIFI_RSSI_MIN_DBM;
    settings.channelFirst = SENTINEL_DEFAULT_WIFI_CHANNEL_FIRST;
    settings.channelLast = SENTINEL_DEFAULT_WIFI_CHANNEL_LAST;
    settings.channelDwellMs = SENTINEL_DEFAULT_WIFI_CHANNEL_DWELL_MS;
    settings.uniqueTtlSeconds = SENTINEL_DEFAULT_WIFI_UNIQUE_TTL_SECONDS;
    settings.wifiCooldownSeconds = SENTINEL_DEFAULT_WIFI_ALERT_COOLDOWN_SECONDS;
    settings.motionCooldownSeconds = SENTINEL_DEFAULT_MOTION_ALERT_COOLDOWN_SECONDS;
    settings.combinedWindowSeconds = SENTINEL_DEFAULT_COMBINED_WINDOW_SECONDS;
    settings.heartbeatSeconds = SENTINEL_DEFAULT_HEARTBEAT_SECONDS;
    settings.portalSeconds = SENTINEL_DEFAULT_PORTAL_SECONDS;
    settings.serialVerbose = SENTINEL_DEFAULT_SERIAL_VERBOSE_PACKETS;
    settings.meshOutput = SENTINEL_DEFAULT_MESH_OUTPUT;
    settings.meshChannelIndex = SENTINEL_DEFAULT_MESH_CHANNEL_INDEX;
    settings.allowPublicChannel = SENTINEL_DEFAULT_ALLOW_PUBLIC_CHANNEL;
}

void loadSettings()
{
    loadFactoryDefaults();
    Preferences p;
    if (!p.begin("sentinel", true)) return;
    SentinelSettings saved = {};
    if (p.getBytesLength("config4") == sizeof(saved) &&
        p.getBytes("config4", &saved, sizeof(saved)) == sizeof(saved) && validSettings(saved))
        settings = saved;
    else if (p.getBytesLength("config4")==0 && p.getBytesLength("config3")==sizeof(SentinelSettingsV3)) {
        SentinelSettings migrated{};
        if(p.getBytes("config3",static_cast<SentinelSettingsV3*>(&migrated),sizeof(SentinelSettingsV3))==sizeof(SentinelSettingsV3) && validSettings(migrated)) settings=migrated;
    }
    p.end();
}

bool saveSettings()
{
    if (!validSettings(settings)) return false;
    Preferences p;
    if (!p.begin("sentinel", false)) return false;
    bool ok = p.putBytes("config4", &settings, sizeof(settings)) == sizeof(settings);
    SentinelSettings check = {};
    ok = ok && p.getBytes("config4", &check, sizeof(check)) == sizeof(check) &&
         memcmp(&check, &settings, sizeof(check)) == 0;
    p.end();
    Serial.printf("[SENTINEL] settings_saved=%s\n", ok ? "true" : "false");
    return ok;
}

uint32_t hashSource(const uint8_t *mac)
{
    uint32_t hash = 2166136261UL;
    for (uint8_t i = 0; i < 6; ++i) {
        hash ^= mac[i];
        hash *= 16777619UL;
    }
    return hash;
}

void wifiPacketCallback(void *buffer, wifi_promiscuous_pkt_type_t type)
{
    if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA)
        return;
    const auto *packet = static_cast<const wifi_promiscuous_pkt_t *>(buffer);
    if (packet == nullptr || packet->rx_ctrl.sig_len < 24)
        return;
    const uint8_t *frame = packet->payload;
    const uint8_t frameType = (frame[0] >> 2) & 0x03;
    const uint8_t frameSubtype = (frame[0] >> 4) & 0x0f;
    if (!((frameType == 0 && frameSubtype == 4) || frameType == 2))
        return;
    const uint8_t *source = frame + 10;
    if ((source[0] & 0x03U) != 2 || packet->rx_ctrl.rssi < settings.rssiMin)
        return;

    portENTER_CRITICAL(&eventMux);
    const uint8_t next = static_cast<uint8_t>((eventWrite + 1U) % EVENT_QUEUE_SIZE);
    wifiPacketsSeen++;
    if (next == eventRead) {
        queueDrops++;
        portEXIT_CRITICAL(&eventMux);
        return;
    }
    volatile WifiEvent &slot = eventQueue[eventWrite];
    slot.atMs = millis();
    slot.sourceHash = hashSource(source);
    slot.rssi = packet->rx_ctrl.rssi;
    slot.channel = packet->rx_ctrl.channel;
    slot.frameType = frameType;
    slot.frameSubtype = frameSubtype;
    eventWrite = next;
    portEXIT_CRITICAL(&eventMux);
}

bool popWifiEvent(WifiEvent &event)
{
    portENTER_CRITICAL(&eventMux);
    if (eventRead == eventWrite) {
        portEXIT_CRITICAL(&eventMux);
        return false;
    }
    const volatile WifiEvent &slot = eventQueue[eventRead];
    event.atMs = slot.atMs;
    event.sourceHash = slot.sourceHash;
    event.rssi = slot.rssi;
    event.channel = slot.channel;
    event.frameType = slot.frameType;
    event.frameSubtype = slot.frameSubtype;
    eventRead = static_cast<uint8_t>((eventRead + 1U) % EVENT_QUEUE_SIZE);
    portEXIT_CRITICAL(&eventMux);
    return true;
}

const char *frameName(const WifiEvent &event)
{
    if (event.frameType == 0 && event.frameSubtype == 4)
        return "probe-request";
    return event.frameSubtype == 8 ? "qos-data" : "data";
}

String checked(bool value)
{
    return value ? " checked" : "";
}

class SentinelPresenceModule : public SinglePortModule, private concurrency::OSThread
{
  public:
    SentinelPresenceModule()
        : SinglePortModule("sentinel-presence", meshtastic_PortNum_TEXT_MESSAGE_APP),
          concurrency::OSThread("SentinelPresence")
    {
    }

  protected:
    int32_t runOnce() override
    {
        if (!initialized) {
            initialize();
            return 25;
        }
        const uint32_t now = millis();
        if (portalActive) {
            portalDns.processNextRequest();
            portalServer.handleClient();
            if (portalStopRequested || now - portalStartedMs >= static_cast<uint32_t>(settings.portalSeconds) * 1000UL)
                stopPortalAndStartScanner();
            return 5;
        }
        pollMavlink(now);
        pollMotion(now);
        processWifiEvents(now);
        hopWifiChannel(now);
        printHeartbeat(now);
        return mavStarted ? 5 : 25;
    }

  private:
    bool initialized = false;
    bool portalActive = false;
    bool portalStopRequested = false;
    bool wifiReady = false;
    bool lastMotion = false;
    uint8_t wifiChannel = 1;
    uint32_t portalStartedMs = 0;
    uint32_t lastChannelHopMs = 0;
    uint32_t lastWifiSeenMs = 0;
    uint32_t lastMotionSeenMs = 0;
    uint32_t lastWifiAlertMs = 0;
    uint32_t lastMotionAlertMs = 0;
    uint32_t lastHeartbeatMs = 0;
    uint32_t lastMeshHeartbeatMs = 0;
    uint32_t uniqueWifiTotal = 0;
    SourceCache seen;
    HardwareSerial mavUart{2};
    bool mavStarted=false;
    sentinelMav::Parser mavParser;
    sentinelMav::Recent mavRecent;
    sentinelMav::Frame mavPending[sentinelMav::typeCount], mavTx[8];
    uint32_t mavPendingAt[sentinelMav::typeCount]={};
    unsigned mavNext=0, mavRead=0, mavWrite=0, mavOffset=0;
    uint32_t mavLastSend=0, mavSubmitted=0, mavSerialFrames=0, mavDropped=0;

    bool mavChannelAllowed(bool sending) {
        if(settings.mavChannel>=channels.getNumChannels()) return false;
        const auto &ch=channels.getByIndex(settings.mavChannel);
        const auto &key=ch.role==meshtastic_Channel_Role_SECONDARY && ch.settings.psk.size==0
            ? channels.getByIndex(channels.getPrimaryIndex()) : ch;
        return channelAllowed(ch.has_settings && key.has_settings && ch.role!=meshtastic_Channel_Role_DISABLED && key.role!=meshtastic_Channel_Role_DISABLED,
            config.lora.region!=meshtastic_Config_LoRaConfig_RegionCode_UNSET, !sending || config.lora.tx_enabled,
            false,key.settings.psk.bytes,key.settings.psk.size);
    }
    bool wantPacket(const meshtastic_MeshPacket *p) override {
        return p->decoded.portnum==meshtastic_PortNum_PRIVATE_APP;
    }
    ProcessMessage handleReceived(const meshtastic_MeshPacket &p) override {
        if(!mavStarted || !settings.mavMeshToSerial || p.channel!=settings.mavChannel ||
            p.from==nodeDB->getNodeNum() || !mavChannelAllowed(false)) return ProcessMessage::CONTINUE;
        sentinelMav::Frame f;
        if(!sentinelMav::decode(p.decoded.payload.bytes,p.decoded.payload.size,f)) {++mavDropped;return ProcessMessage::CONTINUE;}
        if(mavRecent.contains(f,millis())) return ProcessMessage::CONTINUE;
        unsigned next=(mavWrite+1)%8;
        if(next==mavRead) {++mavDropped;return ProcessMessage::CONTINUE;}
        mavTx[mavWrite]=f;mavWrite=next;mavRecent.add(f,millis());
        return ProcessMessage::CONTINUE;
    }
    void pollMavlink(uint32_t now) {
        if(!mavStarted) {
            if(!settings.mavSerialToMesh && !settings.mavMeshToSerial) return;
            mavUart.setRxBufferSize(2048);
            mavUart.begin(settings.mavBaud,SERIAL_8N1,44,43);
            mavStarted=true;
            Serial.printf("[SENTINEL] mavlink_uart RX=44 TX=43 baud=%lu serial_to_mesh=%u mesh_to_serial=%u channel=%u interval_ms=%u\n",
                (unsigned long)settings.mavBaud,settings.mavSerialToMesh,settings.mavMeshToSerial,settings.mavChannel,settings.mavIntervalMs);
        }
        for(unsigned budget=0;budget<1024 && mavUart.available();++budget) {
            uint8_t b=mavUart.read();
            sentinelMav::Frame f;
            if(mavParser.feed(b,now,f) && settings.mavSerialToMesh && !mavRecent.contains(f,now)) {
                int type=sentinelMav::validate(f.data,f.size);
                if(mavPending[type].size) ++mavDropped;
                mavPending[type]=f;mavPendingAt[type]=now;
            }
        }
        if(mavRead!=mavWrite) {
            auto &f=mavTx[mavRead];
            size_t count=mavUart.availableForWrite();
            if(count>f.size-mavOffset) count=f.size-mavOffset;
            if(count) mavOffset+=mavUart.write(f.data+mavOffset,count);
            if(mavOffset==f.size) {mavOffset=0;mavRead=(mavRead+1)%8;++mavSerialFrames;}
        }
        if(!settings.mavSerialToMesh || uint32_t(now-mavLastSend)<settings.mavIntervalMs || !mavChannelAllowed(true)) return;
        for(unsigned i=0;i<sentinelMav::typeCount;++i) {
            unsigned slot=(mavNext+i)%sentinelMav::typeCount;
            auto &f=mavPending[slot];
            if(!f.size) continue;
            if(uint32_t(now-mavPendingAt[slot])>=30000 || mavRecent.contains(f,now)) {f.size=0;++mavDropped;continue;}
            auto *p=allocDataPacket();if(!p) {++mavDropped;return;}
            p->decoded.portnum=meshtastic_PortNum_PRIVATE_APP;
            p->channel=settings.mavChannel;p->want_ack=false;
            p->decoded.payload.size=f.size+4;
            memcpy(p->decoded.payload.bytes,"SMV1",4);memcpy(p->decoded.payload.bytes+4,f.data,f.size);
            mavRecent.add(f,now);f.size=0;mavNext=(slot+1)%sentinelMav::typeCount;mavLastSend=now;
            service->sendToMesh(p,RX_SRC_LOCAL,true);++mavSubmitted;break;
        }
    }

    void initialize()
    {
        initialized = true;
        loadSettings();
        Serial.println();
        Serial.println("[SENTINEL] boot firmware=presence-meshtastic-v2");
        Serial.printf("[SENTINEL] board=seeed-xiao-esp32s3-sx1262 label=%s serial_baud=115200\n", settings.label);
        Serial.printf("[SENTINEL] wiring rcwl_out=GPIO%u default_header=D2 rcwl_vin=5V rcwl_gnd=GND\n", settings.motionGpio);
        printConfiguration();
        startPortal();
    }

    void printConfiguration()
    {
        Serial.printf("{\"sentinel\":\"%s\",\"event\":\"config\",\"motion_gpio\":%u,\"rssi_min\":%d,"
                      "\"scan_channels\":\"%u-%u\",\"dwell_ms\":%u,\"serial_verbose\":%s,"
                      "\"mesh_output\":%s,\"mesh_channel_index\":%u,\"allow_public\":%s,\"portal_seconds\":%u}\n",
                      settings.label, settings.motionGpio, settings.rssiMin, settings.channelFirst, settings.channelLast,
                      settings.channelDwellMs, settings.serialVerbose ? "true" : "false",
                      settings.meshOutput ? "true" : "false", settings.meshChannelIndex,
                      settings.allowPublicChannel ? "true" : "false", settings.portalSeconds);
    }

    String portalPage()
    {
        String page;
        page.reserve(6000);
        page += F("<!doctype html><html><head><meta name=viewport content='width=device-width,initial-scale=1'>"
                  "<title>Sentinel Setup</title><style>body{font-family:system-ui;background:#101512;color:#e5eee8;max-width:760px;"
                  "margin:auto;padding:20px}h1{color:#72d6a0}fieldset{border:1px solid #405348;margin:16px 0;padding:14px}"
                  "label{display:block;margin:10px 0}input[type=text],input[type=number]{width:100%;box-sizing:border-box;"
                  "padding:9px;background:#18211c;color:#fff;border:1px solid #53685b}button{padding:12px 18px;background:#2e8b57;"
                  "color:white;border:0;font-weight:700}.hint{color:#a9b8ae;font-size:.9em}"
                  "body{max-width:1100px;background:#0b1115;color:#e0e9e9;padding:32px}"
                  "h1{font-size:42px;letter-spacing:5px;color:#d4e2d1;margin:8px 0}"
                  "form{display:grid;grid-template-columns:1fr 1fr;gap:18px;grid-template-areas:'i w' 'm w' 't b' 's s'}"
                  "fieldset:nth-of-type(1){grid-area:i}fieldset:nth-of-type(2){grid-area:w}fieldset:nth-of-type(3){grid-area:m}"
                  "fieldset:nth-of-type(4){grid-area:t}fieldset:nth-of-type(5){grid-area:b}button{grid-area:s;min-height:52px}"
                  "fieldset{margin:0;border:1px solid #344349;background:#111c22;padding:20px;min-width:0}"
                  "legend{color:#a9c58e;text-transform:uppercase;font-size:12px;letter-spacing:2px;padding:0 8px}"
                  "input:not([type=checkbox]),select{display:block;width:100%;box-sizing:border-box;padding:10px;"
                  "margin-top:7px;color:#e0e9e9;background:#0c151a;border:1px solid #3b4e55;font-size:15px}"
                  "label{font-size:13px;color:#c0ccce;margin:12px 0}button{background:#b5c98f;color:#101912;cursor:pointer}"
                  ".eyebrow{font:12px monospace;letter-spacing:3px;color:#b5c98f}.sub{color:#92a5ad;line-height:1.6;margin-bottom:26px}"
                  "@media(max-width:650px){body{padding:18px}form{grid-template-columns:1fr;grid-template-areas:'i' 'w' 'm' 't' 'b' 's'}h1{font-size:30px}}"
                  "</style></head><body><div class=eyebrow>MKME FIELD SYSTEMS / REMOTE SENSING + MESH</div>"
                  "<h1>SENTINEL</h1><p class=sub>NODE CONFIGURATION &nbsp; / &nbsp; LOCAL SETUP WINDOW<br>"
                  "Set the sensor and reporting plan. Save to start detection, or let the setup window expire.</p>"
                  "<form method=post action=/save><fieldset><legend>Identity and input</legend>");
        page += "<label>Node label<input name=label maxlength=24 value='" + String(settings.label) + "'></label>";
        page += "<label>RCWL motion GPIO<input type=number name=motionPin min=3 max=4 value='" + String(settings.motionGpio) + "'></label>";
        page += F("<p class=hint>Default: XIAO D2 / GPIO3.</p></fieldset><fieldset><legend>Passive Wi-Fi detection</legend>");
        page += "<label>Minimum RSSI dBm<input type=number name=rssi min=-100 max=-20 value='" + String(settings.rssiMin) + "'></label>";
        page += "<label>First channel<input type=number name=chanFirst min=1 max=11 value='" + String(settings.channelFirst) + "'></label>";
        page += "<label>Last channel<input type=number name=chanLast min=1 max=11 value='" + String(settings.channelLast) + "'></label>";
        page += "<label>Channel dwell ms<input type=number name=dwell min=100 max=2000 value='" + String(settings.channelDwellMs) + "'></label>";
        page += "<label>Unique-device memory seconds<input type=number name=ttl min=10 max=3600 value='" + String(settings.uniqueTtlSeconds) + "'></label>";
        page += "<label>Wi-Fi alert cooldown seconds<input type=number name=wifiCool min=5 max=3600 value='" + String(settings.wifiCooldownSeconds) + "'></label>";
        page += "<label><input type=checkbox name=serialVerb" + checked(settings.serialVerbose) + "> Print every qualifying packet to serial</label></fieldset>";
        page += F("<fieldset><legend>Motion and reporting</legend>");
        page += "<label>Motion alert cooldown seconds<input type=number name=motionCool min=2 max=3600 value='" + String(settings.motionCooldownSeconds) + "'></label>";
        page += "<label>Combined motion + Wi-Fi window seconds<input type=number name=combine min=1 max=300 value='" + String(settings.combinedWindowSeconds) + "'></label>";
        page += "<label>Serial heartbeat seconds<input type=number name=heartbeat min=10 max=3600 value='" + String(settings.heartbeatSeconds) + "'></label></fieldset>";
        page += F("<fieldset><legend>Meshtastic output</legend>");
        page += "<label><input type=checkbox name=meshOut" + checked(settings.meshOutput) + "> Send detections over Meshtastic</label>";
        page += "<label><input type=checkbox name=meshHeartbeat" + checked(settings.meshHeartbeat) + "> Send periodic sensor status over mesh (minimum 60 seconds)</label>";
        page += "<label>Meshtastic output channel<select name=meshChan>";
        bool selectedAvailable=false;
        for (uint8_t i=0;i<channels.getNumChannels();++i) {
            if (channels.getByIndex(i).role == meshtastic_Channel_Role_DISABLED) continue;
            if (i==settings.meshChannelIndex) selectedAvailable=true;
            String name=channels.getName(i);
            name.replace("&", "&amp;"); name.replace("<", "&lt;"); name.replace(">", "&gt;");
            page += "<option value='"+String(i)+"'"+(i==settings.meshChannelIndex?" selected":"")+">"+String(i)+" - "+name+"</option>";
        }
        if (!selectedAvailable)
            page += "<option selected value='"+String(settings.meshChannelIndex)+"'>"+String(settings.meshChannelIndex)+" - unavailable (reports blocked)</option>";
        page += "</select></label>";
        page += "<label><input type=checkbox name=allowPublic" + checked(settings.allowPublicChannel) + "> Allow automatic alerts on public/open channels</label>";
        page += F("<p class=hint>Use a configured private channel for deployed automatic alerts.</p></fieldset><fieldset><legend>MAVLink telemetry bridge</legend>");
        page += "<label><input type=checkbox name=mavOut"+checked(settings.mavSerialToMesh)+"> Serial RX to mesh</label>";
        page += "<label><input type=checkbox name=mavIn"+checked(settings.mavMeshToSerial)+"> Mesh to serial TX</label>";
        page += "<label>UART baud<select name=mavBaud>";
        for(unsigned baud: {9600U,19200U,38400U,57600U,115200U})
            page += "<option value='"+String(baud)+"'"+(baud==settings.mavBaud?" selected":"")+">"+String(baud)+"</option>";
        page += "</select></label><label>Telemetry mesh channel index<input type=number name=mavChan min=0 max=7 value='"+String(settings.mavChannel)+"'></label>";
        page += "<label>Minimum mesh send interval (ms)<input type=number name=mavInterval min=1000 max=60000 value='"+String(settings.mavIntervalMs)+"'></label>";
        page += F("<p class=hint>3.3V UART: RX D7/GPIO44, TX D6/GPIO43, common GND. Private channel required. Both directions off by default. Supported telemetry only; no flight commands. USB remains diagnostic.</p></fieldset><fieldset><legend>Boot portal</legend>");
        page += "<label>Portal duration each boot, seconds<input type=number name=portalSec min=15 max=600 value='" + String(settings.portalSeconds) + "'></label></fieldset>";
        page += F("<button type=submit>Save and start detection</button></form><p><a href=/status style='color:#72d6a0'>View JSON status</a></p></body></html>");
        return page;
    }

    void startPortal()
    {
        if (config.network.wifi_enabled)
            Serial.println("[SENTINEL] WARNING Meshtastic Wi-Fi networking is enabled; disable it for passive scanning");
        WiFi.mode(WIFI_AP);
        const String chipSuffix = String(static_cast<uint32_t>(ESP.getEfuseMac()), HEX);
        const String apName = String("SENTINEL-") + chipSuffix.substring(chipSuffix.length() > 4 ? chipSuffix.length() - 4 : 0);
        if (!WiFi.softAP(apName.c_str(), SENTINEL_PORTAL_AP_PASSWORD)) {
            Serial.println("[SENTINEL] ERROR config_portal_start_failed; starting detector directly");
            stopPortalAndStartScanner();
            return;
        }
        portalServer.on("/", HTTP_GET, [this]() { portalServer.send(200, "text/html", portalPage()); });
        portalServer.on("/save", HTTP_POST, [this]() { handlePortalSave(); });
        portalServer.on("/status", HTTP_GET, [this]() {
            char json[384];
            const uint32_t elapsed = millis() - portalStartedMs;
            const uint32_t total = static_cast<uint32_t>(settings.portalSeconds) * 1000UL;
            const uint32_t remaining = elapsed < total ? (total - elapsed) / 1000UL : 0;
            snprintf(json, sizeof(json),
                     "{\"label\":\"%s\",\"motion_gpio\":%u,\"rssi_min\":%d,\"mesh_output\":%s,"
                     "\"mesh_channel_index\":%u,\"serial_verbose\":%s,\"portal_remaining_seconds\":%lu}",
                     settings.label, settings.motionGpio, settings.rssiMin, settings.meshOutput ? "true" : "false",
                     settings.meshChannelIndex, settings.serialVerbose ? "true" : "false", static_cast<unsigned long>(remaining));
            portalServer.send(200, "application/json", json);
        });
        portalServer.onNotFound([this]() {
            portalServer.sendHeader("Location", "http://192.168.4.1/", true);
            portalServer.send(302, "text/plain", "");
        });
        portalServer.begin();
        portalDns.start(53, "*", WiFi.softAPIP());
        portalStartedMs = millis();
        portalActive = true;
        Serial.printf("[SENTINEL] config_portal=ready ssid=%s password=%s url=http://192.168.4.1/ timeout_seconds=%u\n",
                      apName.c_str(), SENTINEL_PORTAL_AP_PASSWORD, settings.portalSeconds);
    }

    void handlePortalSave()
    {
        const SentinelSettings previous = settings;
        const char *required[]={"motionPin","rssi","chanFirst","chanLast","dwell","ttl","wifiCool","motionCool","combine","heartbeat","portalSec","meshChan","mavBaud","mavChan","mavInterval"};
        for (const char *key: required) {
            String value=portalServer.arg(key);
            bool valid=value.length()>0 && value.length()<8;
            for (unsigned i=0;i<value.length();++i)
                if (!isDigit(value[i]) && !(i==0 && value[i]=='-' && value.length()>1)) valid=false;
            if (!valid) {portalServer.send(400,"text/plain","Missing or invalid numeric setting");return;}
        }
        long pin=portalServer.arg("motionPin").toInt();
        if (pin!=3 && pin!=4) {portalServer.send(400,"text/plain","Use GPIO3 (D2) or GPIO4 (D3)");return;}

        String label = portalServer.arg("label");
        label.trim();
        String safeLabel;
        for (size_t i = 0; i < label.length() && safeLabel.length() < 24; ++i) {
            const char c = label[i];
            if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == ' ')
                safeLabel += c;
        }
        if (!safeLabel.isEmpty())
            strlcpy(settings.label, safeLabel.c_str(), sizeof(settings.label));
        settings.motionGpio = clampU32(portalServer.arg("motionPin").toInt(), 1, 48);
        settings.rssiMin = static_cast<int8_t>(clampU32(portalServer.arg("rssi").toInt() + 100, 0, 80) - 100);
        settings.channelFirst = clampU32(portalServer.arg("chanFirst").toInt(), 1, 11);
        settings.channelLast = clampU32(portalServer.arg("chanLast").toInt(), settings.channelFirst, 11);
        settings.channelDwellMs = clampU32(portalServer.arg("dwell").toInt(), 100, 2000);
        settings.uniqueTtlSeconds = clampU32(portalServer.arg("ttl").toInt(), 10, 3600);
        settings.wifiCooldownSeconds = clampU32(portalServer.arg("wifiCool").toInt(), 5, 3600);
        settings.motionCooldownSeconds = clampU32(portalServer.arg("motionCool").toInt(), 2, 3600);
        settings.combinedWindowSeconds = clampU32(portalServer.arg("combine").toInt(), 1, 300);
        settings.heartbeatSeconds = clampU32(portalServer.arg("heartbeat").toInt(), 10, 3600);
        settings.portalSeconds = clampU32(portalServer.arg("portalSec").toInt(), 15, 600);
        settings.meshChannelIndex = clampU32(portalServer.arg("meshChan").toInt(), 0, 7);
        settings.serialVerbose = portalServer.hasArg("serialVerb");
        settings.meshOutput = portalServer.hasArg("meshOut");
        settings.meshHeartbeat = portalServer.hasArg("meshHeartbeat");
        settings.allowPublicChannel = portalServer.hasArg("allowPublic");
        settings.mavSerialToMesh=portalServer.hasArg("mavOut");
        settings.mavMeshToSerial=portalServer.hasArg("mavIn");
        long baud=portalServer.arg("mavBaud").toInt(), channel=portalServer.arg("mavChan").toInt(), interval=portalServer.arg("mavInterval").toInt();
        if(!validMavBaud(baud) || channel<0 || channel>7 || interval<1000 || interval>60000) {
            settings=previous;portalServer.send(400,"text/plain","Invalid MAVLink bridge setting");return;
        }
        settings.mavBaud=baud;settings.mavChannel=channel;settings.mavIntervalMs=interval;
        if (!saveSettings()) {
            settings=previous;
            portalServer.send(500,"text/plain","Settings not saved. Try again; portal remains open.");
            return;
        }
        printConfiguration();
        portalServer.send(200, "text/html", savedPage());
        portalStopRequested = true;
    }

    String savedPage()
    {
        return F("<!doctype html><html><meta name=viewport content='width=device-width,initial-scale=1'><title>Sentinel saved</title>"
                 "<style>body{background:#0b1115;color:#dce5df;font:18px system-ui;margin:12vh auto;padding:24px;max-width:700px}"
                 "small{color:#b5c98f;letter-spacing:3px}h1{font-size:40px}p{color:#a3b6bf;line-height:1.7}"
                 "section{border:1px solid #344349;background:#111c22;padding:32px}</style>"
                 "<small>SENTINEL / LOCAL SETUP</small><h1>Configuration saved</h1><section>"
                 "<p>The access point is closing. Motion monitoring and passive Wi-Fi detection are starting.</p>"
                 "<p>Open the USB serial monitor at <b>115200 baud</b> to check scanner status and sensor reports.</p>"
                 "<p>Restart the node to reopen the setup window.</p></section></html>");
    }

    void stopPortalAndStartScanner()
    {
        {
            portalDns.stop();
            portalServer.stop();
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_OFF);
            delay(100);
        }
        portalActive = false;
        portalStopRequested = false;
        pinMode(settings.motionGpio, INPUT);
        lastMotion = digitalRead(settings.motionGpio) == HIGH;
        startScanner();
    }

    void startScanner()
    {
        if (config.network.wifi_enabled) {
            Serial.println("[SENTINEL] ERROR wifi-networking-enabled; passive scanner not started");
            return;
        }
        esp_err_t result = esp_event_loop_create_default();
        if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
            Serial.printf("[SENTINEL] ERROR event-loop-init code=%d\n", static_cast<int>(result));
            return;
        }
        wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
        result = esp_wifi_init(&init);
        if (result != ESP_OK && result != ESP_ERR_WIFI_INIT_STATE) {
            Serial.printf("[SENTINEL] ERROR wifi-init code=%d\n", static_cast<int>(result));
            return;
        }
        if (esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK ||
            esp_wifi_set_country_code("CA", true) != ESP_OK || esp_wifi_set_mode(WIFI_MODE_NULL) != ESP_OK) {
            Serial.println("[SENTINEL] ERROR wifi-configuration-failed"); return;
        }
        result = esp_wifi_start();
        if (result != ESP_OK) {
            Serial.printf("[SENTINEL] ERROR wifi-start code=%d\n", static_cast<int>(result));
            return;
        }
        wifi_promiscuous_filter_t filter = {};
        filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA;
        wifiChannel = settings.channelFirst;
        if (esp_wifi_set_promiscuous_filter(&filter) != ESP_OK ||
            esp_wifi_set_promiscuous_rx_cb(wifiPacketCallback) != ESP_OK ||
            esp_wifi_set_channel(wifiChannel, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
            Serial.println("[SENTINEL] ERROR wifi-filter-or-channel-failed"); return;
        }
        result = esp_wifi_set_promiscuous(true);
        wifiReady = result == ESP_OK;
        Serial.println("[SENTINEL] config_portal=closed detector=starting");
        Serial.printf("[SENTINEL] wifi_scanner=%s init_code=%d channel=%u motion_initial=%u\n",
                      wifiReady ? "ready" : "failed", static_cast<int>(result), wifiChannel, lastMotion ? 1U : 0U);
    }

    void hopWifiChannel(uint32_t now)
    {
        if (!wifiReady || now - lastChannelHopMs < settings.channelDwellMs)
            return;
        lastChannelHopMs = now;
        wifiChannel++;
        if (wifiChannel > settings.channelLast)
            wifiChannel = settings.channelFirst;
        const esp_err_t result = esp_wifi_set_channel(wifiChannel, WIFI_SECOND_CHAN_NONE);
        if (result != ESP_OK)
            Serial.printf("[SENTINEL] ERROR channel-hop channel=%u code=%d\n", wifiChannel, static_cast<int>(result));
    }

    bool markUnique(uint32_t hash, uint32_t now)
    {
        bool fresh = seen.observe(hash, now, uint32_t(settings.uniqueTtlSeconds)*1000);
        if (fresh) ++uniqueWifiTotal;
        return fresh;
    }

    void processWifiEvents(uint32_t now)
    {
        WifiEvent event = {};
        for (unsigned budget=0; budget<8 && popWifiEvent(event); ++budget) {
            const bool unique = markUnique(event.sourceHash, event.atMs);
            if (settings.serialVerbose || unique) {
                Serial.printf("{\"sentinel\":\"%s\",\"event\":\"wifi\",\"ms\":%lu,\"new\":%s,\"rssi\":%d,"
                              "\"channel\":%u,\"frame\":\"%s\",\"unique_total\":%lu}\n",
                              settings.label, static_cast<unsigned long>(event.atMs), unique ? "true" : "false",
                              event.rssi, event.channel, frameName(event), static_cast<unsigned long>(uniqueWifiTotal));
            }
            lastWifiSeenMs = event.atMs;
            if (!unique)
                continue;
            if (lastWifiAlertMs == 0 ||
                now - lastWifiAlertMs >= static_cast<uint32_t>(settings.wifiCooldownSeconds) * 1000UL) {
                lastWifiAlertMs = now;
                const bool combined = lastMotionSeenMs != 0 &&
                                      now - lastMotionSeenMs <= static_cast<uint32_t>(settings.combinedWindowSeconds) * 1000UL;
                sendMeshAlert(combined ? "MOTION+WIFI DETECTED" : "WIFI DETECTED", event.rssi, event.channel);
            }
        }
    }

    void pollMotion(uint32_t now)
    {
        const bool motion = digitalRead(settings.motionGpio) == HIGH;
        if (motion) lastMotionSeenMs = now;
        if (motion == lastMotion)
            return;
        lastMotion = motion;
        Serial.printf("{\"sentinel\":\"%s\",\"event\":\"motion\",\"ms\":%lu,\"active\":%s,\"gpio\":%u}\n",
                      settings.label, static_cast<unsigned long>(now), motion ? "true" : "false", settings.motionGpio);
        if (!motion)
            return;
        lastMotionSeenMs = now;
        if (lastMotionAlertMs == 0 ||
            now - lastMotionAlertMs >= static_cast<uint32_t>(settings.motionCooldownSeconds) * 1000UL) {
            lastMotionAlertMs = now;
            const bool combined = lastWifiSeenMs != 0 &&
                                  now - lastWifiSeenMs <= static_cast<uint32_t>(settings.combinedWindowSeconds) * 1000UL;
            sendMeshAlert(combined ? "MOTION+WIFI DETECTED" : "MOTION DETECTED", 0, 0);
        }
    }

    void sendMeshAlert(const char *event, int8_t rssi, uint8_t wifiDetectedChannel)
    {
        char message[96];
        if (wifiDetectedChannel != 0)
            snprintf(message, sizeof(message), "%s: %s RSSI %d WIFI-CH %u", settings.label, event, rssi, wifiDetectedChannel);
        else
            snprintf(message, sizeof(message), "%s: %s", settings.label, event);

        const uint8_t meshChannel = settings.meshChannelIndex;
        const char *meshChannelName = meshChannel < channels.getNumChannels() ? channels.getName(meshChannel) : "INVALID";
        Serial.printf("[SENTINEL] mesh_decision enabled=%u channel_index=%u channel_name=%s message=\"%s\"\n",
                      settings.meshOutput ? 1U : 0U, meshChannel, meshChannelName, message);
        if (!settings.meshOutput) {
            Serial.println("[SENTINEL] mesh_skipped reason=disabled-in-portal");
            return;
        }
        if (meshChannel >= channels.getNumChannels()) {
            Serial.println("[SENTINEL] mesh_blocked reason=channel-index-not-configured");
            return;
        }
        const auto &selected = channels.getByIndex(meshChannel);
        const auto &effective = selected.role == meshtastic_Channel_Role_SECONDARY && selected.settings.psk.size == 0
            ? channels.getByIndex(channels.getPrimaryIndex()) : selected;
        const bool enabled = selected.has_settings && effective.has_settings &&
            selected.role != meshtastic_Channel_Role_DISABLED && effective.role != meshtastic_Channel_Role_DISABLED;
        if (!channelAllowed(enabled,config.lora.region != meshtastic_Config_LoRaConfig_RegionCode_UNSET,
                            config.lora.tx_enabled,settings.allowPublicChannel,effective.settings.psk.bytes,
                            effective.settings.psk.size)) {
            Serial.println("[SENTINEL] mesh_blocked reason=channel-key-or-radio-policy");
            return;
        }
        meshtastic_MeshPacket *packet = allocDataPacket();
        if (packet == nullptr) {
            Serial.println("[SENTINEL] mesh_failed reason=no-packet-buffer");
            return;
        }
        packet->channel = meshChannel;
        packet->want_ack = false;
        packet->decoded.payload.size = strnlen(message, sizeof(message));
        memcpy(packet->decoded.payload.bytes, message, packet->decoded.payload.size);
        const uint32_t packetId = packet->id;
        const uint32_t bytes = packet->decoded.payload.size;
        service->sendToMesh(packet, RX_SRC_LOCAL, true);
        Serial.printf("[SENTINEL] mesh_submitted packet_id=%lu bytes=%lu channel_index=%u channel_name=%s\n",
                      static_cast<unsigned long>(packetId), static_cast<unsigned long>(bytes), meshChannel, meshChannelName);
    }

    void printHeartbeat(uint32_t now)
    {
        if (settings.meshOutput && settings.meshHeartbeat &&
            (lastMeshHeartbeatMs == 0 || uint32_t(now-lastMeshHeartbeatMs) >=
             uint32_t(settings.heartbeatSeconds < 60 ? 60 : settings.heartbeatSeconds)*1000)) {
            lastMeshHeartbeatMs = now;
            char status[60];
            snprintf(status,sizeof(status),"ONLINE WIFI %lu MOTION %u SCAN %s",(unsigned long)uniqueWifiTotal,
                     lastMotion ? 1U : 0U,wifiReady ? "OK" : "FAULT");
            sendMeshAlert(status,0,0);
        }
        if (lastHeartbeatMs != 0 && now - lastHeartbeatMs < static_cast<uint32_t>(settings.heartbeatSeconds) * 1000UL)
            return;
        lastHeartbeatMs = now;
        Serial.printf("[SENTINEL] mavlink mesh_submitted=%lu serial_frames=%lu dropped_or_replaced=%lu\n",
            (unsigned long)mavSubmitted,(unsigned long)mavSerialFrames,(unsigned long)mavDropped);
        uint32_t packets;
        uint32_t drops;
        portENTER_CRITICAL(&eventMux);
        packets = wifiPacketsSeen;
        drops = queueDrops;
        portEXIT_CRITICAL(&eventMux);
        Serial.printf("{\"sentinel\":\"%s\",\"event\":\"heartbeat\",\"ms\":%lu,\"motion\":%s,"
                      "\"wifi_ready\":%s,\"wifi_channel\":%u,\"wifi_packets\":%lu,\"wifi_unique\":%lu,"
                      "\"queue_drops\":%lu,\"mesh_output\":%s,\"mesh_channel_index\":%u}\n",
                      settings.label, static_cast<unsigned long>(now), lastMotion ? "true" : "false",
                      wifiReady ? "true" : "false", wifiChannel, static_cast<unsigned long>(packets),
                      static_cast<unsigned long>(uniqueWifiTotal), static_cast<unsigned long>(drops),
                      settings.meshOutput ? "true" : "false", settings.meshChannelIndex);
    }
};

} // namespace

void setupSentinelPresence()
{
    // This dedicated target owns Wi-Fi. Override in RAM before initWifi(); preserve saved mesh settings.
    config.network.wifi_enabled = false;
    config.power.is_power_saving = false;
    new SentinelPresenceModule();
}

#else
void setupSentinelPresence() {}
#endif
