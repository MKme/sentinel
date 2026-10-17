# Presence, MAVLink and Sentinel locations

Start with [first-node setup](Build-the-First-Node.md). Use [Wi-Fi detection](Wi-Fi-Detection.md) for activity alerts and the separate [flight link](Flight-Controller-Link.md) for supervised aircraft actions.

![Boot setup](../assets/presence/setup-desktop.png)

*Firmware-rendered setup with simulated values. USB diagnostics use 115200 baud.*

## MAVLink serial / mesh bridge

The boot portal offers independent **Serial RX to mesh** and **Mesh to serial TX** switches. Both default off, including when migrating existing Presence settings. Sensor reporting remains independently configurable. Existing saved sensor settings are retained through the `config3`/`config4`/`config5` to `config6` migration; new flight forwarding defaults off.

![MAVLink telemetry forwarding options](../assets/presence/mavlink-bridge.png)

*Actual firmware page rendered with simulated settings; [full-size controls](../assets/presence/mavlink-bridge.png).*

| Connection | Sentinel header | Connect to |
| --- | --- | --- |
| Telemetry input | D7 / GPIO44 / UART RX | Telemetry device TX |
| Telemetry output | D6 / GPIO43 / UART TX | Telemetry device RX |
| Reference | GND | Device signal GND |

Use **3.3 V TTL**, not RS-232 or a 5 V signal. UART is 8N1; choose 9600, 19200, 38400, 57600 (default), or 115200 baud. Only the wires needed by the enabled direction are required. USB retains 115200-baud diagnostics. This dedicated target disables upstream GPS and generic Serial modules because they can compete for UART/pins; Bluetooth configuration remains available.

1. Configure the same private Meshtastic channel/key and compatible RF settings on both nodes. Select the correct **local channel index** in each bridge portal (indices can differ between nodes).
2. On the source node, enable **Serial RX to mesh** and set baud to match the telemetry source. On the receiver, enable **Mesh to serial TX** and match its connected consumer's baud. Enable both switches for two-way telemetry exchange.
3. Choose a mesh send interval from 1000–60000 ms; default 5000 ms. Only one frame is submitted per interval across all types. The latest frame of each type replaces older pending samples, with round-robin selection to avoid starving lower-rate types. Pending observations older than 30 seconds are discarded. Use conservative rates for the configured LoRa preset and network capacity.
4. Save and allow the boot portal to close. USB prints UART configuration and periodic `mesh_submitted`, `serial_frames`, and `dropped_or_replaced` counters. Submission does not prove delivery.
5. Bench-test two nodes with a known telemetry source and consumer. Check received values and frame bytes, disabled directions, mismatched channels, disconnected UART, reconnect, and cold restart before installation.

Supported telemetry (MAVLink IDs): HEARTBEAT (0), SYS_STATUS (1), GPS_RAW_INT (24), ATTITUDE (30), GLOBAL_POSITION_INT (33). MAVLink 1 and 2 are accepted, including MAVLink 2 truncated payloads and signed frames. The bridge checks framing, supported length, incompatibility flags and CRC-extra. It does not hold signing secrets or authenticate signatures; the receiving MAVLink endpoint must enforce its own signature/replay policy. Original frame bytes are preserved, so this bridge does not regenerate signatures or timestamps. Delayed signed messages may be rejected by that endpoint.

Transport: Meshtastic `PRIVATE_APP` port 256, payload ASCII `SMV1` followed by exactly one complete supported MAVLink frame. No text/base64 wrapping or fragmentation. The largest supported frame is 77 bytes (signed GPS_RAW_INT), fitting one mesh payload. Both ends must implement this format. Unrelated private-port payloads and text messages are ignored. Broadcast frames on the chosen channel can reach any enabled bridge sharing its key; use a dedicated channel for each intended group. Public/default-key, disabled or unconfigured channels are blocked. RF TX must also be enabled for serial-to-mesh forwarding.

CRC-invalid, unsupported and flight-command frames are discarded. The serial parser has bounded storage and a 250 ms partial-frame timeout. Mesh-to-UART uses a bounded queue and nonblocking writes; overflow is dropped. A bounded 30-second exact-frame cache suppresses duplicate and immediate echo forwarding. This is best-effort telemetry, with no delivery guarantee, command path, mission/parameter transfer, or replacement for an aircraft's control link.

## MAVLink locations as Sentinel packets

Enable **XTOC Sentinel locations** to publish position-only `T=11` reports. Position conversion and both raw MAVLink forwarding directions default off. They are independent of sensor mesh alerts, whose output switch defaults on but remains subject to radio/channel checks.

![Sentinel location source, MAVLink identity and reporting interval](../assets/presence/sentinel-location.png)

*Actual firmware setup renderer with simulated settings. Choose Serial UART or Mesh SMV1 input; Off is the factory default.*

1. Set the node label, for example `WRAITH-01` or `GROUND-01`. Select a private **Meshtastic output** channel. The location converter blocks public/default-key channels even if public sensor alerts are enabled.
2. Choose **Serial MAVLink** for an attached telemetry source. Connect its 3.3 V TX to D7/GPIO44 and common ground; match the bridge baud setting. Neither raw bridge switch needs to be on for conversion.
3. Alternatively choose **Mesh MAVLink (SMV1)**, select the bridge input channel and enter the originating Meshtastic node's eight hexadecimal digits without `!`. The upstream node must send supported `SMV1` MAVLink frames. Only this selected sender is converted; UART output can remain off. The raw bridge sends one frame per aggregate interval: five active types at the default five-second interval can leave 25 seconds between GPS frames. Reduce the source stream to the needed types or select a suitable faster bridge interval within the mesh capacity, and verify `fresh_fix` stays current.
4. Match MAVLink system/component IDs (both default 1). Set the report interval, default 15 seconds, range 5–3600 seconds. Enable conversion at one endpoint per source to avoid duplicate reports.
5. Save and let the portal close. Supply a continuing `GPS_RAW_INT` stream with a 3D-or-better fix. Both position and fix must be less than 15 seconds old. `GLOBAL_POSITION_INT` can update coordinates only while that GPS fix is fresh. Unknown UTC, invalid coordinates, lost fix or stale telemetry suppress reporting. UTC comes from the Meshtastic clock or a valid GPS epoch timestamp; boot-relative time is not UTC.
6. Connect XTOC to the receiving mesh and enable incoming packet auto-import. Reports use the existing Sentinel decoder and appear as Sentinel data. Verify the label, stable node ID and location before deployment. Use the label to identify the carried platform; this packet does not invent a drone/entity-type field.

The serial source uses this node's Meshtastic ID; mesh conversion preserves the selected originating node's ID. Each report is `X1.11.C.<packet-id>.1/1.<payload>` on `TEXT_MESSAGE_APP`, with Sentinel v1 location, UTC minute, label and **zero sensor readings**. Mode C is application-level clear encoding, carried over the configured private Meshtastic channel; it is not X Suite mode S. Incoming text is not fed back into the MAVLink converter.

USB diagnostics at 115200 baud include `position_submitted`, `position_reports`, `fresh_fix` and `UTC`. A submission is not proof of delivery. Verify reception and XTOC import on your two-node setup.
