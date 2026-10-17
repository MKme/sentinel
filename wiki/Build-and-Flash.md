# Build and flash Presence firmware

The supported target is **XIAO ESP32-S3 + Wio-SX1262 B2B**, environment `sentinel-xiao-s3`. Other radio wiring needs a matching board variant.

## Prepare build inputs

Use the [firmware build scripts](../firmware/SentinelPresenceMeshtastic/README.md), pinned Meshtastic revision `47e129f4bdec33a5012851aa3546073aa87d9254`, PlatformIO Espressif32 6.9.0 and Arduino ESP32 2.0.17. Preparation expects an existing compatible upstream checkout and library cache; it does not automatically obtain missing dependencies.

Read `prepare.py` for installed-source/cache defaults and `--source` / `--platform` options for another location. Keep upstream source and licenses. The generated `.build` tree is output; the maintained implementation is the overlay and scripts.

From the Sentinel repository root:

```powershell
.\firmware\SentinelPresenceMeshtastic\build.ps1
```

The script applies the overlay and local observation ingress hook, builds the dedicated environment and exports available binary/ELF/JSON artifacts plus SHA-256 records to `firmware/SentinelPresenceMeshtastic/artifacts/`.

## Flash the correct image

1. Back up existing radio configuration and identify the exact connected board/serial port.
2. Select the exported Meshtastic manifest for this target. Use its files and addresses; factory and application images are not interchangeable.
3. Include the filesystem image when indicated. Do not guess an offset or erase configuration as routine troubleshooting.
4. Flash through your configured ESP32/Meshtastic tool, then restart with the correct antenna and stable supply.
5. At USB 115200 check the Sentinel boot identification. Open its portal and complete [first-node setup](Build-the-First-Node.md).

![Presence boot portal](../assets/presence/setup-desktop.png)

*Firmware-rendered screen with simulated settings. This portal distinguishes Presence from generic Meshtastic firmware.*

Confirm saved settings survive restart and an alert arrives at a second node. For companion use, also verify the SOI1 sender and received T=11 report; serial output alone is not end-to-end delivery.

Presence includes GPL-3.0 Meshtastic code; preserve corresponding source and dependency notices when distributing binaries. Standalone examples retain their repository license.
