# Build the First Node

1. Wire HC-SR501 `VCC` to the required supply, `GND` to ground, and `OUT` to GPIO 4.
2. Open `arduino/SentinelClearDemo/SentinelClearDemo.ino`.
3. Set a unique `NODE_ID`, operational `LABEL`, and real `LAT`/`LON`.
4. Upload to the exact selected ESP32 board.
5. Open Serial Monitor at 115200 baud.
6. Copy one complete `X1.11.C...` line into XTOC/XCOM Comms import.
7. Confirm identity, map position, sensor type/value, timestamp, and alert state.
8. Adjust PIR sensitivity/delay and perform walk and nuisance tests.
9. Solder the deployed version, add strain relief, enclose it, and repeat the entire test.

The example epoch base is only a demonstration. A deployed node needs a credible time source or a managed time-setting process.
