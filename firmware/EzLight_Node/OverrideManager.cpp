#include "OverrideManager.h"

CommandResult OverrideManager::applyOverride(const String& relayId, const String& overrideMode, RelayDriver& relays) {
  if (!relays.isKnownRelay(relayId)) {
    return {false, "rejected", "invalid_args", "{}"};
  }
  const int index = relays.indexOf(relayId);
  if (relays.mode(index) == RelayMode::Disabled) {
    return {false, "rejected", "unsafe_state", "{}"};
  }
  unsigned long durationMs = 0;
  const OverrideMode parsed = parseMode(overrideMode, durationMs);
  if (parsed == OverrideMode::None) {
    return {false, "rejected", "invalid_args", "{}"};
  }
  (void)durationMs;
  if (overrideMode.startsWith("on_for_") || parsed == OverrideMode::ManualOn) {
    relays.setRelay(relayId, true);
  } else if (parsed == OverrideMode::ManualOff || parsed == OverrideMode::OffUntilNextEvent) {
    relays.setRelay(relayId, false);
  }
  // TODO: Store override mode/expiry in runtime state once persistence rules are finalized.
  String result = String("{\"relay_id\":\"") + relayId + "\",\"override_mode\":\"" + overrideMode + "\"}";
  return {true, "accepted", nullptr, result};
}

CommandResult OverrideManager::resumeSchedule(const String& relayId, RelayDriver& relays) {
  if (!relays.isKnownRelay(relayId)) {
    return {false, "rejected", "invalid_args", "{}"};
  }
  // TODO: Return to calculated schedule/astro state once engines produce target states.
  String result = String("{\"relay_id\":\"") + relayId + "\",\"override_mode\":\"resume_schedule\"}";
  return {true, "accepted", nullptr, result};
}

void OverrideManager::update(RelayDriver& relays) {
  (void)relays;
  // TODO: Expire timed overrides and return to calculated schedule/astro state.
}

OverrideMode OverrideManager::parseMode(const String& overrideMode, unsigned long& durationMs) const {
  durationMs = 0;
  if (overrideMode == "on_for_5_min") { durationMs = 5UL * 60UL * 1000UL; return OverrideMode::OnFor5Min; }
  if (overrideMode == "on_for_15_min") { durationMs = 15UL * 60UL * 1000UL; return OverrideMode::OnFor15Min; }
  if (overrideMode == "on_for_30_min") { durationMs = 30UL * 60UL * 1000UL; return OverrideMode::OnFor30Min; }
  if (overrideMode == "on_for_1_hour") { durationMs = 60UL * 60UL * 1000UL; return OverrideMode::OnFor1Hour; }
  if (overrideMode == "off_until_next_event") { return OverrideMode::OffUntilNextEvent; }
  if (overrideMode == "manual_on") { return OverrideMode::ManualOn; }
  if (overrideMode == "manual_off") { return OverrideMode::ManualOff; }
  return OverrideMode::None;
}
