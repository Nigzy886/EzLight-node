# EzLight-node Standards Baseline

EzLight-node targets **EzSystems Standards v1.1.5**, baseline date **2026-07-03**.

Canonical standards authority:

```text
Nigzy886/EzSystems-standards
```

Primary locked references:

- `EasyHub_Node_JSON_Contract_LOCKED_v2_1_unified.md`
- `EzSystems_Node_Profile_Registry_LOCKED_v1.md`
- `EasyHub_Node_Lifecycle_and_Removal_Spec_LOCKED_v1_1_contract_aligned.md`
- `EasyHub_Hub_and_Node_UI_Spec_LOCKED_v3_1_contract_aligned.md`
- `contracts/EzSystems_Command_Delivery_ACK_LOCKED_v1.md`

EzLight product-specific references:

- `docs/EzLight_Node_SOT.md`
- `docs/EzLight_Firmware_Contract.md`
- `docs/EzLight_EzHub_Integration.md`
- `checklists/EzLight_Node_Checklist.md`

Profile declaration:

```text
experimental.ezlight.v1
```

EzLight does not declare `compact_profile`, because no locked EzLight compact telemetry mapping exists.

## Implementation status

Firmware `0.2.0-managed` implements:

- safe relay-OFF startup before all other subsystems;
- EzSystems v1 ESP-NOW commissioning bootstrap;
- directed `wifi_cfg` validation and application ACK;
- persistent Wi-Fi/Hub provisioning state in NVS;
- Wi-Fi-primary HTTP `hello`, `capabilities`, `state`, and command APIs;
- declared-command gating and application ACKs;
- duplicate `msg_id` protection;
- baseline `node/reboot` with ACK-before-restart and safe outputs;
- validated LittleFS schedule and astro configuration replacement.

Promotion to field/production status still requires all physical and local verification items in `checklists/EzLight_Node_Checklist.md` to pass on the target relay hardware.
