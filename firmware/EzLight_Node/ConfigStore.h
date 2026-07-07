#pragma once

#include <Arduino.h>
#include "EzLightTypes.h"

struct EzLightConfig {
  String nodeId;
  RelayDefinition relays[EZLIGHT_RELAY_COUNT];
  RelayMode modes[EZLIGHT_RELAY_COUNT];
  String timezone;
  bool scheduleEnabled[EZLIGHT_RELAY_COUNT];
  bool astroLocationSet;
  double latitude;
  double longitude;
};

class ConfigStore {
public:
  ConfigStore();
  bool begin();
  EzLightConfig defaults() const;
  bool load(EzLightConfig& outConfig);
  bool validate(const EzLightConfig& config, String& error) const;
  bool applyIfValid(const EzLightConfig& candidate, String& error);
  const EzLightConfig& current() const;
  const String& lastError() const;

private:
  EzLightConfig _current;
  String _lastError;
  bool validateRelayDefaults(const EzLightConfig& config, String& error) const;
};
