# Build and Flash

EzLight-node v0.1 targets ESP32-WROOM DevKit / ESP32 Dev Module using Arduino framework and `arduino-cli`.

## Compile

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32 firmware\EzLight_Node
```

## Upload

Replace `COM_PORT` with the ESP32 serial port.

```powershell
arduino-cli upload -p COM_PORT --fqbn esp32:esp32:esp32 firmware\EzLight_Node
```

## Notes

- Install the ESP32 Arduino core before compiling.
- Wi-Fi provisioning is a TODO in this skeleton.
- The local web UI is status/debug only in v0.1.
- No PIR, lux sensor, current sensing, random/holiday mode, or wall switch input support belongs in v0.1.
