/*
  SentinelControlDemo.ino

  Minimal Arduino demo for receiving XTOC/XCOM CONTROL (T=16) CLEAR packets
  and applying them to an 8-bit output bank on the board.

  This sketch:
  - Listens on Serial for packet lines like: X1.16.C.<ID>.1/1.<payloadB64Url>
  - Decodes the CONTROL v1 payload
  - If nodeId matches NODE_ID, applies outOp/outMask to a local output state
  - Optionally applies control stack entries (type 0..7 => set that output 0/1)
  - If ackRequested is set, prints a Sentinel (T=11) CLEAR packet reflecting the new outMask

  IMPORTANT:
  - CONTROL commands should typically be sent as SECURE in the real world.
    (This sketch only demonstrates CLEAR for simplicity.)
  - Do not transmit encrypted/obscured content on amateur radio; follow local rules.
*/

#include <Arduino.h>
#include <math.h>

// ---- User config ----

static const uint32_t NODE_ID = 0xDEADBEEF; // must match packets you send
static const char* LABEL = "LP-1";          // optional (sent in ACK report)

// Used only for ACK Sentinel packets (Sentinel requires a location).
static const float LAT = 35.9606f;
static const float LON = -83.9207f;

// If you don't have RTC/NTP, set an approximate epoch base here.
// The sketch reports: epochMs = EPOCH_BASE_MS + millis().
static const uint64_t EPOCH_BASE_MS = 1735689600000ULL; // 2025-01-01T00:00:00Z (example)

// Output bank mapping: bit0..bit7 => these Arduino pins.
// Pick pins that suit your board/wiring (avoid 0/1 which are Serial on many boards).
static const uint8_t OUT_PINS[8] = { 2, 3, 4, 5, 6, 7, 8, 9 };

// ---- Template constants ----

static const uint8_t CONTROL_VERSION = 1;
static const uint8_t CONTROL_MAX_CONTROLS = 32;

static const uint8_t SENTINEL_VERSION = 1;
static const uint8_t SENTINEL_MAX_LABEL_BYTES = 32;

// ---- Helpers ----

static inline uint32_t readU32BE(const uint8_t* b, int& o) {
  uint32_t v = 0;
  v |= (uint32_t)b[o++] << 24;
  v |= (uint32_t)b[o++] << 16;
  v |= (uint32_t)b[o++] << 8;
  v |= (uint32_t)b[o++];
  return v;
}

static inline int16_t readI16BE(const uint8_t* b, int& o) {
  int16_t v = (int16_t)(((uint16_t)b[o] << 8) | (uint16_t)b[o + 1]);
  o += 2;
  return v;
}

static inline void writeU32BE(uint8_t* b, int& o, uint32_t v) {
  b[o++] = (uint8_t)((v >> 24) & 0xFF);
  b[o++] = (uint8_t)((v >> 16) & 0xFF);
  b[o++] = (uint8_t)((v >> 8) & 0xFF);
  b[o++] = (uint8_t)(v & 0xFF);
}

static inline void writeI32BE(uint8_t* b, int& o, int32_t v) {
  writeU32BE(b, o, (uint32_t)v);
}

static String makePacketId(int len = 8) {
  static const char* alphabet = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
  const int n = (int)strlen(alphabet);
  String out;
  out.reserve(len);
  for (int i = 0; i < len; i++) out += alphabet[random(n)];
  return out;
}

static int b64Index(char c) {
  if (c >= 'A' && c <= 'Z') return (int)(c - 'A');
  if (c >= 'a' && c <= 'z') return (int)(c - 'a') + 26;
  if (c >= '0' && c <= '9') return (int)(c - '0') + 52;
  if (c == '-') return 62;
  if (c == '_') return 63;
  return -1;
}

// Minimal base64url decoder (no padding '='). Returns decoded length, or -1 on error.
static int base64UrlDecode(const String& in, uint8_t* out, int outMax) {
  const int len = (int)in.length();
  int outLen = 0;
  int i = 0;

  while (i + 4 <= len) {
    const int a = b64Index(in[i]);
    const int b = b64Index(in[i + 1]);
    const int c = b64Index(in[i + 2]);
    const int d = b64Index(in[i + 3]);
    if (a < 0 || b < 0 || c < 0 || d < 0) return -1;
    const uint32_t n = ((uint32_t)a << 18) | ((uint32_t)b << 12) | ((uint32_t)c << 6) | (uint32_t)d;
    if (outLen + 3 > outMax) return -1;
    out[outLen++] = (uint8_t)((n >> 16) & 0xFF);
    out[outLen++] = (uint8_t)((n >> 8) & 0xFF);
    out[outLen++] = (uint8_t)(n & 0xFF);
    i += 4;
  }

  const int rem = len - i;
  if (rem == 0) return outLen;
  if (rem == 2) {
    const int a = b64Index(in[i]);
    const int b = b64Index(in[i + 1]);
    if (a < 0 || b < 0) return -1;
    const uint32_t n = ((uint32_t)a << 18) | ((uint32_t)b << 12);
    if (outLen + 1 > outMax) return -1;
    out[outLen++] = (uint8_t)((n >> 16) & 0xFF);
    return outLen;
  }
  if (rem == 3) {
    const int a = b64Index(in[i]);
    const int b = b64Index(in[i + 1]);
    const int c = b64Index(in[i + 2]);
    if (a < 0 || b < 0 || c < 0) return -1;
    const uint32_t n = ((uint32_t)a << 18) | ((uint32_t)b << 12) | ((uint32_t)c << 6);
    if (outLen + 2 > outMax) return -1;
    out[outLen++] = (uint8_t)((n >> 16) & 0xFF);
    out[outLen++] = (uint8_t)((n >> 8) & 0xFF);
    return outLen;
  }

  return -1;
}

// Minimal base64url encoder (no padding '=') for ACK Sentinel packets.
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

static bool parseClearWrapper(const String& line, int& templateId, String& payloadB64Url) {
  String s = line;
  s.trim();
  if (!s.startsWith("X1.")) return false;

  const int dotTpl = s.indexOf('.', 3);
  if (dotTpl < 0) return false;
  const String tplStr = s.substring(3, dotTpl);
  templateId = tplStr.toInt();

  const int dotMode = s.indexOf('.', dotTpl + 1);
  if (dotMode < 0) return false;
  const String mode = s.substring(dotTpl + 1, dotMode);
  if (mode != "C") return false;

  const int dotId = s.indexOf('.', dotMode + 1);
  if (dotId < 0) return false;
  const int dotPn = s.indexOf('.', dotId + 1);
  if (dotPn < 0) return false;

  payloadB64Url = s.substring(dotPn + 1);
  payloadB64Url.trim();
  return payloadB64Url.length() > 0;
}

static void applyOutState(uint8_t state) {
  for (int i = 0; i < 8; i++) {
    const bool on = (state & (1 << i)) != 0;
    digitalWrite(OUT_PINS[i], on ? HIGH : LOW);
  }
}

static void printSentinelAck(uint8_t outMask) {
  const uint64_t epochMs = EPOCH_BASE_MS + (uint64_t)millis();
  const uint32_t unixMinutes = (uint32_t)(epochMs / 60000ULL);

  const bool hasLabel = (LABEL && LABEL[0] != '\0');
  const int labelLen = hasLabel ? (int)min((size_t)SENTINEL_MAX_LABEL_BYTES, strlen(LABEL)) : 0;

  // Sentinel v1 payload with:
  // - sensorCount = 0
  // - hasIo=true (inMask=0, outMask=current)
  // - hasLabel optional
  const uint8_t sensorCount = 0;
  uint8_t flags = 0;
  flags |= 2; // hasIo
  if (hasLabel) flags |= 4;

  const int totalLen = 19 + 2 + (hasLabel ? (1 + labelLen) : 0); // no sensors
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

  buf[o++] = 0x00; // inMask
  buf[o++] = outMask;

  if (hasLabel) {
    buf[o++] = (uint8_t)labelLen;
    memcpy(buf + o, LABEL, (size_t)labelLen);
    o += labelLen;
  }

  const String payloadB64Url = base64UrlEncode(buf, o);
  const String pktId = makePacketId(8);
  const String wrapper = String("X1.11.C.") + pktId + ".1/1." + payloadB64Url;
  Serial.println(wrapper);
}

// ---- State ----

static uint8_t g_outState = 0x00;
static String g_lineBuf = "";

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 8; i++) {
    pinMode(OUT_PINS[i], OUTPUT);
    digitalWrite(OUT_PINS[i], LOW);
  }

  // Best-effort seed.
  randomSeed((uint32_t)millis());

  Serial.println("SentinelControlDemo ready. Paste X1.16.C... lines to control outputs.");
}

static void handleControlPacket(const String& payloadB64Url) {
  uint8_t bytes[160];
  const int n = base64UrlDecode(payloadB64Url, bytes, (int)sizeof(bytes));
  if (n < 0) {
    Serial.println("CONTROL decode error: bad base64url");
    return;
  }
  if (n < 10) {
    Serial.println("CONTROL decode error: payload too short");
    return;
  }

  int o = 0;
  const uint8_t ver = bytes[o++];
  if (ver != CONTROL_VERSION) {
    Serial.println("CONTROL decode error: unsupported version");
    return;
  }

  const uint32_t nodeId = readU32BE(bytes, o);
  const uint32_t unixMinutes = readU32BE(bytes, o);
  (void)unixMinutes; // available for freshness checks if you have real time

  const uint8_t flags = bytes[o++];
  const bool ackRequested = (flags & 1) != 0;
  const bool hasOut = (flags & 2) != 0;
  const bool hasLabel = (flags & 4) != 0;
  const bool hasControls = (flags & 8) != 0;

  if (nodeId != NODE_ID) {
    // Not for us.
    return;
  }

  uint8_t outOp = 0;
  uint8_t outMask = 0;
  if (hasOut) {
    if (o + 2 > n) {
      Serial.println("CONTROL decode error: out truncated");
      return;
    }
    outOp = bytes[o++];
    outMask = bytes[o++];
  }

  if (hasLabel) {
    if (o + 1 > n) {
      Serial.println("CONTROL decode error: label truncated");
      return;
    }
    const uint8_t ln = bytes[o++];
    if (o + ln > n) {
      Serial.println("CONTROL decode error: label truncated");
      return;
    }
    // Ignore label in this demo (you can use it for logging/routing).
    o += ln;
  }

  // Apply outMask operation first (if present).
  if (hasOut) {
    if (outOp == 0) g_outState = outMask;            // ABS
    else if (outOp == 1) g_outState = g_outState | outMask;   // SET bits
    else if (outOp == 2) g_outState = g_outState & (uint8_t)(~outMask); // CLEAR bits
    else if (outOp == 3) g_outState = g_outState ^ outMask;   // TOGGLE bits
  }

  // Apply control stack entries (optional).
  if (hasControls) {
    if (o + 1 > n) {
      Serial.println("CONTROL decode error: controls truncated");
      return;
    }
    const uint8_t count = bytes[o++];
    if (count > CONTROL_MAX_CONTROLS) {
      Serial.println("CONTROL decode error: too many controls");
      return;
    }
    for (uint8_t i = 0; i < count; i++) {
      if (o + 3 > n) {
        Serial.println("CONTROL decode error: controls truncated");
        return;
      }
      const uint8_t type = bytes[o++];
      const int16_t value = readI16BE(bytes, o);

      // Demo convention:
      // - type 0..7 => set that output bit absolute (value 0/1).
      if (type <= 7) {
        const bool on = value != 0;
        if (on) g_outState |= (uint8_t)(1 << type);
        else g_outState &= (uint8_t)(~(1 << type));
      }
    }
  }

  applyOutState(g_outState);

  Serial.print("CONTROL applied: outState=0x");
  if (g_outState < 16) Serial.print("0");
  Serial.println(g_outState, HEX);

  if (ackRequested) {
    printSentinelAck(g_outState);
  }
}

void loop() {
  while (Serial.available() > 0) {
    const char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      const String line = g_lineBuf;
      g_lineBuf = "";

      int tpl = 0;
      String payload = "";
      if (parseClearWrapper(line, tpl, payload)) {
        if (tpl == 16) {
          handleControlPacket(payload);
        }
      }
      continue;
    }
    if (g_lineBuf.length() < 220) g_lineBuf += c;
  }
}

