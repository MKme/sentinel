# Presence, MAVLink and XTOC location setup

Sentinel supports a stationary ground node or an independently powered sensing/telemetry payload carried on Wraith XDRONE. Keep aircraft control on its established control link.

![Wraith XDRONE carrier concept](../assets/presence/sentinel-field-header.png)

## Open local setup

On the XIAO ESP32-S3/Wio-SX1262 target, join `SENTINEL-xxxx` with password `sentinel123` during the first 120 seconds and open `http://192.168.4.1`. Configure RF region, keys and role with the Meshtastic Bluetooth app first.

![Boot portal with simulated node and channel settings](../assets/presence/setup-desktop.png)

*Current firmware renderer, simulated settings; the controls are not a photograph of a live radio.*

## MAVLink locations as Sentinel packets

Enable **XTOC Sentinel locations** to publish position-only `T=11` reports. This is independent of sensor alerts and the raw MAVLink forwarding switches; all three functions default off.

![Sentinel location source, MAVLink identity and reporting interval](../assets/presence/sentinel-location.png)

*Actual firmware setup renderer with simulated settings. Choose Serial UART or Mesh SMV1 input; Off is the factory default.*

1. Set the node label, for example `WRAITH-01` or `GROUND-01`. Select a private **Meshtastic output** channel. The location converter blocks public/default-key channels even if public sensor alerts are enabled.
2. Choose **Serial MAVLink** for an attached telemetry source. Connect its 3.3 V TX to D7/GPIO44 and common ground; match the bridge baud setting. Neither raw bridge switch needs to be on for conversion.
3. Alternatively choose **Mesh MAVLink (SMV1)**, select the bridge input channel and enter the originating Meshtastic node's eight hexadecimal digits without `!`. The upstream node must send supported `SMV1` MAVLink frames. Only this selected sender is converted; UART output can remain off. The raw bridge sends one frame per aggregate interval: five active types at the default five-second interval can leave 25 seconds between GPS frames. Reduce the source stream to the needed types or select a suitable faster bridge interval within the mesh capacity, and verify `fresh_fix` stays current.
4. Match MAVLink system/component IDs (both default 1). Set the report interval, default 15 seconds, range 5–3600 seconds. Enable conversion at one endpoint per source to avoid duplicate reports.
5. Save and let the portal close. Supply a continuing `GPS_RAW_INT` stream with a 3D-or-better fix. Both position and fix must be less than 15 seconds old. `GLOBAL_POSITION_INT` can update coordinates only while that GPS fix is fresh. Unknown UTC, invalid coordinates, lost fix or stale telemetry suppress reporting. UTC comes from the Meshtastic clock or a valid GPS epoch timestamp; boot-relative time is not UTC.
6. Connect XTOC to the receiving mesh and enable incoming packet auto-import. Reports use the existing Sentinel decoder and appear as Sentinel data. Verify the label, stable node ID and location before deployment. Use the label to identify the carried platform; this packet does not invent a drone/entity-type field.

The serial source uses this node's Meshtastic ID; mesh conversion preserves the selected originating node's ID. Each report is `X1.11.C.<packet-id>.1/1.<payload>` on `TEXT_MESSAGE_APP`, with Sentinel v1 location, UTC minute, label and **zero sensor readings**. Mode C is application-level clear encoding, carried over the configured private Meshtastic channel; it is not X Suite mode S. Incoming text is not fed back into the MAVLink converter.

USB diagnostics at 115200 baud include `position_submitted`, `position_reports`, `fresh_fix` and `UTC`. A submission is not proof of delivery. Native extraction, actual bridge-path tests and the actual XTOC codec contract pass; physical UART, over-air reception and live XTOC import still require a two-node bench test.

## Save and verify

![Saved configuration confirmation](../assets/presence/saved.png)

*Save persists settings and starts operation. This source-rendered confirmation is not an over-air reception result.*

For raw serial-to-mesh or mesh-to-serial telemetry, use the independent switches below. Connect UART TX D6/GPIO43 to the consumer RX only when needed, RX D7/GPIO44 to source TX, and common ground. Use 3.3 V TTL, 8N1, matching baud (57600 default). USB diagnostics remain 115200.

![Independent MAVLink bridge directions and rate](../assets/presence/mavlink-bridge.png)

*Both raw forwarding directions default off. Supported types: HEARTBEAT, SYS_STATUS, GPS_RAW_INT, ATTITUDE and GLOBAL_POSITION_INT; no command or mission transfer.*

See the [firmware reference](../firmware/SentinelPresenceMeshtastic/README.md) for build steps, parser behavior, channel rules and the bench-test checklist.
