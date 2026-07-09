# ScheduleEngine Boundary Tests

These tests verify the fixed HH:MM schedule boundary behavior before EzLight adds dusk/dawn calculation.

Run from the repo root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File test\schedule_engine\run.ps1
```

The harness mirrors the small fixed-window logic in `firmware/EzLight_Node/ScheduleEngine.cpp` instead of compiling Arduino classes on the host. It is a boundary verifier for:

- same-day windows where `on <= now < off`
- overnight windows where `now >= on || now < off`
- no relay writes when `time_valid=false`
- no fixed schedule writes for manual, disabled, or astro relays
- stable `next_event` strings for simple same-day and overnight schedules
- meaningful per-relay `schedule_state`

It does not test astro dusk/dawn, schedule editing UI, or hardware GPIO behavior.
