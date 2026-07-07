# Hardware Decisions

## Locked v0.1 Target

| Decision | Value |
|---|---|
| Board | ESP32-WROOM DevKit / ESP32 Dev Module |
| Relay count | 4 |
| Relay polarity | `active_low:true` for all relays |
| Active-low OFF level | GPIO HIGH |
| Time source | NTP only |
| RTC fallback | Not required |

## Relay Mapping

| Relay id | GPIO | OFF level |
|---|---|---|
| `path_lights` | GPIO14 | GPIO HIGH |
| `driveway_lights` | GPIO27 | GPIO HIGH |
| `entrance_lights` | GPIO26 | GPIO HIGH |
| `spare_lights` | GPIO25 | GPIO HIGH |

Firmware must write GPIO HIGH before setting each relay pin to `OUTPUT`.

## Hardware Safety Requirement

Hardware must keep relay interfaces OFF while the ESP32 is resetting. Acceptable approaches include suitable pull-ups, relay enable gating, or an equivalent safe interface design.

The ESP32 drives low-voltage relay/control inputs only. It does not directly switch mains wiring.
