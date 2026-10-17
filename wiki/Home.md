# Sentinel field guide

Sentinel turns a XIAO ESP32-S3 and Wio-SX1262 into a passive Wi-Fi activity detector and LoRa reporting node. Add motion sensing, MAVLink telemetry or observations from your own companion computer. Deploy it on the ground or carry it aboard Wraith XDRONE or another suitable platform.

## Choose your task

| Task | Guide | Result |
| --- | --- | --- |
| Detect nearby Wi-Fi activity | [Wi-Fi detection](Wi-Fi-Detection.md) | Readable mesh alert with label, RSSI and Wi-Fi channel |
| Combine motion and Wi-Fi | [Build the first node](Build-the-First-Node.md) | Motion, Wi-Fi or combined alerts |
| Return position to XTOC | [Presence and MAVLink](Presence-and-MAVLink.md) | Structured Sentinel T=11 location |
| Exchange UART telemetry | [Presence and MAVLink](Presence-and-MAVLink.md) | Supported MAVLink frames over SMV1 |
| Connect a flight controller | [Flight-controller link](Flight-Controller-Link.md) | Paired telemetry, supervised actions and missions |
| Report a Pi classifier result | [Companion observations](Companion-Observations.md) | Class, confidence and fresh/stale/missing position |
| Learn the packet format | [Standalone examples](Standalone-Examples.md) | Serial T=11 and a separate T=16 output-bank demo |

Wi-Fi detection needs no Pi, camera or classifier. Presence and the standalone examples are different programs with different wiring and transports. Choose firmware before assembling a node.

## Build and operate

1. Select [parts](Parts-and-Kits.md) and check [wiring](Wiring-Reference.md).
2. [Build and flash](Build-and-Flash.md), then [configure the first node](Build-the-First-Node.md).
3. Tune [Wi-Fi detection](Wi-Fi-Detection.md) using the [configuration reference](Configuration-Reference.md).
4. Follow [XTOC/XCOM integration](XTOC-XCOM-Integration.md) for your report format.
5. Complete [deployment checks](Deployment-and-Troubleshooting.md).

![Sentinel setup portal](../assets/presence/setup-desktop.png)

*Firmware-rendered setup with example values and simulated channel state. Save, then verify reception at a second radio.*

## References

- [Packet and firmware reference](Packet-and-Firmware.md)
- [Firmware source](../firmware/SentinelPresenceMeshtastic/README.md)
- [Training Hub](https://store.mkme.org/training/): Disaster Operations, MILSIM Operations, technical specifications, flight-link and companion-observation HTML/PDF guides
- [Sentinel project](https://store.mkme.org/sentinel/)
- [XDRONE featuring Wraith](https://store.mkme.org/xdrone/)

Enter your email once in the Training Hub to unlock its library in that browser.
