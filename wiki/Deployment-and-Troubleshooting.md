# Deployment and troubleshooting

## Commission the installed node

Record board variant, firmware version, label, ID, role, channel, supply and placement. Confirm reception from the installed position, then test cold restart, power interruption and transport loss. Keep a manual check-in/fallback process.

For Wi-Fi, survey background activity and tune RSSI, channels and cooldowns. For motion, test nuisance cues while stationary. Bench-test aircraft links with propulsion disabled and verify controller loss-of-link behavior. For companion input, test fresh/stale/no GPS, unknown UTC and filtered classes.

Secure cables, antenna and enclosure. Measure supply during transmission and size for continuous Wi-Fi/Bluetooth/LoRa operation, not a sleeping sensor duty cycle.

## Find the failing stage

| Symptom | Check / recovery |
| --- | --- |
| No setup SSID | Restart and connect within boot window, default 120 s; check power and Presence firmware |
| Setup SSID disappears | Expected after save/timeout; restart to configure again |
| Save fails | Check ranges, channel/identity and conflicting bridge modes; portal stays open |
| Scanner does not start | Wait for portal close, disable normal Meshtastic Wi-Fi networking and inspect initialization errors |
| Nearby device not detected | Traffic must match channels, RSSI and randomized/local unicast filter; silent/nonqualifying devices are not detected |
| Local activity, no remote alert | Check deduplication/cooldown, output enable, local channel/key, region and TX; inspect mesh decisions |
| Too many alerts | Survey background, tighten RSSI, adjust memory/cooldowns and check motion mounting |
| Text arrives, no Sentinel row | Presence text is not T=11; use the correct report/import path |
| No MAVLink frames | Cross TX/RX, common ground, 3.3 V TTL, matching baud, selected direction and supported message IDs |
| SMV1 stale/delayed | One aggregate frame per interval; five types at 5 s can leave 25 s between GPS frames; reduce streams or tune within radio capacity |
| No T=11 position | Check source, peer, system/component, private output, TX, UTC and fresh GPS_RAW_INT 3D fix |
| Companion rejected | Enable SOI1 and output, close portal, use local API addressed to this node and validate fields |
| Observation dropped/delayed | Check repeated IDs/labels, queue bounds, 30 s expiry, flight priority and private-channel policy |
| NO GPS / STALE GPS | Check fix and source age; observations still send but position-only conversion suppresses invalid/stale input |
| USB busy | Close other clients; one process owns the port; use Bluetooth for radio configuration |
| Flight telemetry but no control | Check signing, pairing, channel, UTC, fresh heartbeat, operator scope/lease and control acquisition |
| Mission transfer fails | Check disarmed state, signing, authority and timeout; require readback |
| Reconnect does not repeat action | Expected; obtain fresh telemetry, reacquire control and review before new action |
| T=11 imported, no popup | Enable/save rule, check source/class/confidence/GPS filters and cooldown; SOA does not match plain Wi-Fi text |

## Useful diagnostics

USB **115200 baud** provides scanner health and mesh decisions, `mesh_submitted`, `serial_frames`, `dropped_or_replaced`, `position_reports`, `fresh_fix`, `UTC` and observation accepted/rejected/duplicate/dropped/sent/queued counters. Submission is not receipt: compare with the receiving radio and XTOC record.

![Ground connection and telemetry](../assets/presence/flight-ground.png)

*Current XTOC with simulated data. Check aircraft identity, freshness and imported reports separately.*

If delivery remains unreliable, simplify to two nearby nodes and one reporting function. Restore features one at a time while watching receipt and traffic. Keep video on its separate link; LoRa carries compact messages.
