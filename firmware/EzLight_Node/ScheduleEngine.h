#pragma once

#include <Arduino.h>
#include "RelayDriver.h"

class ScheduleEngine {
public:
  ScheduleEngine();
  void update(bool timeValid, RelayDriver& relays);
  const String& nextEvent() const;
  const String& stateFor(uint8_t index) const;

private:
  String _nextEvent;
  String _states[EZLIGHT_RELAY_COUNT];
};
