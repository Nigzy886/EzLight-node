# AstroEngine Civil Twilight Tests

These host-side tests sanity-check the EzLight civil dawn / civil dusk calculation before adding any lux, PIR, or advanced astro features.

Run from the repo root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File test\astro_engine\run.ps1
```

The harness mirrors the NOAA-style civil twilight calculation used by `firmware/EzLight_Node/AstroEngine.cpp`.

Coverage:

- Civil dawn and civil dusk use zenith `96.0` degrees.
- Local `HH:MM` output is calculated from latitude, longitude, date, and UTC offset.
- Known New Zealand west-coast coordinates produce plausible winter and summer civil twilight ranges.
- Civil dawn occurs before civil dusk for the tested dates.

This is a sanity check for firmware behavior, not a substitute for a calibrated astronomical library.
