#pragma once

#include <Arduino.h>
#include "RelayDriver.h"
#include "ScheduleEngine.h"
#include "AstroEngine.h"
#include "TimeService.h"

class TelemetryReporter {
public:
  String stateJson(const String& nodeId, bool provisioned, const String& ipAddress, const String& hostName, int32_t wifiRssi, const RelayDriver& relays, const ScheduleEngine& schedule, const AstroEngine& astro, const TimeService& timeService) const;
};
