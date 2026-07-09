# EzLight-node

Initial firmware repository skeleton for the EzSystems `EzLight-node` outdoor lighting controller.

This repo is intentionally a v0.1 skeleton. It is not production firmware yet.

## Source of Truth

Canonical standards live in:

```text
C:\Users\Bass\Desktop\GITHUB\EzSystems-standards
```

Required source documents:

- `docs/EzLight_Node_SOT.md`
- `docs/EzLight_Firmware_Contract.md`
- `docs/EzLight_Hardware.md`
- `docs/EzLight_Schedule_Logic.md`
- `docs/EzLight_Astro_Dusk_Dawn.md`
- `docs/EzLight_EzHub_Integration.md`
- `docs/EzLight_Safety_Mains_Wiring.md`
- `checklists/EzLight_Node_Checklist.md`
- `examples/esp32_4relay_light_node/README.md`

## Locked v0.1 Target

| Decision | Value |
|---|---|
| Board | ESP32-WROOM DevKit / ESP32 Dev Module |
| Framework | Arduino / `arduino-cli` compatible |
| Relay count | 4 |
| Time source | NTP only |
| RTC | Not required for v0.1 |
| Astro convention | Civil dusk / civil dawn |
| Config storage | LittleFS JSON |
| Local web UI | Status/debug only |

## Relay Mapping

| Relay id | GPIO | `active_low` | OFF level |
|---|---|---|---|
| `path_lights` | GPIO14 | `true` | GPIO HIGH |
| `driveway_lights` | GPIO27 | `true` | GPIO HIGH |
| `entrance_lights` | GPIO26 | `true` | GPIO HIGH |
| `spare_lights` | GPIO25 | `true` | GPIO HIGH |

Firmware must write each OFF level before setting the relay pin to `OUTPUT`.

## Safety Boundary

The ESP32 drives low-voltage relay/control inputs only. It must not be described as directly switching mains lighting. Mains wiring must be handled through suitable relays/contactors, enclosures, fusing, isolation, and qualified electrical work where required.

Before connecting any real relay loads or outdoor lighting hardware, complete the bench-test gate in `docs/Bench_Test_Readiness.md`. Firmware validation must use LEDs, a meter, or an unloaded relay module only.

## Build Sketch

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32 firmware\EzLight_Node
```

Install the ESP32 Arduino core before compiling if it is not already installed.
