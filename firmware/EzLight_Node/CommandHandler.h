#pragma once

#include <Arduino.h>
#include "RelayDriver.h"
#include "OverrideManager.h"
#include "AstroEngine.h"
#include "ConfigStore.h"
#include "ScheduleEngine.h"
#include "TimeService.h"

class CommandHandler {
public:
  CommandHandler(RelayDriver& relays, OverrideManager& overrides, AstroEngine& astro, ConfigStore& config, ScheduleEngine& schedule, TimeService& timeService);
  String handle(const String& body, const String& nodeId);
  bool takeRebootRequested();

private:
  RelayDriver& _relays;
  OverrideManager& _overrides;
  AstroEngine& _astro;
  ConfigStore& _config;
  ScheduleEngine& _schedule;
  TimeService& _timeService;
  bool _rebootRequested;
  String _lastMsgId;
  String _lastAck;

  bool declared(const String& target, const String& action) const;
  String remember(const String& msgId, const String& response);
  String ack(const String& refMsgId, const CommandResult& result) const;
  String nack(const String& refMsgId, const char* error) const;
  bool persistConfig(const EzLightConfig& candidate, String& error);
  void applyConfigToRuntime(const EzLightConfig& config);
  bool parseTimeOfDay(const String& value, uint16_t& minuteOfDay) const;
  int relayIndex(const String& relayId, const EzLightConfig& config) const;
};
