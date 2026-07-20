/*
  SentinelClearDemo.ino

  Generates XTOC/XCOM Sentinel (T=11) CLEAR packets and prints them to Serial.

  Wiring (example):
  - RCWL-0516 OUT -> MOTION_PIN
  - RCWL-0516 GND/VCC as required

  Output format:
    X1.11.C.<ID>.1/1.<base64url(payloadBytes)>
*/

#include <Arduino.h>
#include <math.h>

// ---- User config ----

static const uint32_t NODE_ID = 0xDEADBEEF;   // e.g. Meshtastic node id (hex)
static const char* LABEL = "LP-1";            // optional

static const float LAT = 35.9606f;            // required
static const float LON = -83.9207f;            // required

static const uint8_t MOTION_PIN = 4;           // RCWL-0516 digital OUT

// If you don't have RTC/NTP, set an approximate epoch base here.
// The sketch reports: epochMs = EPOCH_BASE_MS + millis().
static const uint64_t EPOCH_BASE_MS = 1735689600000ULL; // 2025-01-01T00:00:00Z (example)

// Optional IO masks (set HAS_IO=false if unused).
static const bool HAS_IO = false;
static const uint8_t IO_IN_MASK = 0x00;
static const uint8_t IO_OUT_MASK = 0x00;

// Sensor type codes (team convention)
static const uint8_t SENSOR_RCWL_MOTION = 1;  // value 0/1

// ---- Sentinel template constants ----

static const uint8_t SENTINEL_VERSION = 1;
static const uint8_t SENTINEL_MAX_LABEL_BYTES = 32;

struct SensorReading {
  uint8_t type;
  int16_t value;
};

static inline void writeU32BE(uint8_t* b, int& o, uint32_t v) {
  b[o++] = (uint8_t)((v >> 24) & 0xFF);
  b[o++] = (uint8_t)((v >> 16) & 0xFF);
  b[o++] = (uint8_t)((v >> 8) & 0xFF);
  b[o++] = (uint8_t)(v & 0xFF);
}

static inline void writeI32BE(uint8_t* b, int& o, int32_t v) {
  writeU32BE(b, o, (uint32_t)v);
}

static inline void writeI16BE(uint8_t* b, int& o, int16_t v) {
  b[o++] = (uint8_t)((v >> 8) & 0xFF);
  b[o++] = (uint8_t)(v & 0xFF);
}

// Minimal base64url encoder (no padding '=')
static String base64UrlEncode(const uint8_t* data, int len) {
  static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
  String out;
  out.reserve(((len + 2) / 3) * 4);

  int i = 0;
  while (i + 2 < len) {
    const uint32_t n = ((uint32_t)data[i] << 16) | ((uint32_t)data[i + 1] << 8) | (uint32_t)data[i + 2];
    out += tbl[(n >> 18) & 63];
    out += tbl[(n >> 12) & 63];
    out += tbl[(n >> 6) & 63];
    out += tbl[n & 63];
    i += 3;
  }

  const int rem = len - i;
  if (rem == 1) {
    const uint32_t n = ((uint32_t)data[i] << 16);
    out += tbl[(n >> 18) & 63];
    out += tbl[(n >> 12) & 63];
  } else if (rem == 2) {
    const uint32_t n = ((uint32_t)data[i] << 16) | ((uint32_t)data[i + 1] << 8);
    out += tbl[(n >> 18) & 63];
    out += tbl[(n >> 12) & 63];
    out += tbl[(n >> 6) & 63];
  }

  return out;
}

static String makePacketId(int len = 8) {
  static const char* alphabet = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
  const int n = (int)strlen(alphabet);
  String out;
  out.reserve(len);
  for (int i = 0; i < len; i++) {
    out += alphabet[random(n)];
  }
  return out;
}

void setup() {
  Serial.begin(115200);
  pinMode(MOTION_PIN, INPUT);

  // Best-effort seed.
  randomSeed((uint32_t)millis());
}

void loop() {
  const bool motion = (digitalRead(MOTION_PIN) == HIGH);
  const uint64_t epochMs = EPOCH_BASE_MS + (uint64_t)millis();
  const uint32_t unixMinutes = (uint32_t)(epochMs / 60000ULL);

  // Build sensor list (add more as needed)
  SensorReading sensors[1];
  const uint8_t sensorCount = 1;
  sensors[0] = { SENSOR_RCWL_MOTION, (int16_t)(motion ? 1 : 0) };

  const bool hasLabel = (LABEL && LABEL[0] != '\0');
  const int labelLen = hasLabel ? (int)min((size_t)SENTINEL_MAX_LABEL_BYTES, strlen(LABEL)) : 0;

  uint8_t flags = 0;
  if (motion) flags |= 1;      // alert
  if (HAS_IO) flags |= 2;
  if (hasLabel) flags |= 4;

  // Layout v1:
  // base(19) + io(2?) + label(1+N?) + sensors(sensorCount*3)
  const int totalLen = 19 + (HAS_IO ? 2 : 0) + (hasLabel ? (1 + labelLen) : 0) + (int)sensorCount * 3;
  if (totalLen > 128) {
    Serial.println("ERROR: payload too large");
    delay(5000);
    return;
  }

  uint8_t buf[128];
  int o = 0;
  buf[o++] = SENTINEL_VERSION;
  buf[o++] = sensorCount;
  writeU32BE(buf, o, unixMinutes);

  const int32_t latE5 = (int32_t)lround((double)LAT * 100000.0);
  const int32_t lonE5 = (int32_t)lround((double)LON * 100000.0);
  writeI32BE(buf, o, latE5);
  writeI32BE(buf, o, lonE5);
  writeU32BE(buf, o, NODE_ID);
  buf[o++] = flags;

  if (HAS_IO) {
    buf[o++] = IO_IN_MASK;
    buf[o++] = IO_OUT_MASK;
  }

  if (hasLabel) {
    buf[o++] = (uint8_t)labelLen;
    memcpy(buf + o, LABEL, (size_t)labelLen);
    o += labelLen;
  }

  for (uint8_t i = 0; i < sensorCount; i++) {
    buf[o++] = sensors[i].type;
    writeI16BE(buf, o, sensors[i].value);
  }

  const String payloadB64Url = base64UrlEncode(buf, o);
  const String pktId = makePacketId(8);
  const String wrapper = String("X1.11.C.") + pktId + ".1/1." + payloadB64Url;

  Serial.println(wrapper);
  delay(5000);
}

