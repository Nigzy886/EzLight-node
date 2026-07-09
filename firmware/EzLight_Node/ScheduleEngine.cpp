#include "ScheduleEngine.h"
#include <time.h>

ScheduleEngine::ScheduleEngine() : _enabled(false), _ruleCount(0), _nextEvent("none") {
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    _states[i] = "manual";
  }
}

void ScheduleEngine::configure(const EzLightConfig& config) {
  _enabled = config.scheduleGloballyEnabled;
  _ruleCount = config.scheduleRuleCount;
  for (uint8_t i = 0; i < _ruleCount; ++i) {
    _rules[i] = config.scheduleRules[i];
  }
  for (uint8_t i = _ruleCount; i < EZLIGHT_MAX_SCHEDULE_RULES; ++i) {
    _rules[i] = {0, 0, 0};
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

  if (!_enabled || _ruleCount == 0) {
    _nextEvent = "not_scheduled";
    for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
      _states[i] = relayModeToString(relays.mode(i));
    }
    return;
  }

  struct tm timeInfo;
  if (!getLocalTime(&timeInfo, 50)) {
    _nextEvent = "time_invalid";
    for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
      _states[i] = (relays.mode(i) == RelayMode::Schedule || relays.mode(i) == RelayMode::Astro) ? "paused_time_invalid" : relayModeToString(relays.mode(i));
    }
    return;
  }

  const uint16_t nowMinute = static_cast<uint16_t>((timeInfo.tm_hour * 60) + timeInfo.tm_min);
  bool ruleApplied[EZLIGHT_RELAY_COUNT] = {false, false, false, false};
  uint16_t soonestDelta = 1441;
  String soonestEvent = "none";

  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    _states[i] = relayModeToString(relays.mode(i));
  }

  for (uint8_t i = 0; i < _ruleCount; ++i) {
    const FixedScheduleRule& rule = _rules[i];
    if (rule.relayIndex >= EZLIGHT_RELAY_COUNT || relays.mode(rule.relayIndex) != RelayMode::Schedule) {
      continue;
    }

    const bool targetOn = ruleIsActive(rule, nowMinute);
    relays.setRelay(relays.definition(rule.relayIndex).id, targetOn);
    _states[rule.relayIndex] = targetOn ? "scheduled_on" : "scheduled_off";
    ruleApplied[rule.relayIndex] = true;

    const uint16_t eventMinute = targetOn ? rule.offMinute : rule.onMinute;
    uint16_t delta = minutesUntil(nowMinute, eventMinute);
    if (delta == 0) {
      delta = 1440;
    }
    if (delta < soonestDelta) {
      soonestDelta = delta;
      soonestEvent = formatEvent(relays, rule, !targetOn);
    }
  }

  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    if (relays.mode(i) == RelayMode::Schedule && !ruleApplied[i]) {
      _states[i] = "schedule_no_rule";
    }
  }
  _nextEvent = soonestEvent;
}

const String& ScheduleEngine::nextEvent() const { return _nextEvent; }
const String& ScheduleEngine::stateFor(uint8_t index) const { return _states[index < EZLIGHT_RELAY_COUNT ? index : 0]; }

bool ScheduleEngine::ruleIsActive(const FixedScheduleRule& rule, uint16_t nowMinute) const {
  if (rule.onMinute < rule.offMinute) {
    return nowMinute >= rule.onMinute && nowMinute < rule.offMinute;
  }
  return nowMinute >= rule.onMinute || nowMinute < rule.offMinute;
}

uint16_t ScheduleEngine::minutesUntil(uint16_t nowMinute, uint16_t eventMinute) const {
  if (eventMinute >= nowMinute) {
    return static_cast<uint16_t>(eventMinute - nowMinute);
  }
  return static_cast<uint16_t>((1440 - nowMinute) + eventMinute);
}

String ScheduleEngine::formatEvent(const RelayDriver& relays, const FixedScheduleRule& rule, bool nextOn) const {
  const uint16_t minute = nextOn ? rule.onMinute : rule.offMinute;
  char timeBuffer[6];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02u:%02u", minute / 60, minute % 60);
  return String(relays.definition(rule.relayIndex).id) + "@" + timeBuffer + "=" + (nextOn ? "on" : "off");
}
