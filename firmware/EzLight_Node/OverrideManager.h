#pragma once

#include <Arduino.h>
#include "RelayDriver.h"

class OverrideManager {
public:
  CommandResult applyOverride(const String& relayId, const String& overrideMode, RelayDriver& relays);
  CommandResult resumeSchedule(const String& relayId, RelayDriver& relays);
  void update(RelayDriver& relays);

private:
  OverrideMode parseMode(const String& overrideMode, unsigned long& durationMs) const;
};
