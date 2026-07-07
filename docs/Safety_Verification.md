# Safety Verification

Run this before connecting any real lighting loads.

## Relay Boot Safety

- Confirm all relay control pins are written to their OFF level before `pinMode(..., OUTPUT)`.
- Confirm all relay outputs remain OFF at cold boot.
- Confirm all relay outputs remain OFF during reset.
- Confirm all relay outputs remain OFF before Wi-Fi starts.
- Confirm invalid config is rejected before apply.

## Time and Schedule Safety

- Confirm schedules do not transition outputs until `time_valid:true`.
- Confirm time loss after boot holds current relay state.
- Confirm time loss stops schedule and astro transitions.
- Confirm telemetry reports `time_valid:false` when time is lost.

## Command Safety

- Confirm unknown commands return an application ACK-style rejection.
- Confirm undeclared commands return `not_declared` or another stable rejection.
- Confirm disabled relays reject manual relay commands until mode changes.
- Confirm manual overrides do not rewrite saved schedules.

## Electrical Boundary

- Confirm documentation and UI say the ESP32 drives relay/control inputs only.
- Confirm mains wiring is handled through suitable relays/contactors, enclosures, fusing, isolation, and qualified electrical work where required.
