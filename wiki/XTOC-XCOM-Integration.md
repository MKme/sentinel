# XTOC and XCOM integration

## Match the receiver to the report

| Report | Where to look |
| --- | --- |
| Presence Wi-Fi/motion text | Receiving mesh messages; not Sentinel T=11 |
| T=11 location/sensor report | XTOC Sentinel after import |
| T=11 companion observation | Updated XTOC Sentinel, including quality fields |
| SMV1 MAVLink | Matching bridge UART consumer |
| SMF1 flight traffic | XTOC Drones through the Drones Bridge |

## Import and inspect T=11

1. For a manual test, copy the entire `X1.11.C...` packet into **Comms → Import/Export**, paste and import. The standalone sketch prints one packet per line.
2. For radio operation, connect the receiving mesh using the app's mesh controls and enable incoming packet auto-import. A shared flight-ground USB connection needs Drones Bridge `sentinelReports` and `sentinelChannel`; do not open two USB clients.
3. In XTOC open **Sentinel**, select the report and check node ID, label, timestamp, readings and location quality.
4. Enable the **Sentinel** Tactical Map layer. NO GPS is not zero latitude/longitude and must not create a map point.

XCOM supports Sentinel reception, decoding and map workflows. Use an updated receiver that understands observation quality flags. XTOC provides the observation review and SOA workflow below. A label such as WRAITH-01 identifies your platform by convention; T=11 has no drone/entity-type field.

![XTOC Sentinel with no GPS](../assets/presence/observation-no-gps.png)

*Current XTOC with simulated observations. Missing position is explicit.*

## Configure observation alerts

Acquire **Sentinel Object Observation Alerts** in XTOC **XPASS**. In **Triggers**, search **SOA** and edit the fire, smoke, custom-object, stale-GPS or no-GPS rule. Set source, class, confidence and GPS filters, enable and save. Templates and two local plugin examples start disabled. Initial acquisition needs online authorization; installed authorized content continues offline.

![SOA trigger settings](../assets/presence/observation-trigger.png)

*Example filters. Configure, enable and save the intended rule.*

Submit a new matching observation after cooldown and verify the alert and Sentinel details. Test a nonmatching class too. Coordinates locate the observing sensor, not the detected object.

## Other sensors and control

For custom T=11 rules use exact label and sensor convention: RCWL type 1, PIR type 2, temperature type 6 in tenths °C, battery type 7 in mV. Assign contact bits in `inMask` consistently. Keep a separate expected-report/silence process.

Standalone **T=16 CONTROL** is for engineered low-voltage output banks. Target the correct node and verify returned `outMask`. It does not make Presence a T=16 actuator. For aircraft use [Flight-controller link](Flight-Controller-Link.md).

XINTEL provides local radio/SDR monitoring and packet ingest; XCORE provides local AI-assisted analysis of XTOC/XCOM data. Neither replaces the radio/import setup above.
