# EzLight Physical Bench Test Procedure

Use this procedure to test EzLight-node on a workshop bench before connecting any real relay loads or outdoor lighting hardware.

## 1. Test Boundary

- No mains is connected during this procedure.
- No outdoor lighting circuits are connected during this procedure.
- Use LEDs, relay module indicator LEDs, or a multimeter only.
- The ESP32 controls low-voltage relay/control inputs only.
- Mains lighting work must not start until relay/contactors, enclosure, fusing, isolation, and wiring are handled correctly by a qualified person where required.

## 2. Required Bench Hardware

- ESP32-WROOM DevKit / ESP32 Dev Module.
- USB cable.
- 4-channel relay module or LED test board.
- Optional multimeter.
- Computer with `arduino-cli`.
- No mains load.
- Optional 5 V relay-module supply if required by the relay board.

## 3. Wiring Table

| Relay id | ESP32 GPIO | `active_low` | OFF level | ON level |
|---|---:|---:|---|---|
| `path_lights` | GPIO14 | `true` | GPIO HIGH | GPIO LOW |
| `driveway_lights` | GPIO27 | `true` | GPIO HIGH | GPIO LOW |
| `entrance_lights` | GPIO26 | `true` | GPIO HIGH | GPIO LOW |
| `spare_lights` | GPIO25 | `true` | GPIO HIGH | GPIO LOW |

Active-low relay modules may energise if GPIOs float. The relay interface hardware must default OFF while the ESP32 is resetting, using suitable pull-ups, enable gating, or an equivalent safe interface.

## 4. Pre-Flash Checks

From the repo root:

```powershell
git status --short
git log --oneline -1
arduino-cli compile --fqbn esp32:esp32:esp32 firmware\EzLight_Node
powershell -NoProfile -ExecutionPolicy Bypass -File test/config_validation/run.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File test/schedule_engine/run.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File test/astro_engine/run.ps1
```

Expected baseline before this procedure:

```text
212022e Add EzLight bench-test readiness docs
```

Proceed only when the repo is clean, compile passes, and all fixture/helpers pass.

## 5. Flash Procedure

Replace `COM_PORT` with the ESP32 serial port:

```powershell
arduino-cli upload -p COM_PORT --fqbn esp32:esp32:esp32 firmware\EzLight_Node
```

Open serial output after flashing if available. Keep the relay module disconnected for the first power-on if the interface hardware has not already been proven safe.

## 6. Boot-Off Test

1. Disconnect the relay module or use LEDs/meter first if unsure.
2. Power the ESP32 by USB.
3. Confirm GPIO14, GPIO27, GPIO26, and GPIO25 are at the OFF level.
4. For the v0.1 active-low default, confirm OFF = GPIO HIGH.
5. Confirm no relay channel energises during boot.
6. Confirm Serial output says relays were forced OFF.

Stop if any output turns ON during boot.

## 7. Reset-Off Test

1. Press EN/reset.
2. Watch relay LEDs or the meter.
3. Confirm no channel flickers ON.
4. Confirm the hardware interface holds relay inputs OFF while the ESP32 resets.
5. If any channel flickers ON, stop and fix the hardware interface before continuing.

## 8. Manual Relay Command Test

Use the status/debug device IP address and replace `DEVICE_IP` as needed.

Turn `path_lights` ON:

```powershell
Invoke-WebRequest -Method Post -Uri http://DEVICE_IP/api/cmd -Body '{"msg_id":"bench-relay-on","node_id":"ezlight_001","target":"relay","action":"set","relay_id":"path_lights","state":true}' -ContentType 'application/json'
```

Turn `path_lights` OFF:

```powershell
Invoke-WebRequest -Method Post -Uri http://DEVICE_IP/api/cmd -Body '{"msg_id":"bench-relay-off","node_id":"ezlight_001","target":"relay","action":"set","relay_id":"path_lights","state":false}' -ContentType 'application/json'
```

Toggle `driveway_lights`:

```powershell
Invoke-WebRequest -Method Post -Uri http://DEVICE_IP/api/cmd -Body '{"msg_id":"bench-toggle","node_id":"ezlight_001","target":"relay","action":"toggle","relay_id":"driveway_lights"}' -ContentType 'application/json'
```

Disabled relay rejection test:

```powershell
Invoke-WebRequest -Method Post -Uri http://DEVICE_IP/api/cmd -Body '{"msg_id":"bench-disabled","node_id":"ezlight_001","target":"relay","action":"set","relay_id":"spare_lights","state":true}' -ContentType 'application/json'
```

Unknown or undeclared command rejection test:

```powershell
Invoke-WebRequest -Method Post -Uri http://DEVICE_IP/api/cmd -Body '{"msg_id":"bench-unknown","node_id":"ezlight_001","target":"relay","action":"blink","relay_id":"path_lights"}' -ContentType 'application/json'
```

Expected results:

- `relay.set` works for manual relays.
- `relay.toggle` works for manual relays.
- Disabled relays reject manual relay commands.
- Unknown or undeclared commands return an application ACK-style rejection.

## 9. Config Validation Bench Test

Test config files through the project fixture helper first:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File test/config_validation/run.ps1
```

For device-level config tests, place the candidate file at LittleFS `/config.json` using the selected LittleFS upload method for the bench environment.

Test cases:

- Missing `/config.json`.
- Valid config.
- Malformed config.
- Bad GPIO.
- `active_low:false`.
- Bad schedule.
- Bad astro location.

Expected results:

- Missing `/config.json` uses safe defaults.
- Bad config is rejected before apply.
- Active config is not partially changed.
- Status page shows `config_loaded`, `config_source`, and `config_error`.

## 10. Fixed Schedule Bench Test

Use short test windows near the current time. Keep only LEDs, a meter, or an unloaded relay module connected.

Example:

1. Put `path_lights` in `schedule` mode.
2. Add a rule that turns ON one minute from now.
3. Add OFF two minutes later.
4. Upload the config as LittleFS `/config.json`.
5. Reboot the ESP32.
6. Confirm the ON time is inclusive.
7. Confirm the OFF time is exclusive.
8. Confirm `schedule_state` and `next_event` update.
9. Confirm `time_valid=false` prevents schedule transitions if time is unavailable.

Host-side boundary check:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File test/schedule_engine/run.ps1
```

## 11. Civil Dusk / Civil Dawn Bench Test

Use a known local latitude and longitude.

1. Set `location_set:true`.
2. Set `latitude` and `longitude`.
3. Put `entrance_lights` in `astro` mode.
4. Add a `civil_dusk` to `civil_dawn` rule.
5. Upload the config as LittleFS `/config.json`.
6. Reboot the ESP32.
7. Confirm status/telemetry shows `civil_dawn` and `civil_dusk`.
8. Confirm the times are reasonable for the current local date.
9. Confirm astro does not move manual, schedule, or disabled relays.
10. Confirm astro does not move relays when `time_valid=false`.

Host-side civil twilight sanity check:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File test/astro_engine/run.ps1
```

## 12. Status / Debug Page Checklist

Open:

```text
http://DEVICE_IP/status
```

Confirm visible:

- `config_loaded`
- `config_source`
- `config_error`
- relay states
- relay modes
- `schedule_state`
- `next_event`
- `time_valid`
- `civil_dawn`
- `civil_dusk`
- `uptime`

## 13. Pass / Fail Record

| Test | Expected result | Pass/fail | Notes |
|---|---|---|---|
| Pre-flash checks | Compile and helpers pass |  |  |
| Boot-off test | All outputs remain OFF |  |  |
| Reset-off test | No output flickers ON |  |  |
| Manual relay command | Manual relay commands work; disabled rejects |  |  |
| Config validation | Bad config rejected before apply |  |  |
| Fixed schedule | ON inclusive, OFF exclusive, time gate works |  |  |
| Civil dusk/dawn | Times reasonable; only astro relays move |  |  |
| Status/debug page | Required fields visible |  |  |

## 14. Stop Conditions

Stop testing if:

- Any relay energises during boot or reset.
- GPIO OFF level is wrong.
- Config errors are ignored.
- Schedule moves a manual, disabled, or astro relay incorrectly.
- Astro moves a manual, schedule, or disabled relay incorrectly.
- `time_valid=false` still allows schedule or astro movement.
- The relay board behaves differently from expected active-low logic.

## 15. Next Step After Pass

After bench tests pass with no mains connected, the next stage is relay/contactors/enclosure planning. Do not connect outdoor lighting circuits directly to the ESP32 or to a loose bench relay module.
