# EzLight-node

EzSystems-managed outdoor lighting controller for an ESP32-WROOM DevKit / ESP32 Dev Module with four active-low relay outputs.

The current firmware target is `0.2.0-managed`. It implements the EzSystems managed-node transport and API foundation, but it must still pass the documented physical bench-test gate before use with real relay loads.

## Standards baseline

This repository targets **EzSystems Standards v1.1.5**. See `STANDARDS_BASELINE.md`.

The canonical standards authority is:

```text
Nigzy886/EzSystems-standards
```

The local development checkout is normally:

```text
C:\Users\Bass\Desktop\GITHUB\EzSystems-standards
```

## Managed-node behavior

- Relays are forced physically OFF before Wi-Fi, ESP-NOW, LittleFS, configuration, time, schedules, astro calculation, or HTTP startup.
- Unprovisioned nodes scan ESP-NOW channels for a valid EzHub `pair_beacon` or Hub `hello`.
- Directed `wifi_cfg` is validated against `node_id` and the sender Hub MAC.
- Wi-Fi credentials, Hub MAC, provisioning state, and Wi-Fi channel are stored in Preferences/NVS.
- Provisioning returns an application `ack` before restart.
- Provisioned nodes connect to router Wi-Fi, advertise mDNS, send a fresh bootstrap `hello`, and expose authoritative HTTP APIs.
- `POST /api/cmd` requires `node_id`, `msg_id`, `target`, and `action`.
- Declared commands return application ACKs and duplicate `msg_id` requests are deduplicated.
- `node/reboot` forces all relays OFF, sends its ACK, waits briefly, and then restarts.

## Authoritative HTTP APIs

- `GET /api/hello`
- `GET /api/capabilities`
- `GET /api/state`
- `POST /api/cmd`

The node-local `/` and `/status` pages are diagnostics only. EzHub remains the normal operator interface.

## Relay mapping

| Relay ID | GPIO | `active_low` | OFF level |
|---|---:|---:|---:|
| `path_lights` | 14 | `true` | HIGH |
| `driveway_lights` | 27 | `true` | HIGH |
| `entrance_lights` | 26 | `true` | HIGH |
| `spare_lights` | 25 | `true` | HIGH |

## Time, schedules, and astro

| Decision | Value |
|---|---|
| Time source | NTP only |
| RTC | Not required |
| Astro convention | Civil dusk / civil dawn |
| Configuration storage | LittleFS JSON |
| Provisioning storage | Preferences/NVS |
| Local web UI | Status/debug only |

Schedule and astro commands validate the complete replacement configuration before applying it. A rejected replacement leaves the active configuration unchanged.

## Safety boundary

The ESP32 drives low-voltage relay/control inputs only. It must not be described as directly switching mains lighting. Mains wiring must use suitable relays/contactors, enclosures, fusing, isolation, and qualified electrical work where required.

Before connecting real relay loads or outdoor lighting hardware, complete:

- `docs/Bench_Test_Readiness.md`
- `docs/Physical_Bench_Test_Procedure.md`

Firmware validation must initially use LEDs, a meter, or an unloaded relay module only.

## Build

```powershell
arduino-cli core install esp32:esp32
arduino-cli lib install ArduinoJson
arduino-cli compile --fqbn esp32:esp32:esp32 firmware\EzLight_Node
```
