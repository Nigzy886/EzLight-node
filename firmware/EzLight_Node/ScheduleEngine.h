#pragma once

#include <Arduino.h>
#include "ConfigStore.h"
#include "RelayDriver.h"

class ScheduleEngine {
public:
  ScheduleEngine();
  void configure(const EzLightConfig& config);
  void update(bool timeValid, RelayDriver& relays);
  const String& nextEvent() const;
  const String& stateFor(uint8_t index) const;

private:
  bool _enabled;
  uint8_t _ruleCount;
  FixedScheduleRule _rules[EZLIGHT_MAX_SCHEDULE_RULES];
  String _nextEvent;
  String _states[EZLIGHT_RELAY_COUNT];

  bool ruleIsActive(const FixedScheduleRule& rule, uint16_t nowMinute) const;
  uint16_t minutesUntil(uint16_t nowMinute, uint16_t eventMinute) const;
  String formatEvent(const RelayDriver& relays, const FixedScheduleRule& rule, bool nextOn) const;
};
