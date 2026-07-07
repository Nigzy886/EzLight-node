# AGENTS.md

This repository contains the initial EzLight-node firmware skeleton.

## Rules

- Treat `C:\Users\Bass\Desktop\GITHUB\EzSystems-standards` as the source of truth.
- Do not change EzSystems standards from this repo.
- Keep relay safety behavior explicit and auditable.
- The ESP32 controls low-voltage relay/control inputs only. Do not imply direct mains switching.
- All relays must be forced OFF immediately on boot before Wi-Fi, filesystem, config loading, schedule evaluation, astro calculation, EzHub connection, or HTTP startup.
- For v0.1 active-low relays, write GPIO HIGH before setting each relay pin to `OUTPUT`.
- Schedules and astro transitions must not run until valid NTP time is available.
- If valid time is lost after boot, hold current relay state, stop schedule/astro transitions, and report `time_valid:false`.
- Bad config must be rejected before apply.
- EzHub commands must be declared in capabilities before they are accepted.
- Unknown or undeclared commands must return an application ACK-style rejection.
- Do not add PIR, lux sensing, current sensing, holiday/random mode, or wall switch inputs for v0.1.
