# EzLight Config Validation Fixtures

These fixtures exercise the v0.1 `/config.json` contract without requiring ESP32 hardware.

Run from the repo root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File test\config_validation\run.ps1
```

The helper mirrors the locked v0.1 validation rules from `firmware/EzLight_Node/ConfigStore.cpp`:

- `valid_default_config.json` must pass.
- Every `invalid_*.json` fixture must be rejected before any config is applied.
- A missing `/config.json` is not represented as a file fixture; firmware treats that path as safe defaults with `config_loaded=false`.
- Schedule execution, astro calculation, and config editing UI are intentionally not tested here because they are not part of v0.1.

The helper is a host-side guardrail, not a replacement for compiling the Arduino firmware.
