#pragma once

#include <Arduino.h>
#include "RelayDriver.h"
#include "OverrideManager.h"
#include "AstroEngine.h"
#include "ConfigStore.h"
#include "TimeService.h"

class CommandHandler {
public:
  CommandHandler(RelayDriver& relays, OverrideManager& overrides, AstroEngine& astro, ConfigStore& config, TimeService& timeService);
  String handle(const String& body);

private:
  RelayDriver& _relays;
  OverrideManager& _overrides;
  AstroEngine& _astro;
  ConfigStore& _config;
  TimeService& _timeService;
  bool declared(const String& target, const String& action) const;
  String extractString(const String& body, const String& key) const;
  bool extractBool(const String& body, const String& key, bool defaultValue) const;
  String ack(const String& refMsgId, const CommandResult& result) const;
  String nack(const String& refMsgId, const char* error) const;
};
