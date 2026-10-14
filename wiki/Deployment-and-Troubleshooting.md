# Deployment and Troubleshooting

## Presence firmware update

For missing MAVLink locations, check selected source/peer/system/component, private output channel, RF TX, UTC and fresh GPS_RAW_INT 3D fix. USB fresh_fix and UTC flags explain suppression. Check the independent raw forwarding switches only when raw transport is required.

[Presence, MAVLink and Sentinel location setup](Presence-and-MAVLink.md) includes current setup screenshots and validation steps.

## Acceptance

- label, node ID, coordinates, and map marker agree
- cold boot and power recovery work
- normal and alert packets arrive through the intended transport
- false-motion, open/closed contact, temperature comparison, and low-battery behavior are tested
- enclosure, cable glands, antenna clearance, and strain relief are inspected
- operator knows how to verify and respond

## Troubleshooting order

1. Check damage, heat, swelling, shorts, and polarity.
2. Measure supply voltage at the controller during transmission.
3. Read the serial boot/output log.
4. Test the sensor locally without the transport.
5. Verify node ID, label, time, coordinates, packet wrapper, and channel/address.
6. Verify bridge/import, map layer, label filter, and trigger cooldown.

PIR nodes need warm-up. Sunlight, HVAC, pets, vegetation, loose mounts, and rapid thermal changes can cause nuisance alerts. A silent node is unknown—not safe.
