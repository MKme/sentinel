# Sentinel Ground — Presence firmware

Standalone local sensing for **XIAO ESP32-S3 + Wio-SX1262 B2B**: RCWL-0516 motion, passive Wi-Fi activity, Meshtastic alerts, optional status reports and local companion observations. No internet is needed for setup or use.

## Wire and configure

1. Connect RCWL VIN to the appropriate 5 V supply, GND to GND and OUT to **D2 / GPIO3**. GPIO4 is the alternate configurable input. Use a suitable regulated supply and attach the correct radio antenna.
2. Configure region, private channel key and radio role using the Meshtastic app over Bluetooth. Disable Meshtastic Wi-Fi networking; the passive scanner owns Wi-Fi.
3. Restart and join **SENTINEL-xxxx** during the 120-second setup window. The factory setup password is `sentinel123`. Open `http://192.168.4.1`.

![Sentinel local setup](../../assets/presence/setup-desktop.png)

*Current firmware renderer with example values and simulated channel state. Set label, motion GPIO and detection thresholds.*

4. Select the configured output channel and enable detections. Optional periodic sensor status has a minimum 60-second interval. Public/open channels require explicit opt-in; companion observations always require a private channel.

![Mesh output settings](../../assets/presence/mesh-output.png)

*Select an existing channel. Configure its key through the Meshtastic app.*

5. Select **Save and start detection**, then inspect USB diagnostics at **115200 baud**. Restart to reopen setup. Settings persist in flash.

![Configuration saved](../../assets/presence/saved.png)

*The access point closes and sensing starts. Confirm the alert arrives at your receiving node.*

Wi-Fi observations indicate radio activity, not a person count or verified identity. Mount motion sensors stationary and test their warm-up and environment. Sentinel is an awareness aid; do not connect sensor outputs to hazardous loads.

## Optional current/voltage sensing

The optional **Sentinel Ground Power** target adds calibrated current/voltage sensing while retaining motion detection, the Wi-Fi scanner and private Meshtastic output. Edit `overlay/src/modules/optional/SentinelPresence/SentinelPowerConfig.h`, then build:

```powershell
.\firmware\SentinelPresenceMeshtastic\build.ps1 -Power
```

The target is `sentinel-ground-power-xiao-s3`; its output is `artifacts-ground-power/`. Sensing defaults off in the normal Ground build. In the Power target a zero calibration gain keeps acquisition disabled until configured. Existing saved Ground settings remain compatible. The configured mesh output, channel, radio region and transmit setting apply; Power packets require a private channel.

XIAO ESP32S3 GPIO1/D0 is the conditioned current ADC input and GPIO2/D1 is the optional conditioned voltage input. Keep existing motion GPIO3/4 and radio wiring intact. Use the sensor manufacturer's isolated low-voltage front end and have mains work performed by a qualified installer. Match a current-output clamp's burden or a voltage-output clamp's designed interface, and clamp around one conductor. Calibrate zero and several known loads against a reference meter. Configure A per ADC volt, DC zero-current bias and voltage gain as appropriate for the exact board.

AC mode reports RMS current. A measured RMS voltage adds V and VA; configured nominal voltage instead labels a VA estimate. DC mode can report signed A, V and calculated DC watts. Clipped or invalid sampling windows are omitted. Acquisition is cooperative, taking a sample per short scheduler pass over a 200 ms window; the default reporting interval is 60 seconds.

The MIT `XtocPowerPacket.h` codec emits the existing **X1 template 11 SENTINEL Power profile 1**, with types80–95. Node identity and circuit channel stay stable; the mesh packet's unique ID identifies each sample. Unknown source UTC stays explicit and location is unavailable. XTOC/XCOM use their existing Communications transport/import, then display the scaled sensor fields. In XTOC Power turn examples off, choose the received circuit's **Map readings**, select source/load/storage and bind the supplied quantities. Forward the original packet over other configured transports when needed.

The complete field contract, ESPHome/Arduino LAN alternatives, calibration and transport instructions are maintained in [XTOC edge/power](https://github.com/MKme/XTOC/tree/main/edge/power). `npm test` includes native tests of the actual optional driver, timing, clipping and private-radio gates. Build and commission on your own sensor/front end before deployment.

## Companion observations

Use the local USB Meshtastic API and the [SOI1 contract](OBSERVATIONS.md). Input is disabled by default. Reports contain label, confidence, count and age; location is explicitly unavailable. The classifier and images stay on your companion computer.

## Build locally

Use the existing pinned Meshtastic checkout (`47e129f4bdec33a5012851aa3546073aa87d9254`), PlatformIO Espressif32 6.9.0, Arduino ESP32 2.0.17 and installed library cache. `prepare.py --source <path> --platform <path>` accepts alternate local paths. No dependencies are downloaded by these scripts.

```powershell
.\firmware\SentinelPresenceMeshtastic\build.ps1
```

Target: **sentinel-ground-xiao-s3**. Output: **artifacts-ground/** with SHA-256 records. Build staging: `.build-ground/meshtastic`. Use only this target's new output when flashing Sentinel Ground; older `artifacts/` files may contain previous firmware. Follow [build and flash](../../wiki/Build-and-Flash.md) and back up radio settings first.

The overlay is the maintained source. `tests/run_tests.py` runs local native regressions. Preserve GPL-3.0 notices and corresponding source when distributing Meshtastic-based firmware.
