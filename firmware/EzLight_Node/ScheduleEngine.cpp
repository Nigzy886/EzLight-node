#include "ScheduleEngine.h"

ScheduleEngine::ScheduleEngine() : _nextEvent("none") {
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    _states[i] = "manual";
  }
}

void ScheduleEngine::update(bool timeValid, RelayDriver& relays) {
  if (!timeValid) {
    _nextEvent = "time_invalid";
    for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
      _states[i] = (relays.mode(i) == RelayMode::Schedule || relays.mode(i) == RelayMode::Astro) ? "paused_time_invalid" : relayModeToString(relays.mode(i));
    }
    return;
  }
  // TODO: Apply fixed HH:MM schedule events after config schema is implemented.
  _nextEvent = "not_scheduled";
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    _states[i] = relayModeToString(relays.mode(i));
  }
}

const String& ScheduleEngine::nextEvent() const { return _nextEvent; }
const String& ScheduleEngine::stateFor(uint8_t index) const { return _states[index < EZLIGHT_RELAY_COUNT ? index : 0]; }
