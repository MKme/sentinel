# Packet and firmware reference

## Distinct transports

| Format | Carrier | Purpose |
| --- | --- | --- |
| Readable Wi-Fi/motion/status text | Meshtastic TEXT_MESSAGE_APP | Presence cues; not T=11 |
| `X1.11.C.<packet-id>.1/1.<base64url>` | TEXT_MESSAGE_APP | Structured Sentinel report |
| `SMV1` + MAVLink frame | PRIVATE_APP port 256 | Telemetry-only UART/mesh bridge |
| `SMF1` + uint32 little-endian UTC seconds + MAVLink frame | PRIVATE_APP port 256 | Paired flight exchange |
| `SOI1` + compact observation | Local ToRadio API, port 256, self-addressed | Companion input consumed before radio routing |
| T=16 CONTROL | Standalone demo transport | Low-voltage output mask commands, not flight control |

Mode C is clear application encoding, not encryption or independent authentication. The configured private Meshtastic channel supplies its normal radio protection. Public/default keys are not private keys. Public-channel opt-in applies only to Presence sensor text alerts.

## Sentinel v1 payload

T=11 multibyte values are **big-endian**, unlike the little-endian SOI1 input.

| Offset | Field |
| --- | --- |
| 0 | Version 1 |
| 1 | Sensor reading count |
| 2–5 | UTC Unix minutes, uint32 |
| 6–9 | Latitude ×100000, signed int32 |
| 10–13 | Longitude ×100000, signed int32 |
| 14–17 | Node ID, uint32 |
| 18 | Flags |
| Following | Optional IO masks, optional length-prefixed UTF-8 label, then readings |

Each reading is a byte type and signed int16 value. Flags: bit 0 alert, bit 1 IO present, bit 2 label present, bit 3 no location, bit 4 stale location, bit 5 unknown UTC, bit 6 object observation. IO is input mask then output mask. No-location observations store INT32_MIN in both coordinates; unknown time uses zero plus its flag. Updated receivers must respect these semantics.

Position-only conversion has a label and **zero readings**, requires fresh valid fix/UTC and suppresses output when unavailable. Companion observations can still send with explicit missing/stale position or unknown UTC.

## Observation metadata

The ESP node ID, `OBS ` class label, alert flag and six readings identify an observation:

| Type | Value |
| --- | --- |
| 240 | Confidence percent ×100 |
| 241 | Count |
| 242 | GPS age seconds; -1 for no usable position |
| 243 | Observation age at mesh submission, seconds |
| 244 / 245 | ID high / low 16-bit halves, interpreted unsigned |

Age values saturate at 32767 seconds. Coordinates locate the sensor, not the detected object. Queue delay increments age and can turn fresh into stale. [Companion observations](Companion-Observations.md) defines SOI1 byte offsets, validation and queues.

## MAVLink modes

SMV1 supports HEARTBEAT (0), SYS_STATUS (1), GPS_RAW_INT (24), ATTITUDE (30) and GLOBAL_POSITION_INT (33). It preserves MAVLink 1/2 frame bytes, validates framing/CRC and does not authenticate signatures. Commands/missions cannot pass through SMV1. See [rates and deduplication](Presence-and-MAVLink.md).

SMF1 is paired unicast with zero relay hops, no radio ACK request and no fragmentation. Freshness, sender/target, message allowlist and duplicate checks apply. Controller signing enforcement and XTOC operator authority remain essential. Sentinel does not hold the signing key. See [flight setup](Flight-Controller-Link.md).

## Firmware families

- **Presence:** XIAO/Wio Meshtastic overlay, portal, scanner and optional telemetry/flight/observation paths.
- **SentinelClearDemo:** Arduino serializer and USB packet output.
- **SentinelControlDemo:** Arduino clear T=16 output-bank demo with T=11 acknowledgement.

Use [Build and flash](Build-and-Flash.md) or [Standalone examples](Standalone-Examples.md) as appropriate.
