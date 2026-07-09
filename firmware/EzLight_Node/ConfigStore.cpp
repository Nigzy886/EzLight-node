#include "ConfigStore.h"
#include <LittleFS.h>

ConfigStore::ConfigStore()
  : _current(defaults()), _lastError(""), _configSource("defaults"), _configLoaded(false) {}

bool ConfigStore::begin() {
  if (!LittleFS.begin(true)) {
    _lastError = "littlefs_mount_failed";
    _configSource = "defaults_littlefs_failed";
    _configLoaded = false;
    return false;
  }
  return true;
}

EzLightConfig ConfigStore::defaults() const {
  EzLightConfig config;
  config.nodeId = EZLIGHT_NODE_ID;
  config.nodeType = EZLIGHT_NODE_TYPE;
  config.profile = EZLIGHT_PROFILE;
  config.timeSource = "ntp";
  config.rtcRequired = false;
  config.astroConvention = "civil_dusk_civil_dawn";
  config.relays[0] = {"path_lights", "Path lights", 14, true};
  config.relays[1] = {"driveway_lights", "Driveway lights", 27, true};
  config.relays[2] = {"entrance_lights", "Entrance lights", 26, true};
  config.relays[3] = {"spare_lights", "Spare lights", 25, true};
  config.timezone = "NZST-12NZDT,M9.5.0,M4.1.0/3";
  config.scheduleGloballyEnabled = false;
  config.scheduleRuleCount = 0;
  config.astroLocationSet = false;
  config.latitude = 0.0;
  config.longitude = 0.0;
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    config.modes[i] = RelayMode::Manual;
    config.scheduleEnabled[i] = false;
  }
  return config;
}

bool ConfigStore::load(EzLightConfig& outConfig) {
  outConfig = defaults();
  if (!LittleFS.exists("/config.json")) {
    _configSource = "defaults_missing_config";
    _configLoaded = false;
    _lastError = "";
    return true;
  }

  File file = LittleFS.open("/config.json", "r");
  if (!file) {
    return rejectLoad("config_open_failed");
  }

  JsonDocument doc;
  DeserializationError parseError = deserializeJson(doc, file);
  file.close();
  if (parseError) {
    return rejectLoad("config_malformed_json");
  }
  if (!doc.is<JsonObject>()) {
    return rejectLoad("config_wrong_shape");
  }

  JsonObject root = doc.as<JsonObject>();
  if (!root["pir"].isNull() || !root["lux_sensor"].isNull() || !root["current_sensing"].isNull() || !root["holiday_mode"].isNull() || !root["random_holiday_mode"].isNull() || !root["wall_switches"].isNull()) {
    return rejectLoad("unsupported_v0_1_feature");
  }
  if (!root["node_id"].is<const char*>() || !root["node_type"].is<const char*>() || !root["profile"].is<const char*>()) {
    return rejectLoad("missing_identity_fields");
  }
  if (!root["timezone"].is<const char*>() || !root["time_source"].is<const char*>() || !root["rtc_required"].is<bool>() || !root["astro_convention"].is<const char*>()) {
    return rejectLoad("missing_time_fields");
  }

  outConfig.nodeId = root["node_id"].as<const char*>();
  outConfig.nodeType = root["node_type"].as<const char*>();
  outConfig.profile = root["profile"].as<const char*>();
  outConfig.timezone = root["timezone"].as<const char*>();
  outConfig.timeSource = root["time_source"].as<const char*>();
  outConfig.rtcRequired = root["rtc_required"].as<bool>();
  outConfig.astroConvention = root["astro_convention"].as<const char*>();

  if (!root["relays"].is<JsonArray>()) {
    return rejectLoad("missing_relays");
  }
  JsonArray relays = root["relays"].as<JsonArray>();
  if (relays.size() != EZLIGHT_RELAY_COUNT) {
    return rejectLoad("invalid_relay_count");
  }
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    if (!relays[i].is<JsonObject>()) {
      return rejectLoad("invalid_relay_shape");
    }
    String relayError;
    if (!parseRelayConfig(relays[i].as<JsonObject>(), i, outConfig, relayError)) {
      return rejectLoad(relayError);
    }
  }

  if (!root["schedule"].is<JsonObject>()) {
    return rejectLoad("missing_schedule");
  }
  String scheduleError;
  if (!parseScheduleConfig(root["schedule"].as<JsonObject>(), outConfig, scheduleError)) {
    return rejectLoad(scheduleError);
  }

  if (!root["astro"].is<JsonObject>()) {
    return rejectLoad("missing_astro");
  }
  JsonObject astro = root["astro"].as<JsonObject>();
  if (!astro["location_set"].is<bool>()) {
    return rejectLoad("missing_astro_location_flag");
  }
  outConfig.astroLocationSet = astro["location_set"].as<bool>();
  if (outConfig.astroLocationSet) {
    if (!astro["latitude"].is<double>() || !astro["longitude"].is<double>()) {
      return rejectLoad("missing_astro_location");
    }
    outConfig.latitude = astro["latitude"].as<double>();
    outConfig.longitude = astro["longitude"].as<double>();
  } else {
    outConfig.latitude = 0.0;
    outConfig.longitude = 0.0;
  }

  String validationError;
  if (!validate(outConfig, validationError)) {
    return rejectLoad(validationError);
  }

  _lastError = "";
  _configSource = "/config.json";
  _configLoaded = true;
  return true;
}

bool ConfigStore::validate(const EzLightConfig& config, String& error) const {
  if (config.nodeId.length() == 0) {
    error = "missing_node_id";
    return false;
  }
  if (config.nodeId != EZLIGHT_NODE_ID || config.nodeType != EZLIGHT_NODE_TYPE || config.profile != EZLIGHT_PROFILE) {
    error = "invalid_identity";
    return false;
  }
  if (config.timezone.length() == 0) {
    error = "missing_timezone";
    return false;
  }
  if (config.timeSource != "ntp") {
    error = "unsupported_time_source";
    return false;
  }
  if (config.rtcRequired) {
    error = "rtc_not_supported_v0_1";
    return false;
  }
  if (config.astroConvention != "civil_dusk_civil_dawn") {
    error = "unsupported_astro_convention";
    return false;
  }
  if (!validateRelayDefaults(config, error)) {
    return false;
  }
  if (config.astroLocationSet && (config.latitude < -90.0 || config.latitude > 90.0 || config.longitude < -180.0 || config.longitude > 180.0)) {
    error = "invalid_astro_location";
    return false;
  }
  if (config.scheduleGloballyEnabled && config.scheduleRuleCount == 0) {
    error = "missing_schedule_rules";
    return false;
  }
  if (config.scheduleRuleCount > EZLIGHT_MAX_SCHEDULE_RULES) {
    error = "too_many_schedule_rules";
    return false;
  }
  bool usedRelayRules[EZLIGHT_RELAY_COUNT] = {false, false, false, false};
  for (uint8_t i = 0; i < config.scheduleRuleCount; ++i) {
    const FixedScheduleRule& rule = config.scheduleRules[i];
    if (rule.relayIndex >= EZLIGHT_RELAY_COUNT) {
      error = "unknown_schedule_relay";
      return false;
    }
    if (config.modes[rule.relayIndex] == RelayMode::Disabled) {
      error = "schedule_targets_disabled_relay";
      return false;
    }
    if (config.modes[rule.relayIndex] != RelayMode::Schedule) {
      error = "schedule_targets_non_schedule_relay";
      return false;
    }
    if (usedRelayRules[rule.relayIndex]) {
      error = "duplicate_schedule_rule";
      return false;
    }
    if (rule.onMinute >= 1440 || rule.offMinute >= 1440 || rule.onMinute == rule.offMinute) {
      error = "invalid_schedule_window";
      return false;
    }
    usedRelayRules[rule.relayIndex] = true;
  }
  return true;
}

bool ConfigStore::applyIfValid(const EzLightConfig& candidate, String& error) {
  if (!validate(candidate, error)) {
    _lastError = error;
    return false;
  }
  _current = candidate;
  _lastError = "";
  if (_configSource.length() == 0) {
    _configSource = "defaults";
  }
  return true;
}

const EzLightConfig& ConfigStore::current() const { return _current; }
const String& ConfigStore::lastError() const { return _lastError; }
const String& ConfigStore::configSource() const { return _configSource; }
bool ConfigStore::configLoaded() const { return _configLoaded; }

bool ConfigStore::parseMode(const char* value, RelayMode& mode) const {
  if (value == nullptr) {
    return false;
  }
  const String modeValue(value);
  if (modeValue == "manual") {
    mode = RelayMode::Manual;
    return true;
  }
  if (modeValue == "schedule") {
    mode = RelayMode::Schedule;
    return true;
  }
  if (modeValue == "astro") {
    mode = RelayMode::Astro;
    return true;
  }
  if (modeValue == "disabled") {
    mode = RelayMode::Disabled;
    return true;
  }
  return false;
}

bool ConfigStore::parseRelayConfig(JsonObject relay, uint8_t index, EzLightConfig& config, String& error) const {
  if (!relay["id"].is<const char*>() || !relay["gpio"].is<int>() || !relay["active_low"].is<bool>() || !relay["mode"].is<const char*>()) {
    error = "invalid_relay_shape";
    return false;
  }
  const char* expectedIds[EZLIGHT_RELAY_COUNT] = {"path_lights", "driveway_lights", "entrance_lights", "spare_lights"};
  if (String(relay["id"].as<const char*>()) != expectedIds[index]) {
    error = "invalid_relay_id";
    return false;
  }
  const uint8_t expectedGpios[EZLIGHT_RELAY_COUNT] = {14, 27, 26, 25};
  const int parsedGpio = relay["gpio"].as<int>();
  if (parsedGpio != expectedGpios[index]) {
    error = "invalid_relay_gpio";
    return false;
  }
  RelayMode mode;
  if (!parseMode(relay["mode"].as<const char*>(), mode)) {
    error = "invalid_relay_mode";
    return false;
  }
  config.relays[index].gpio = static_cast<uint8_t>(parsedGpio);
  config.relays[index].activeLow = relay["active_low"].as<bool>();
  config.modes[index] = mode;
  return true;
}

bool ConfigStore::parseScheduleConfig(JsonObject schedule, EzLightConfig& config, String& error) const {
  if (!schedule["enabled"].is<bool>()) {
    error = "invalid_schedule_config";
    return false;
  }
  if (!schedule["storage"].isNull() && (!schedule["storage"].is<const char*>() || String(schedule["storage"].as<const char*>()) != "littlefs_json")) {
    error = "invalid_schedule_config";
    return false;
  }
  if (!schedule["events"].isNull()) {
    if (!schedule["events"].is<JsonArray>()) {
      error = "missing_schedule_events";
      return false;
    }
    if (schedule["events"].as<JsonArray>().size() > 0) {
      error = "schedule_execution_not_supported_v0_1";
      return false;
    }
  }

  config.scheduleGloballyEnabled = schedule["enabled"].as<bool>();
  config.scheduleRuleCount = 0;
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    config.scheduleEnabled[i] = config.scheduleGloballyEnabled;
  }

  const bool hasRules = schedule["rules"].is<JsonArray>();
  if (config.scheduleGloballyEnabled && !hasRules) {
    error = "missing_schedule_rules";
    return false;
  }
  if (!schedule["rules"].isNull() && !hasRules) {
    error = "invalid_schedule_rules";
    return false;
  }
  if (!hasRules) {
    return true;
  }

  JsonArray rules = schedule["rules"].as<JsonArray>();
  if (rules.size() > EZLIGHT_MAX_SCHEDULE_RULES) {
    error = "too_many_schedule_rules";
    return false;
  }

  bool usedRelayRules[EZLIGHT_RELAY_COUNT] = {false, false, false, false};
  for (JsonVariant ruleVariant : rules) {
    if (!ruleVariant.is<JsonObject>()) {
      error = "invalid_schedule_rule";
      return false;
    }
    if (!parseScheduleRule(ruleVariant.as<JsonObject>(), config, config.scheduleRuleCount, usedRelayRules, error)) {
      return false;
    }
    config.scheduleRuleCount++;
  }

  if (config.scheduleGloballyEnabled && config.scheduleRuleCount == 0) {
    error = "missing_schedule_rules";
    return false;
  }
  return true;
}

bool ConfigStore::parseScheduleRule(JsonObject rule, EzLightConfig& config, uint8_t ruleIndex, bool usedRelayRules[EZLIGHT_RELAY_COUNT], String& error) const {
  if (!rule["relay_id"].is<const char*>() || !rule["on"].is<const char*>() || !rule["off"].is<const char*>()) {
    error = "invalid_schedule_rule";
    return false;
  }

  const String relayId = rule["relay_id"].as<const char*>();
  int relayIndex = -1;
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    if (relayId == config.relays[i].id) {
      relayIndex = i;
      break;
    }
  }
  if (relayIndex < 0) {
    error = "unknown_schedule_relay";
    return false;
  }

  if (config.modes[relayIndex] == RelayMode::Disabled) {
    error = "schedule_targets_disabled_relay";
    return false;
  }
  if (config.modes[relayIndex] != RelayMode::Schedule) {
    error = "schedule_targets_non_schedule_relay";
    return false;
  }
  if (usedRelayRules[relayIndex]) {
    error = "duplicate_schedule_rule";
    return false;
  }

  uint16_t onMinute = 0;
  uint16_t offMinute = 0;
  if (!parseTimeOfDay(rule["on"].as<const char*>(), onMinute) || !parseTimeOfDay(rule["off"].as<const char*>(), offMinute)) {
    error = "invalid_schedule_time";
    return false;
  }
  if (onMinute == offMinute) {
    error = "invalid_schedule_window";
    return false;
  }

  usedRelayRules[relayIndex] = true;
  config.scheduleRules[ruleIndex] = {static_cast<uint8_t>(relayIndex), onMinute, offMinute};
  return true;
}

bool ConfigStore::parseTimeOfDay(const char* value, uint16_t& minuteOfDay) const {
  if (value == nullptr) {
    return false;
  }
  const String text(value);
  if (text.length() != 5 || text.charAt(2) != ':') {
    return false;
  }
  for (uint8_t i = 0; i < 5; ++i) {
    if (i == 2) {
      continue;
    }
    if (!isDigit(text.charAt(i))) {
      return false;
    }
  }
  const int hour = text.substring(0, 2).toInt();
  const int minute = text.substring(3, 5).toInt();
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
    return false;
  }
  minuteOfDay = static_cast<uint16_t>((hour * 60) + minute);
  return true;
}

bool ConfigStore::rejectLoad(const String& error) {
  _lastError = error;
  _configSource = "rejected_config_defaults_active";
  _configLoaded = false;
  return false;
}

bool ConfigStore::validateRelayDefaults(const EzLightConfig& config, String& error) const {
  const char* expectedIds[EZLIGHT_RELAY_COUNT] = {"path_lights", "driveway_lights", "entrance_lights", "spare_lights"};
  const uint8_t expectedGpios[EZLIGHT_RELAY_COUNT] = {14, 27, 26, 25};
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    if (String(config.relays[i].id) != expectedIds[i]) {
      error = "invalid_relay_id";
      return false;
    }
    if (config.relays[i].gpio != expectedGpios[i]) {
      error = "invalid_relay_gpio";
      return false;
    }
    if (!config.relays[i].activeLow) {
      error = "invalid_relay_polarity";
      return false;
    }
  }
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    for (uint8_t j = i + 1; j < EZLIGHT_RELAY_COUNT; ++j) {
      if (String(config.relays[i].id) == config.relays[j].id) {
        error = "duplicate_relay_id";
        return false;
      }
      if (config.relays[i].gpio == config.relays[j].gpio) {
        error = "duplicate_relay_gpio";
        return false;
      }
    }
  }
  return true;
}
