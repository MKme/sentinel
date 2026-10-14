# Wiring Reference

## Presence firmware update

Presence UART: D7/GPIO44 RX connects to source TX; D6/GPIO43 TX connects to consumer RX; share GND and use 3.3 V TTL. USB diagnostics use 115200 baud. RCWL uses GPIO3 (alternate GPIO4).

[Presence, MAVLink and Sentinel location setup](Presence-and-MAVLink.md) includes current setup screenshots and validation steps.

## Direct input pattern

| Function | ESP32 example | Notes |
| --- | --- | --- |
| PIR/RCWL output | GPIO 4 | verify the module output is safe for 3.3 V logic |
| reed contact | GPIO 18 to contact, other contact to GND | configure `INPUT_PULLUP`; closed normally reads low |
| DS18B20 data | GPIO 19 | normally requires a 4.7 kΩ pull-up from data to 3.3 V |
| battery ADC | GPIO 34 through calculated divider | never apply battery voltage directly above the ADC rating |

All modules must share ground unless a professionally designed isolated interface is required. Confirm the exact board pinout; GPIO behavior differs across ESP32 families.

## Output warning

GPIO pins cannot drive coils, lamps, motors, or relays directly. The control demo is logic-level reference code. Use a suitable low-voltage driver, flyback protection for inductive loads, fuse, safe default state, and acknowledgement. Never use this project to switch mains or hazardous loads.
