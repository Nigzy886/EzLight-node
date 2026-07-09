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
  bool scheduleGloballyEnabled;
  bool scheduleEnabled[EZLIGHT_RELAY_COUNT];
  FixedScheduleRule scheduleRules[EZLIGHT_MAX_SCHEDULE_RULES];
  uint8_t scheduleRuleCount;
  bool astroLocationSet;
  double latitude;
  double longitude;
  AstroRule astroRules[EZLIGHT_MAX_ASTRO_RULES];
  uint8_t astroRuleCount;
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
  bool parseScheduleConfig(JsonObject schedule, EzLightConfig& config, String& error) const;
  bool parseScheduleRule(JsonObject rule, EzLightConfig& config, uint8_t ruleIndex, bool usedRelayRules[EZLIGHT_RELAY_COUNT], String& error) const;
  bool parseAstroConfig(JsonObject astro, EzLightConfig& config, String& error) const;
  bool parseAstroRule(JsonObject rule, EzLightConfig& config, uint8_t ruleIndex, bool usedRelayRules[EZLIGHT_RELAY_COUNT], String& error) const;
  bool parseAstroEvent(const char* value, AstroEvent& event) const;
  bool parseTimeOfDay(const char* value, uint16_t& minuteOfDay) const;
  bool rejectLoad(const String& error);
  bool validateRelayDefaults(const EzLightConfig& config, String& error) const;
};
