#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "EzLightTypes.h"

struct EzLightConfig {
  String nodeId;
  String nodeType;
  String profile;
  String timeSource;
  bool rtcRequired;
  String astroConvention;
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
  const String& configSource() const;
  bool configLoaded() const;

private:
  EzLightConfig _current;
  String _lastError;
  String _configSource;
  bool _configLoaded;

  bool parseMode(const char* value, RelayMode& mode) const;
  bool parseRelayConfig(JsonObject relay, uint8_t index, EzLightConfig& config, String& error) const;
  bool rejectLoad(const String& error);
  bool validateRelayDefaults(const EzLightConfig& config, String& error) const;
};
