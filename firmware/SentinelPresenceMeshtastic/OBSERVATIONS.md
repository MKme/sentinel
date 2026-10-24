# Companion observation input

Send a small classifier result from your own Pi or other computer to Sentinel. The ESP transmits an ordinary T=11 Sentinel alert. Implement your own camera/model; the supplied example only encodes and submits observations.

## Wiring and setup

Connect the companion USB host to the ESP's USB data connection. On Pi Zero use its USB data/OTG port with a host adapter, not the power-only port. Provide a suitable regulated supply.

In the Sentinel boot portal enable **Accept local companion observations (SOI1)**, mesh output and a configured private mesh channel. Observation interval / repeated-label cooldown defaults to 15 seconds (5–3600). Input is off by default and saved in flash. No position source is required.

![Companion input controls in Sentinel setup](../../assets/presence/companion-setup.png)

*Actual firmware setup with simulated settings. Enable local input and private mesh output.*

USB retains the Meshtastic API. Local client API access is the trust boundary: protect USB access and any enabled local BLE/network client access. The hook consumes SOI submissions before routing; remote LoRa/MQTT packets cannot invoke it. Raw serial text is not accepted. The input accepts observations only.

## Input contract: SOI1

Use a Meshtastic `ToRadio.packet` decoded payload, `PRIVATE_APP` port **256**, addressed to **this ESP's own node number**, with ACK and response requests off. All numeric input fields are little-endian.

| Offset | Field | Meaning |
|---|---|---|
| 0–3 | `SOI1` | Four ASCII bytes identifying version 1 |
| 4–7 | observation ID, uint32 | Nonzero; new event = new ID |
| 8–9 | confidence, uint16 | 0–10000; 9200 means 92% |
| 10–11 | count, uint16 | 1–32767 |
| 12–13 | age in milliseconds, uint16 | 0–5000 since classifier observation |
| 14 | label length, uint8 | 1–24 |
| 15 onward | label | Uppercase A–Z, digits, underscore or hyphen; exact length |

Maximum input is 39 bytes. Do not send images, long text or arbitrary JSON. Frames with invalid fields, wrong destination, unsupported SOI version, disabled input or active setup portal are consumed/rejected, not relayed as observations. Other Meshtastic API traffic keeps its existing behavior.

The generic [sender example](examples/send_observation.py) uses the Meshtastic Python client on the companion. Call its `encode()` function from your classifier, or submit a manual test:

```sh
python send_observation.py --port /dev/ttyACM0 --label FIRE --confidence 92 --count 1
```

Use the device path assigned to your ESP; some USB adapters use `/dev/ttyUSB0`. Run only one client on that port. An observation ID is deduplicated for 60 seconds while retained in the 16-entry recent cache; same-label cooldown follows the configured interval. The four-entry pending queue drops overflow and expires reports after 30 seconds. This is bounded best-effort reporting, not guaranteed delivery or a radio airtime reservation.

## Location and clock

Local companion reports explicitly mark location unavailable, with INT32_MIN coordinates and GPS age -1. The node's UTC clock supplies observation time when valid; otherwise time is marked unknown. Queue delay increments observation age. No coordinates or epoch are invented.

## Mesh output: existing Sentinel v1 / T=11

Envelope: `X1.11.C.<packet-id>.1/1.<base64url>`, sent as Meshtastic `TEXT_MESSAGE_APP` on the configured private mesh-output channel. Output is always one bounded packet, with no radio ACK request. The regular T=11 node ID is the ESP's own identity; label is `OBS ` followed by the submitted class. Alert is set. The outer clear format is not application-level authentication; the configured mesh channel supplies its usual radio protection.

The existing 19-byte v1 header and optional label are retained. New flag bits: **3 no location**, **4 stale location**, **5 unknown UTC**, **6 object observation**. No-location coordinate storage uses signed INT32_MIN for both E5 fields; supporting receivers suppress the map position. Unknown UTC uses zero in the time field and bit 5. Older receivers require an update for these quality semantics; unchanged legacy reports remain unchanged.

Six sensor readings carry observation metadata (existing one-byte type + signed big-endian int16 value):

| Type | Meaning |
|---|---|
| 240 | Confidence percent ×100 |
| 241 | Count |
| 242 | GPS age seconds; -1 = no usable position |
| 243 | Observation age seconds at mesh submission |
| 244 | Observation ID high 16 bits, interpreted unsigned |
| 245 | Observation ID low 16 bits, interpreted unsigned |

Use the updated XTOC Sentinel view and mesh auto-import. XTOC XPASS **Sentinel Object Observation Alerts** installs five disabled SOA trigger templates and two disabled local plugin examples. Configure source, class, confidence and GPS filters in Triggers, enable and save. A source filter is not independent proof of identity. Review observations before consequential actions.

Diagnostic heartbeat counters report accepted, rejected, duplicate, dropped, sent and queued observations. “Sent” means submitted to the mesh stack, not received by the ground operator. Camera video remains on its separate link or local storage.


![Observation without GPS in XTOC](../../assets/presence/observation-no-gps.png)

*Current XTOC with simulated observations. Missing GPS is shown explicitly and has no map position.*

![SOA observation rule filters in XTOC](../../assets/presence/observation-trigger.png)

*Current XTOC with example settings. Configure the source, label and confidence, then enable and save the rule.*
