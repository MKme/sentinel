# Standalone Arduino examples

These sketches teach the packet format on an ESP32. They are separate from Presence firmware and do not provide passive Wi-Fi scanning, its portal, SMV1/SMF1 or SOI1.

## Print a T=11 report

1. Open [SentinelClearDemo.ino](../arduino/SentinelClearDemo/SentinelClearDemo.ino).
2. Check the exact board and motion sensor electrical requirements. The sketch uses GPIO4 and RCWL type 1. For PIR, adapt the sensor convention to type 2 as well as wiring.
3. Set `NODE_ID`, `LABEL` and valid installation coordinates. Replace the example epoch-base approach with a credible clock or managed time-setting method for operational use.
4. Compile/upload for the selected board. PlatformIO users can build with `pio run -d arduino/SentinelClearDemo` from the repository root.
5. Open Serial Monitor at 115200 baud. Copy a complete `X1.11.C...` line into **Comms → Import/Export** in XTOC/XCOM.
6. Check identity, timestamp, coordinates and sensor value. Trigger the input and confirm the changed report.

The sketch prints packets; automatic remote delivery needs a transport you supply. Fixed example coordinates are not GPS measurements.

## Low-voltage output demonstration

[SentinelControlDemo.ino](../arduino/SentinelControlDemo/SentinelControlDemo.ino) parses clear T=16 commands for a targeted node, applies ABS/SET/CLEAR/TOGGLE to an 8-bit output bank and returns a T=11 acknowledgement. Build with `pio run -d arduino/SentinelControlDemo`.

Use engineered low-voltage drivers and safe defaults. Clear packets alone are not authorization. Add authenticated transport, freshness/replay checks and recovery before adapting this example to real outputs. Confirm returned `outMask`; a sent command does not prove a physical load changed.

Use [Presence setup](Build-the-First-Node.md) for integrated detection and [Flight-controller link](Flight-Controller-Link.md) for MAVLink aircraft operations.
