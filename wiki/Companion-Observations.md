# Companion object observations

Your Pi runs the classifier. Sentinel receives the result, attaches the available sensor position and sends a compact Sentinel alert. A FIRE result can become an XTOC popup through the SOA trigger templates; no image is transmitted and no aircraft action follows automatically.

1. Connect the Pi's USB host/data port to Sentinel USB using a data cable. Leave the flight controller on the separate 3.3 V MAVLink UART. Do not join two transmit wires.
2. Enable **Accept local companion observations (SOI1)** in setup, mesh output and a configured private channel. The default observation interval is 15 seconds.
3. Configure Serial MAVLink for position, or use the enabled flight link's controller identity. Without telemetry Sentinel reports **NO GPS**. A valid fix older than 15 seconds is **STALE GPS**; a bad fix clears the usable position.
4. Send your classifier's label, confidence, count, unique ID and age to the ESP's own node ID using the Meshtastic local API. Follow the [input contract and generic sender](../firmware/SentinelPresenceMeshtastic/OBSERVATIONS.md). Camera and classifier code are yours to supply.

![Sentinel companion input setup](../assets/presence/companion-setup.png)

*Actual firmware setup with simulated settings.*

5. Enable ground mesh auto-import in XTOC. A shared flight-bridge connection also needs its `sentinelReports` setting. Review the report in **Sentinel**.
6. Install **Sentinel Object Observation Alerts** from XPASS. In **Triggers**, search **SOA**, set label, source, confidence and GPS filters, then enable and save the appropriate rule. Optional local plugins start disabled too.

Coordinates locate the observing sensor, not the object. Review the reported age, source and confidence before acting. No GPS must never be interpreted as zero latitude/longitude. Repeated labels and IDs are suppressed; queued observations expire after 30 seconds, and flight traffic takes priority. Radio delivery is not guaranteed.

See the XTOC **Sentinel companion observations** guide for illustrated trigger/plugin instructions. Initial XPASS acquisition requires authorization; installed content uses the established offline lifecycle.


![XTOC observation configuration](../assets/presence/observation-trigger.png)

*Current XTOC with example SOA settings. Enable and save the configured rule.*

![XTOC observation with no GPS](../assets/presence/observation-no-gps.png)

*Current XTOC with simulated data. No telemetry still produces an alert with explicit missing location.*
