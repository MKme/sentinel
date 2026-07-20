# XTOC and XCOM Integration

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
