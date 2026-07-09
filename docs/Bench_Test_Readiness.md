# EzLight Bench Test Readiness

Use this checklist before connecting any real relay loads or outdoor lighting hardware.

## Safety Boundary

- Bench testing must use LEDs, a relay module with no mains connected, or a meter only.
- Do not connect outdoor lighting circuits during firmware validation.
- The ESP32 drives low-voltage relay/control inputs only. It does not directly switch mains wiring.
- Real relay/contactors, enclosures, fusing, isolation, and outdoor wiring must be handled properly by a qualified person where required.

## Boot Safety Test

- Power cycle the ESP32.
- Confirm all four relay outputs remain physically OFF at boot.
- Confirm GPIO14, GPIO27, GPIO26, and GPIO25 are written to the OFF level before `pinMode(..., OUTPUT)`.
- Confirm the v0.1 active-low OFF level is GPIO HIGH.
- Fail the bench test if any output turns ON before firmware has validated config and time state.

## Reset Safety Test

- Press reset while watching each relay output with LEDs, a meter, or an unloaded relay module.
- Confirm relay outputs do not flicker ON.
- Confirm the relay interface hardware defaults OFF while the ESP32 is resetting.
- Fail the bench test if the relay interface depends only on firmware to stay OFF during reset.

## Config Validation Test

- Confirm missing `/config.json` uses safe defaults.
- Confirm malformed JSON is rejected.
- Confirm bad GPIO, bad relay mode, bad astro location, bad schedule, and unsupported v0.1 features are rejected.
- Confirm a rejected config does not partially update the active config.
- Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File test\config_validation\run.ps1
```

## Manual Relay Command Test

- Confirm `relay.set` works for relays in `manual` mode.
- Confirm a relay in `disabled` mode rejects a manual relay command.
- Confirm an unknown command is rejected.
- Confirm an undeclared command is rejected.
- Confirm manual relay commands do not alter saved schedules or astro rules.

## Fixed Schedule Test

- Use a short same-day schedule window while observing only LEDs, a meter, or an unloaded relay module.
- Confirm the relay turns ON at the inclusive ON time.
- Confirm the relay turns OFF at the exclusive OFF time.
- Confirm `time_valid=false` prevents schedule transitions and holds current relay state.
- Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File test\schedule_engine\run.ps1
```

## Civil Dusk / Civil Dawn Test

- Set a known latitude and longitude.
- Confirm calculated `civil_dawn` and `civil_dusk` look reasonable for the local date.
- Confirm an astro relay only moves when `time_valid=true`.
- Confirm manual, schedule, and disabled relays are not moved by astro logic.
- Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File test\astro_engine\run.ps1
```

## Status / Debug Page Test

- Confirm `config_loaded`, `config_source`, and `config_error` are visible.
- Confirm relay modes and relay states are visible.
- Confirm `schedule_state`, `next_event`, `civil_dawn`, `civil_dusk`, and `time_valid` are visible.
- Confirm the page remains status/debug only and does not expose schedule or config editing.

## Pass / Fail Gate

- The node is not ready for real relay hardware until these bench tests pass.
- The node is not ready for mains or outdoor lighting until relay/contactors, enclosure, fusing, isolation, and wiring boundaries are handled properly by a qualified person where required.
- Do not promote a firmware build that fails boot safety, reset safety, config validation, command rejection, schedule timing, astro timing, or status/debug visibility checks.
