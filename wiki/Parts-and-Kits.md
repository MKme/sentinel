# Parts and kits

## Presence node

| Part | Purpose / selection |
| --- | --- |
| XIAO ESP32-S3 | Controller and passive Wi-Fi receiver |
| Wio-SX1262 B2B for XIAO | Match this exact radio assembly/pin mapping |
| Correct-band LoRa antenna | Fit before transmitting; match radio and region |
| USB data cable and regulated supply | Setup, diagnostics/API and continuous reception |
| Second compatible Meshtastic node | Verify actual radio reception |
| Optional RCWL-0516 | Stationary motion cue; OUT to D2/GPIO3 by default |
| Enclosure, connectors and strain relief | Protect the board and wiring |

Wi-Fi detection needs no external sensor. Keep a disconnected motion input stable and verify its idle state before interpreting motion events.

## Add the needed interface

Connect the companion through USB data. Plan power to avoid backfeeding supplies.

## Standalone learning kit

The [Arduino demos](Standalone-Examples.md) use an ESP32 DevKit and motion sensor. They print packets and are not the Presence mesh firmware. Contacts, temperature probes and battery ADC circuits are custom extensions, not automatically enabled Presence inputs.

Carry a programmed spare of the same variant, data cable, antenna, connectors, fuse and known-good supply. Use protected, chemistry-compatible power. Measure consumption during radio activity: Presence is a continuous receiver, not a sleeping sensor.

Next: [Wiring](Wiring-Reference.md) and [first-node setup](Build-the-First-Node.md).
