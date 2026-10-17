# Configuration reference

Use the boot portal for Sentinel settings and the Meshtastic Bluetooth app for region, modem preset, keys and radio role. Save to start operation. Restart to reopen setup.

| Setting | Factory default | Accepted range / behavior |
| --- | --- | --- |
| Node label | SENTINEL-1 | 1–24 letters/digits/spaces/hyphen/underscore |
| Motion GPIO | 3 (D2) | 3 or 4 (D3) |
| Minimum RSSI | -82 dBm | -100 to -20 |
| Wi-Fi channels | 1–11 | First ≤ last, within 1–11 |
| Channel dwell | 250 ms | 100–2000 ms |
| Duplicate memory | 120 s | 10–3600 s |
| Wi-Fi cooldown | 30 s | 5–3600 s |
| Motion cooldown | 10 s | 2–3600 s |
| Combined window | 10 s | 1–300 s |
| Serial heartbeat | 60 s | 10–3600 s |
| Setup window | 120 s | 15–600 s |
| Verbose diagnostics | On | USB |
| Sensor mesh output | On | Subject to radio/channel checks |
| Output channel index | 0 | Local configured index 0–7 |
| Public/open channel opt-in | Off | Sensor text alerts only |
| Periodic mesh status | Off | Heartbeat cadence, minimum 60 s |
| Raw UART→mesh / mesh→UART | Off / Off | Telemetry-only directions |
| UART baud | 57600 | 9600, 19200, 38400, 57600, 115200; 8N1 |
| Raw bridge interval | 5000 ms | 1000–60000 ms across all types |
| Position source | Off | Serial MAVLink or Mesh MAVLink (SMV1) |
| Controller system/component | 1 / 1 | Match the source/controller |
| Position interval | 15 s | 5–3600 s |
| Aircraft link | Off | Raw bridge directions must be off |
| Aircraft downlink interval | 1000 ms | 250–2000 ms |
| Local companion input | Off | SOI1 through local API |
| Observation interval / same-label cooldown | 15 s | 5–3600 s |

![Configuration overview](../assets/presence/setup-desktop.png)

*Actual firmware renderer with simulated settings. Saved values can differ from factory defaults.*

Channel indices are local. Match UART baud and controller IDs separately. Aircraft portal pairing uses the ground ID in hex; helper `peerNode` uses the aircraft ID as decimal.

Settings versions 3/4/5 migrate to version 6, preserving sensor settings while new forwarding defaults off. Companion enable/interval are stored separately. Failed saves leave setup available. Do not reset configuration just to enable another feature.

## Select a mode

- Sensor text alerts: mesh output.
- Raw telemetry: [SMV1 bridge](Presence-and-MAVLink.md).
- Structured positions: position source, independent of raw forwarding.
- Supervised flight actions: [paired SMF1](Flight-Controller-Link.md), raw directions off.
- Classifier input: [SOI1](Companion-Observations.md), mesh output and private channel; position optional.

Normal relay behavior follows the Meshtastic role. Disabling sensor reports does not disable ordinary Meshtastic traffic or every separately configured reporting function.
