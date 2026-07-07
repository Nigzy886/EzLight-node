#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "EzHubCapabilities.h"
#include "TelemetryReporter.h"
#include "CommandHandler.h"
#include "RelayDriver.h"
#include "ScheduleEngine.h"
#include "AstroEngine.h"
#include "TimeService.h"
#include "ConfigStore.h"

class WebRoutes {
public:
  WebRoutes(WebServer& server, EzHubCapabilities& capabilities, TelemetryReporter& telemetry, CommandHandler& commands, RelayDriver& relays, ScheduleEngine& schedule, AstroEngine& astro, TimeService& timeService, ConfigStore& configStore);
  void begin();
  void handleClient();

private:
  WebServer& _server;
  EzHubCapabilities& _capabilities;
  TelemetryReporter& _telemetry;
  CommandHandler& _commands;
  RelayDriver& _relays;
  ScheduleEngine& _schedule;
  AstroEngine& _astro;
  TimeService& _timeService;
  ConfigStore& _configStore;
  void sendStatusPage();
  void sendJson(uint16_t code, const String& json);
};
