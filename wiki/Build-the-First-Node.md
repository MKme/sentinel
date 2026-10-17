# Build the first node

Start with a **Presence Wi-Fi detection node**, then add telemetry or a companion. The [standalone Arduino demos](Standalone-Examples.md) are a separate path.

## Assemble and load firmware

Use the XIAO ESP32-S3/Wio-SX1262 assembly, correct antenna and stable supply. Add RCWL motion if needed, following [Wiring](Wiring-Reference.md). Follow [Build and flash](Build-and-Flash.md); generic Meshtastic firmware does not provide the Sentinel detector or portal.

## Configure the radio

Use the Meshtastic Bluetooth app to configure region, matching modem preset, channel/key and radio role on both nodes. Enable TX. Create a private channel and note its local index on each radio; indices can differ.

## Open local setup

Restart, join **SENTINEL-xxxx**, enter **sentinel123**, and open **http://192.168.4.1** during the first **120 seconds**. This local network needs no internet. Restart to reopen an expired window. Configure in a controlled location: the setup password is a known default.

![Boot configuration screen](../assets/presence/setup-desktop.png)

*Actual firmware renderer with simulated settings. Choose label, motion GPIO and Wi-Fi filters.*

Set a label such as `SITE-01`. Start with channels 1–11, -82 dBm threshold and 250 ms dwell. Enable **Send detections over Meshtastic** and choose your private channel. Leave telemetry forwarding, aircraft link, position conversion and companion input off until the base path works.

![Mesh output settings](../assets/presence/mesh-output.png)

*Example channels. Selecting an index does not configure its key or RF region.*

## Save and prove reception

Select **Save and start detection**. Successful save persists settings and closes the portal; failed save keeps it open. The setup SSID disappearing after save/timeout is expected.

![Saved configuration result](../assets/presence/saved.png)

*Firmware-rendered confirmation. Next inspect scanner startup and a received alert.*

Open USB diagnostics at **115200 baud**, with no competing serial client. Confirm scanner startup, observe a nearby device emitting qualifying traffic and verify the corresponding alert on the receiving mesh node. Devices need not join Sentinel, but not every transmission qualifies. Read [Wi-Fi detection](Wi-Fi-Detection.md) for filtering and cooldown behavior.

Test motion separately, then inside the combined-event window. Restart and confirm settings survive and another alert arrives before enclosing the node.

Next: [configuration](Configuration-Reference.md), [integration](XTOC-XCOM-Integration.md), [deployment](Deployment-and-Troubleshooting.md).
