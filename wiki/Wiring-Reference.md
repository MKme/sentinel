# Wiring reference

## XIAO ESP32-S3 + Wio-SX1262 Presence

| Function | Header / GPIO | Connect to |
| --- | --- | --- |
| Motion | D2 / GPIO3 default | RCWL OUT |
| Alternate motion | D3 / GPIO4, portal-selectable | RCWL OUT instead of D2 |
| Sensor power | Suitable 5 V supply | RCWL VIN |
| Signal reference | GND | Sensor/controller signal ground |
| MAVLink receive | D7 / GPIO44 / UART RX | Source/controller TX |
| MAVLink transmit | D6 / GPIO43 / UART TX | Consumer/controller RX |
| Companion | USB data | Pi/computer USB host |

Use **3.3 V TTL**, not RS-232 or 5 V logic. UART is 8N1 at 9600, 19200, 38400, **57600 default**, or 115200. USB diagnostics/API use 115200 independently. Connect only the UART directions needed.

GPIO5/6 are reserved for I²C; GPIO7/8/9 and 38–42 serve the radio. GPIO43/44 carry telemetry. Upstream GPS and generic Serial modules are disabled on this target to prevent conflicts. Header D2 is not GPIO2.

Check labels and orientation with power disconnected. Fit the correct antenna before transmitting and secure wiring.

## Pi plus flight controller

```text
Pi USB host/data ------- USB cable -------- Sentinel USB
Controller TX ---------------------------- Sentinel RX D7 / GPIO44
Controller RX ---------------------------- Sentinel TX D6 / GPIO43
Controller GND --------------------------- Sentinel GND
Sentinel radio -------- LoRa ------------- Ground radio -- USB -- PC
```

The Pi submits SOI1 through the local Meshtastic API; the UART carries MAVLink. Never tie two TX outputs together. Use one USB client per radio; close Serial Monitor before starting the companion or bridge.

![Aircraft pairing and UART controls](../assets/presence/flight-link.png)

*Firmware-rendered settings with simulated values. Wiring alone does not enable forwarding.*

## Standalone examples

The separate sketch uses GPIO4 and RCWL type 1. For PIR, change the sensor convention to type 2 and check its electrical requirements. Custom DevKit contacts, temperature and ADC circuits need their own code; they are not Presence assignments.

GPIOs cannot drive coils or motors directly. The standalone output demo needs engineered low-voltage drivers, protection, fusing and safe defaults. It is unrelated to MAVLink flight control.
