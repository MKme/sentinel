# XTOC and XCOM Integration

## Presence firmware update

XTOC connected-mesh auto-import can ingest the converted X1.11.C reports through its existing Sentinel decoder. Enable incoming auto-import and verify the displayed label/location. WRAITH-01 is a platform label; T=11 has no drone classification field. Live hardware import remains a bench acceptance step.

[Presence, MAVLink and Sentinel location setup](Presence-and-MAVLink.md) includes current setup screenshots and validation steps.

## Import a report

1. Copy the complete packet printed by the node.
2. Open `Comms -> Import/Export` in XTOC or XCOM.
3. Paste and import.
4. Review it on the Sentinel page.
5. Enable the Sentinel Tactical Map layer.

## Create useful triggers

- motion: filter the exact node label and choose type 1 or 2
- temperature: use type 6 with a threshold and re-arm interval
- battery: use type 7 with a conservative millivolt threshold
- contact: assign a documented `inMask` bit
- silence: use a heartbeat/report expectation and a separate no-report process

## CONTROL

Compose `T=16 CONTROL` only for engineered low-voltage outputs. Target the exact node ID, prefer `SET`/`CLEAR` over ambiguous toggles, request acknowledgement, and confirm the resulting `outMask` report.

## Direct aircraft supervision

Use the [paired flight-controller link](Flight-Controller-Link.md) for XTOC commands and returning telemetry through a USB ground node. This is separately enabled from the legacy telemetry bridge and T=11 position reporting.
