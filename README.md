# Sentinel

![Sentinel open-source field sensor kit](assets/sentinel-hero.png)

Sentinel is the open-source remote sensor project for the MKME X Suite. It turns inexpensive Arduino-compatible boards and off-the-shelf sensors into named, mapped field nodes that report motion, contact state, temperature, battery voltage, and custom readings to XTOC or XCOM using the compact `T=11 SENTINEL` packet.

Repository: <https://github.com/MKme/sentinel>

> Sentinel is an awareness aid, not a certified alarm, security, fire, medical, or life-safety system. Do not directly control mains, machinery, weapons, pyrotechnics, vehicles, or hazardous loads.

## What you can build

- gate, door, cabinet, or equipment-case contact node
- PIR or RCWL motion cue
- shelter, freezer, generator-compartment, or outdoor temperature node
- battery-health and remote-power heartbeat
- combined multi-sensor node with a stable name and map position
- optional low-voltage output demonstrator using `T=16 CONTROL` and acknowledgement

![Sentinel direct-sensor architecture](assets/sentinel-assembly.png)

## Fastest first build

Start with an ESP32 DevKit and one HC-SR501 PIR. The supplied sketch needs no third-party Arduino library.

| Part | Connect to ESP32 |
| --- | --- |
| HC-SR501 `VCC` | board `5V/VIN` when the module requires 5 V |
| HC-SR501 `GND` | `GND` |
| HC-SR501 `OUT` | GPIO `4` |

1. Install Arduino IDE and the Espressif ESP32 board package.
2. Open [`arduino/SentinelClearDemo/SentinelClearDemo.ino`](arduino/SentinelClearDemo/SentinelClearDemo.ino).
3. Set `NODE_ID`, `LABEL`, `LAT`, and `LON` near the top.
4. Select the exact ESP32 board and serial port.
5. Upload, open Serial Monitor at `115200`, and wait for an `X1.11.C...` line.
6. In XTOC or XCOM, open `Comms -> Import/Export`, paste the complete line, and import it.
7. Confirm the report appears in `Sentinel` and on the Tactical Map Sentinel layer.
8. Walk-test the PIR, then test stillness, sunlight changes, warm-up, and the planned mounting position.

## Code included

| Sketch | Purpose |
| --- | --- |
| [`SentinelClearDemo.ino`](arduino/SentinelClearDemo/SentinelClearDemo.ino) | reads a motion input, builds the binary Sentinel v1 payload, base64url-encodes it, and prints a complete clear `T=11` wrapper |
| [`SentinelControlDemo.ino`](arduino/SentinelControlDemo/SentinelControlDemo.ino) | decodes clear `T=16 CONTROL`, applies an 8-bit low-voltage output bank, and emits a `T=11` acknowledgement with the resulting `outMask` |

The demos are intentionally self-contained so builders can see the entire packet implementation without hidden libraries. Copy a demo before adapting it for a deployed node.

PlatformIO users can build either self-contained project directly:

```powershell
pio run -d arduino/SentinelClearDemo
pio run -d arduino/SentinelControlDemo
```

## Suggested parts

The links below are Amazon search links carrying affiliate tag `wbca-20`; availability and sellers change. Match voltage, pinout, dimensions, and chemistry before ordering.

### Core starter kit

- [ESP32 DevKit V1 boards](https://www.amazon.com/s?k=ESP32+DevKit+V1&tag=wbca-20)
- [HC-SR501 PIR motion sensors](https://www.amazon.com/s?k=HC-SR501+PIR+motion+sensor&tag=wbca-20)
- [magnetic reed contact switches](https://www.amazon.com/s?k=magnetic+reed+contact+switch+wired&tag=wbca-20)
- [waterproof DS18B20 temperature probes](https://www.amazon.com/s?k=waterproof+DS18B20+temperature+sensor&tag=wbca-20)
- [solderless breadboard and jumper kit](https://www.amazon.com/s?k=electronics+breadboard+jumper+wire+kit&tag=wbca-20)
- [USB data cables for ESP32](https://www.amazon.com/s?k=USB+data+cable+ESP32&tag=wbca-20)

### Deployable enclosure and power

- [weather-resistant project enclosures](https://www.amazon.com/s?k=waterproof+electrical+project+enclosure&tag=wbca-20)
- [IP-rated cable glands](https://www.amazon.com/s?k=waterproof+cable+glands+assortment&tag=wbca-20)
- [protected 18650 battery holders with switch](https://www.amazon.com/s?k=protected+18650+battery+holder+with+switch&tag=wbca-20)
- [quality protected 18650 cells](https://www.amazon.com/s?k=protected+18650+rechargeable+battery&tag=wbca-20)
- [5 V USB power banks](https://www.amazon.com/s?k=USB+power+bank+5V&tag=wbca-20)
- [inline low-voltage fuse holders](https://www.amazon.com/s?k=inline+low+voltage+fuse+holder&tag=wbca-20)
- [heat-shrink tubing kit](https://www.amazon.com/s?k=heat+shrink+tubing+kit&tag=wbca-20)
- [digital multimeter](https://www.amazon.com/s?k=digital+multimeter+electronics&tag=wbca-20)

Do not charge loose lithium cells with improvised wiring. Use protected cells, a chemistry-compatible certified charger/power module, fusing, and an enclosure that prevents shorts and crushing.

## Sensor conventions

| Type | Meaning | Stored value |
| --- | --- | --- |
| `1` | RCWL-0516 motion | `0` or `1` |
| `2` | PIR motion | `0` or `1` |
| `6` | temperature | degrees Celsius × 10 |
| `7` | battery | millivolts |

`T=11` can also carry `inMask`, `outMask`, a label, location, node ID, and an alert flag. Additional sensor type values from `0..255` can be assigned by a team convention.

## Documentation

- [Wiki home](https://github.com/MKme/sentinel/wiki)
- [Parts and kits](https://github.com/MKme/sentinel/wiki/Parts-and-Kits)
- [Build the first node](https://github.com/MKme/sentinel/wiki/Build-the-First-Node)
- [Wiring reference](https://github.com/MKme/sentinel/wiki/Wiring-Reference)
- [XTOC and XCOM integration](https://github.com/MKme/sentinel/wiki/XTOC-XCOM-Integration)
- [Deployment and troubleshooting](https://github.com/MKme/sentinel/wiki/Deployment-and-Troubleshooting)
- [Packet and firmware notes](https://github.com/MKme/sentinel/wiki/Packet-and-Firmware)

The rendered GitHub Wiki is published from the source-controlled pages in `wiki/`.

## Safe deployment checklist

- label the enclosure and software with the same stable node name
- record board, firmware revision, node ID, coordinates, power, and installation photo
- replace breadboard jumpers with soldered or locking connections
- strain-relieve cables and keep the antenna clear of metal
- test cold boot, power loss, transport loss, false triggers, and heartbeat silence
- verify every alert with a second source before dispatch or escalation
- keep a manual PACE fallback and a scheduled physical inspection

## X Suite

Sentinel reports are decoded by XTOC and XCOM. XTOC provides the Sentinel review page, Tactical Map layer, time-based triggers, label filters, temperature/battery thresholds, input/output-mask triggers, and `T=16 CONTROL` composition. XINTEL supplies local radio/SDR monitoring and packet ingest, while XCORE provides local AI-assisted analysis of XTOC/XCOM operational data.

- [XTOC](https://github.com/MKme/XTOC)
- [XCOM](https://github.com/MKme/xcom)
- [XINTEL](https://github.com/MKme/xintel) provides local radio/SDR monitoring, transcription, and packet ingest
- [XCORE](https://github.com/MKme/xcore) provides local AI-assisted summaries and operational analysis
- [XCAM](https://github.com/MKme/xcam) can also post camera-motion Sentinel reports
- [XNODE](https://github.com/MKme/xnode) provides wearable and keyboard field endpoints

## Radio and security note

The example sketches emit clear packets so they are easy to inspect and test. Use an appropriate authenticated/secure transport for real control where lawful. Encryption or obscured content may be prohibited on amateur radio; the operator is responsible for applicable rules.
