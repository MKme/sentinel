# Packet and Firmware

Sentinel uses X Suite template `T=11`. The v1 payload includes version, sensor count, time in Unix minutes, latitude/longitude at 1e-5 degrees, node ID, flags, optional IO masks, optional UTF-8 label, and repeated 1-byte type plus signed 16-bit value readings.

The wrapper is:

```text
X1.11.C.<packet-id>.1/1.<base64url-payload>
```

`T=16 CONTROL` targets a node ID and can apply `ABS`, `SET`, `CLEAR`, or `TOGGLE` to an 8-bit output mask. The included control sketch parses clear demonstration packets and can emit a `T=11` state acknowledgement.

Before deployment, add freshness checks, credible time, appropriate transport, rate limiting, configuration persistence, watchdog/recovery behavior, and secure/authenticated control where lawful.
