# EzLight Firmware Checklist

## Source of Truth

- Read `C:\Users\Bass\Desktop\GITHUB\EzSystems-standards\docs\EzLight_Node_SOT.md`.
- Read the EzLight firmware, hardware, schedule, astro, EzHub, and safety standards docs before changing behavior.

## Boot Safety

- Relay OFF levels are defined before setup continues.
- GPIO14, GPIO27, GPIO26, and GPIO25 are written HIGH before `pinMode(..., OUTPUT)`.
- All relays remain OFF before Wi-Fi, LittleFS, config loading, schedule evaluation, astro calculation, EzHub connection, or HTTP startup.
- Bench testing uses LEDs, a meter, or an unloaded relay module only. No mains or outdoor lighting circuits are connected during firmware validation.
- Reset testing confirms relay outputs do not flicker ON and the relay interface hardware defaults OFF while the ESP32 is resetting.

## Config

- Defaults define exactly four relays.
- Defaults use `active_low:true` for all four relays.
- Bad relay id, GPIO, polarity, timezone, schedule, or astro config is rejected before apply.
- Bad config is never half-applied.

## Commands

- Capabilities declare every accepted command.
- Unknown commands are rejected.
- Undeclared commands are rejected.
- Disabled relays reject manual relay commands until mode changes.
- Command responses use application ACK shape.

## Time, Schedule, Astro

- NTP is the only v0.1 time source.
- RTC fallback is not required.
- Schedule and astro transitions do not run until valid time exists.
- Time loss holds current relay state, stops schedule/astro transitions, and reports `time_valid:false`.
- Civil dusk/civil dawn is the default astro convention.

## UI and Scope

- Local web UI is status/debug only.
- No local schedule editor exists in v0.1.
- No config editor exists in v0.1.
- PIR, lux sensor, current sensing, holiday/random mode, and wall switch inputs are not implemented.

## Bench Test Gate

- Complete `docs/Bench_Test_Readiness.md` before connecting real relay hardware.
- Complete `docs/Physical_Bench_Test_Procedure.md` on a real bench with no mains connected before connecting real relay hardware.
- Do not connect mains or outdoor lighting until relay/contactors, enclosure, fusing, isolation, and wiring boundaries are handled properly by a qualified person where required.
