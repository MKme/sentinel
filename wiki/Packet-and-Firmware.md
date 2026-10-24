# Packet and firmware reference

| Source | Output |
|---|---|
| SentinelClearDemo | Clear T=11 Sentinel v1 with configured stationary position |
| SentinelControlDemo | Demonstration low-voltage T=16 control and acknowledgement |
| Presence motion/Wi-Fi | Readable Meshtastic sensor alerts |
| Presence companion input | SOI1 input through local Meshtastic API; T=11 observation output |

See [companion binary contract](../firmware/SentinelPresenceMeshtastic/OBSERVATIONS.md) for exact observation fields and quality flags. Motion and Wi-Fi text alerts remain distinct from T=11 packets.

Build **sentinel-ground-xiao-s3** using the [local firmware scripts](../firmware/SentinelPresenceMeshtastic/README.md). Keep source and GPL notices with distributed firmware.
