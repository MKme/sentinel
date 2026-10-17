# Passive Wi-Fi detection

Sentinel listens for Wi-Fi activity, filters it and sends a readable LoRa alert from a named observation point. This core function needs no Pi, camera, internet or association with the detected device.

```text
Wi-Fi traffic → scan → RSSI/address filter → duplicate memory
             → cooldown / optional motion correlation → mesh alert
```

The scanner accepts management/data traffic with randomized or locally administered unicast transmitter addresses. It does not inventory every access point or device. A bounded 64-entry RAM cache holds temporary hashes. Reports omit MAC addresses, SSIDs and packet contents.

## Tune the detector

1. Complete [first-node setup](Build-the-First-Node.md).
2. Set **Minimum RSSI (dBm)**. A less-negative threshold, such as -65 instead of -82, rejects weaker signals. Test at the mounting location.
3. Choose first/last channels within 1–11 and dwell time. Wider scanning revisits each channel less often.
4. Set **Unique-device memory seconds** and Wi-Fi cooldown. Repeated observations refresh remembered time. The cache is bounded and randomized addresses can create new observations; it is not a permanent inventory.
5. If using RCWL, select its GPIO, motion cooldown and combined motion/Wi-Fi window. Use motion while stationary or landed.
6. Enable mesh output, select a configured channel and save. Confirm the alert at another node.

![Wi-Fi configuration controls](../assets/presence/setup-desktop.png)

*Firmware-rendered setup with simulated values. Tune signal threshold, channels and event timing separately.*

## Read the result

Illustrative mesh messages:

```text
SITE-01: WIFI DETECTED RSSI -58 WIFI-CH 6
SITE-01: MOTION+WIFI DETECTED
SITE-01: ONLINE WIFI 12 MOTION 0 SCAN OK
```

`WIFI-CH` is the detected Wi-Fi channel, not the Meshtastic output index. `WIFI` status is cumulative deduplicated observations since boot, not current occupancy. Periodic mesh status is off by default and no faster than once per 60 seconds.

**Presence Wi-Fi/motion alerts are text, not T=11 packets.** Receive them as mesh messages. Separate location and companion modes emit T=11 for Sentinel. An SOA object-observation rule does not match plain Wi-Fi text.

## Diagnose and deploy

At USB **115200 baud**, inspect RSSI/channel, new observations, motion transitions, scanner health, queue drops and mesh decisions. Verbose mode prints queued qualifying observations. `mesh_submitted` means stack submission, not remote reception.

If nothing appears, wait for portal closure, disable normal Meshtastic Wi-Fi networking and check scanner startup, channels and threshold. If activity appears locally but not remotely, check deduplication/cooldown, output enable, channel/key, region and TX. Public/default-key sensor reporting requires opt-in; other transport modes require private channels.

Use an equipment site, access route or observation point to establish background activity before tuning alerts. Silent devices can remain undetected; signals may originate beyond the mounting area. RSSI is not exact distance and detections do not identify or count people. Keep a separate check-in process for silence.

See [defaults](Configuration-Reference.md) and [troubleshooting](Deployment-and-Troubleshooting.md).
