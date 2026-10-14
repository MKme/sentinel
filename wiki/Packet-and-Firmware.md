# Packet and Firmware

## Presence firmware update

Presence can emit position-only Sentinel v1 reports with zero sensor readings, label, UTC minute, coordinates and stable node ID. Legacy telemetry uses PRIVATE_APP port 256 with SMV1 framing; paired flight requests and replies use SMF1 plus UTC seconds and the original MAVLink frame; converted X1.11.C reports use TEXT_MESSAGE_APP. These are distinct transports.

[Presence, MAVLink and Sentinel location setup](Presence-and-MAVLink.md) includes current setup screenshots and validation steps.

Sentinel uses X Suite template `T=11`. The v1 payload includes version, sensor count, time in Unix minutes, latitude/longitude at 1e-5 degrees, node ID, flags, optional IO masks, optional UTF-8 label, and repeated 1-byte type plus signed 16-bit value readings.

The wrapper is:

```text
X1.11.C.<packet-id>.1/1.<base64url-payload>
```

`T=16 CONTROL` targets a node ID and can apply `ABS`, `SET`, `CLEAR`, or `TOGGLE` to an 8-bit output mask. The included control sketch parses clear demonstration packets and can emit a `T=11` state acknowledgement.

For custom derivatives of the standalone Arduino examples, add freshness checks, credible time, appropriate transport, rate limiting, configuration persistence, watchdog/recovery behavior, and secure/authenticated control where lawful.

## Direct aircraft supervision

Use the [paired flight-controller link](Flight-Controller-Link.md) for XTOC commands and returning telemetry through a USB ground node. This is separately enabled from the legacy telemetry bridge and T=11 position reporting.
