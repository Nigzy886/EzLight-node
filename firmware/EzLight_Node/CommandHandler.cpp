#include "CommandHandler.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "EzLightTypes.h"

namespace {
String jsonEscape(String value) {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  value.replace("\n", "\\n");
  value.replace("\r", "\\r");
  return value;
}

String quote(const String& value) {
  return "\"" + jsonEscape(value) + "\"";
}

String stringValue(JsonObject object, const char* key) {
  return object[key].is<const char*>() ? String(object[key].as<const char*>()) : String();
}

const char* astroEventToString(AstroEvent event) {
  return event == AstroEvent::CivilDawn ? "civil_dawn" : "civil_dusk";
}

bool parseAstroEventText(const String& value, AstroEvent& event) {
  if (value == "civil_dawn") {
    event = AstroEvent::CivilDawn;
    return true;
  }
  if (value == "civil_dusk") {
    event = AstroEvent::CivilDusk;
    return true;
  }
  return false;
}

String minuteToText(uint16_t minuteOfDay) {
  char buffer[6];
  snprintf(buffer, sizeof(buffer), "%02u:%02u", minuteOfDay / 60, minuteOfDay % 60);
  return String(buffer);
}

void removeScheduleRulesForRelay(EzLightConfig& config, uint8_t relayIndex) {
  uint8_t writeIndex = 0;
  for (uint8_t i = 0; i < config.scheduleRuleCount; ++i) {
    if (config.scheduleRules[i].relayIndex != relayIndex) {
      config.scheduleRules[writeIndex++] = config.scheduleRules[i];
    }
  }
  config.scheduleRuleCount = writeIndex;
  config.scheduleEnabled[relayIndex] = false;
  if (config.scheduleRuleCount == 0) {
    config.scheduleGloballyEnabled = false;
  }
}

void removeAstroRulesForRelay(EzLightConfig& config, uint8_t relayIndex) {
  uint8_t writeIndex = 0;
  for (uint8_t i = 0; i < config.astroRuleCount; ++i) {
    if (config.astroRules[i].relayIndex != relayIndex) {
      config.astroRules[writeIndex++] = config.astroRules[i];
    }
  }
  config.astroRuleCount = writeIndex;
}

void removeAutomationRulesForRelay(EzLightConfig& config, uint8_t relayIndex) {
  removeScheduleRulesForRelay(config, relayIndex);
  removeAstroRulesForRelay(config, relayIndex);
}
}  // namespace

CommandHandler::CommandHandler(RelayDriver& relays, OverrideManager& overrides, AstroEngine& astro, ConfigStore& config, ScheduleEngine& schedule, TimeService& timeService)
  : _relays(relays),
    _overrides(overrides),
    _astro(astro),
    _config(config),
    _schedule(schedule),
    _timeService(timeService),
    _rebootRequested(false) {}

String CommandHandler::handle(const String& body, const String& nodeId) {
  JsonDocument doc;
  const DeserializationError parseError = deserializeJson(doc, body);
  if (parseError || !doc.is<JsonObject>()) {
    return nack("", "invalid_args");
  }

  JsonObject root = doc.as<JsonObject>();
  const String eco = stringValue(root, "eco");
  const String type = stringValue(root, "type");
  const String msgId = stringValue(root, "msg_id");
  const String targetNodeId = stringValue(root, "node_id");
  const String target = stringValue(root, "target");
  const String action = stringValue(root, "action");

  if ((eco.length() > 0 && eco != "easysystems") || (type.length() > 0 && type != "cmd")) {
    return remember(msgId, nack(msgId, "invalid_args"));
  }
  if (msgId.length() == 0 || targetNodeId.length() == 0 || target.length() == 0 || action.length() == 0) {
    return remember(msgId, nack(msgId, "invalid_args"));
  }
  if (targetNodeId != nodeId) {
    return remember(msgId, nack(msgId, "wrong_target"));
  }
  if (msgId == _lastMsgId && _lastAck.length() > 0) {
    return _lastAck;
  }
  if (!declared(target, action)) {
    return remember(msgId, nack(msgId, "not_declared"));
  }

  JsonObject params = root["params"].as<JsonObject>();
  if (params.isNull()) {
    params = root;
  }

  if (target == "relay" && action == "set") {
    const String relayId = stringValue(params, "relay_id");
    JsonVariant stateValue = params["state"];
    if (stateValue.isNull()) {
      stateValue = params["value"];
    }
    const int index = _relays.indexOf(relayId);
    if (index < 0 || !stateValue.is<bool>()) {
      return remember(msgId, nack(msgId, "invalid_args"));
    }
    if (_relays.mode(index) == RelayMode::Disabled) {
      return remember(msgId, nack(msgId, "unsafe_state"));
    }

    const bool requestedState = stateValue.as<bool>();
    if (!_relays.setRelay(relayId, requestedState)) {
      return remember(msgId, nack(msgId, "invalid_args"));
    }
    return remember(msgId, ack(msgId, {true, "accepted", nullptr, String("{\"relay_id\":") + quote(relayId) + ",\"relay_state\":" + (requestedState ? "true" : "false") + "}"}));
  }

  if (target == "relay" && action == "toggle") {
    const String relayId = stringValue(params, "relay_id");
    const int index = _relays.indexOf(relayId);
    if (index < 0) {
      return remember(msgId, nack(msgId, "invalid_args"));
    }
    if (_relays.mode(index) == RelayMode::Disabled) {
      return remember(msgId, nack(msgId, "unsafe_state"));
    }
    if (!_relays.toggleRelay(relayId)) {
      return remember(msgId, nack(msgId, "invalid_args"));
    }
    return remember(msgId, ack(msgId, {true, "accepted", nullptr, String("{\"relay_id\":") + quote(relayId) + ",\"relay_state\":" + (_relays.logicalState(index) ? "true" : "false") + "}"}));
  }

  if (target == "relay" && action == "set_mode") {
    const String relayId = stringValue(params, "relay_id");
    const String modeText = stringValue(params, "mode");
    const int index = relayIndex(relayId, _config.current());
    if (index < 0 || (modeText != "manual" && modeText != "disabled")) {
      return remember(msgId, nack(msgId, "invalid_args"));
    }

    // A mode change is fail-safe: de-energize before touching persistent state.
    _relays.setRelay(relayId, false);

    EzLightConfig candidate = _config.current();
    removeAutomationRulesForRelay(candidate, static_cast<uint8_t>(index));
    candidate.modes[index] = modeText == "disabled" ? RelayMode::Disabled : RelayMode::Manual;

    String configError;
    if (!persistConfig(candidate, configError)) {
      Serial.println("relay.set_mode rejected: " + configError);
      return remember(msgId, nack(msgId, "invalid_args"));
    }
    applyConfigToRuntime(candidate);
    return remember(msgId, ack(msgId, {true, "accepted", nullptr, String("{\"relay_id\":") + quote(relayId) + ",\"mode\":" + quote(modeText) + ",\"relay_state\":false}"}));
  }

  if (target == "relay" && action == "override") {
    const CommandResult result = _overrides.applyOverride(stringValue(params, "relay_id"), stringValue(params, "override_mode"), _relays);
    return remember(msgId, ack(msgId, result));
  }

  if (target == "relay" && action == "resume_schedule") {
    const CommandResult result = _overrides.resumeSchedule(stringValue(params, "relay_id"), _relays);
    return remember(msgId, ack(msgId, result));
  }

  if (target == "schedule" && action == "set") {
    if (!params["enabled"].is<bool>() || !params["rules"].is<JsonArray>()) {
      return remember(msgId, nack(msgId, "invalid_args"));
    }

    JsonArray rules = params["rules"].as<JsonArray>();
    if (rules.size() > EZLIGHT_MAX_SCHEDULE_RULES) {
      return remember(msgId, nack(msgId, "invalid_args"));
    }

    EzLightConfig candidate = _config.current();
    candidate.scheduleGloballyEnabled = params["enabled"].as<bool>();
    candidate.scheduleRuleCount = 0;
    bool usedRelays[EZLIGHT_RELAY_COUNT] = {false, false, false, false};

    for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
      candidate.scheduleEnabled[i] = false;
      if (candidate.modes[i] == RelayMode::Schedule) {
        candidate.modes[i] = RelayMode::Manual;
      }
    }

    for (JsonVariant ruleVariant : rules) {
      if (!ruleVariant.is<JsonObject>()) {
        return remember(msgId, nack(msgId, "invalid_args"));
      }
      JsonObject rule = ruleVariant.as<JsonObject>();
      const String relayId = stringValue(rule, "relay_id");
      const String onText = stringValue(rule, "on");
      const String offText = stringValue(rule, "off");
      const int index = relayIndex(relayId, candidate);
      uint16_t onMinute = 0;
      uint16_t offMinute = 0;

      if (index < 0 || usedRelays[index] || !parseTimeOfDay(onText, onMinute) || !parseTimeOfDay(offText, offMinute) || onMinute == offMinute) {
        return remember(msgId, nack(msgId, "invalid_args"));
      }
      if (_config.current().modes[index] == RelayMode::Disabled) {
        return remember(msgId, nack(msgId, "unsafe_state"));
      }

      usedRelays[index] = true;
      removeAstroRulesForRelay(candidate, static_cast<uint8_t>(index));
      candidate.modes[index] = RelayMode::Schedule;
      candidate.scheduleEnabled[index] = true;
      candidate.scheduleRules[candidate.scheduleRuleCount++] = {static_cast<uint8_t>(index), onMinute, offMinute};
    }

    String configError;
    if (!persistConfig(candidate, configError)) {
      Serial.println("schedule.set rejected: " + configError);
      return remember(msgId, nack(msgId, "invalid_args"));
    }
    applyConfigToRuntime(candidate);
    return remember(msgId, ack(msgId, {true, "accepted", nullptr, String("{\"schedule_enabled\":") + (candidate.scheduleGloballyEnabled ? "true" : "false") + ",\"rule_count\":" + String(candidate.scheduleRuleCount) + "}"}));
  }

  if (target == "schedule" && (action == "enable" || action == "disable")) {
    EzLightConfig candidate = _config.current();
    candidate.scheduleGloballyEnabled = action == "enable";

    String configError;
    if (!persistConfig(candidate, configError)) {
      Serial.println("schedule command rejected: " + configError);
      return remember(msgId, nack(msgId, "invalid_args"));
    }
    applyConfigToRuntime(candidate);
    return remember(msgId, ack(msgId, {true, "accepted", nullptr, String("{\"schedule_enabled\":") + (candidate.scheduleGloballyEnabled ? "true" : "false") + "}"}));
  }

  if (target == "astro" && action == "set_location") {
    const JsonVariant latitudeValue = params["latitude"];
    const JsonVariant longitudeValue = params["longitude"];
    if (!latitudeValue.is<double>() || !longitudeValue.is<double>()) {
      return remember(msgId, nack(msgId, "invalid_args"));
    }

    const double latitude = latitudeValue.as<double>();
    const double longitude = longitudeValue.as<double>();
    if (latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0) {
      return remember(msgId, nack(msgId, "invalid_args"));
    }

    EzLightConfig candidate = _config.current();
    candidate.astroLocationSet = true;
    candidate.latitude = latitude;
    candidate.longitude = longitude;

    String configError;
    if (!persistConfig(candidate, configError)) {
      Serial.println("astro.set_location rejected: " + configError);
      return remember(msgId, nack(msgId, "invalid_args"));
    }
    applyConfigToRuntime(candidate);
    _astro.recalculate(_timeService.valid());
    return remember(msgId, ack(msgId, {true, "accepted", nullptr, String("{\"latitude\":") + String(latitude, 6) + ",\"longitude\":" + String(longitude, 6) + "}"}));
  }

  if (target == "astro" && action == "set_rules") {
    if (!params["rules"].is<JsonArray>()) {
      return remember(msgId, nack(msgId, "invalid_args"));
    }

    JsonArray rules = params["rules"].as<JsonArray>();
    if (rules.size() > EZLIGHT_MAX_ASTRO_RULES) {
      return remember(msgId, nack(msgId, "invalid_args"));
    }

    EzLightConfig candidate = _config.current();
    candidate.astroRuleCount = 0;
    bool usedRelays[EZLIGHT_RELAY_COUNT] = {false, false, false, false};

    for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
      if (candidate.modes[i] == RelayMode::Astro) {
        candidate.modes[i] = RelayMode::Manual;
      }
    }

    if (rules.size() > 0 && !candidate.astroLocationSet) {
      return remember(msgId, nack(msgId, "invalid_args"));
    }

    for (JsonVariant ruleVariant : rules) {
      if (!ruleVariant.is<JsonObject>()) {
        return remember(msgId, nack(msgId, "invalid_args"));
      }

      JsonObject rule = ruleVariant.as<JsonObject>();
      const String relayId = stringValue(rule, "relay_id");
      const String onText = stringValue(rule, "on");
      const String offText = stringValue(rule, "off");
      const int index = relayIndex(relayId, candidate);
      AstroEvent onEvent;
      AstroEvent offEvent;

      if (index < 0 || usedRelays[index] || !parseAstroEventText(onText, onEvent) || !parseAstroEventText(offText, offEvent) || onEvent == offEvent) {
        return remember(msgId, nack(msgId, "invalid_args"));
      }
      if (_config.current().modes[index] == RelayMode::Disabled) {
        return remember(msgId, nack(msgId, "unsafe_state"));
      }

      usedRelays[index] = true;
      removeScheduleRulesForRelay(candidate, static_cast<uint8_t>(index));
      candidate.modes[index] = RelayMode::Astro;
      candidate.astroRules[candidate.astroRuleCount++] = {static_cast<uint8_t>(index), onEvent, offEvent};
    }

    String configError;
    if (!persistConfig(candidate, configError)) {
      Serial.println("astro.set_rules rejected: " + configError);
      return remember(msgId, nack(msgId, "invalid_args"));
    }
    applyConfigToRuntime(candidate);
    _astro.recalculate(_timeService.valid());
    return remember(msgId, ack(msgId, {true, "accepted", nullptr, String("{\"rule_count\":") + String(candidate.astroRuleCount) + "}"}));
  }

  if (target == "astro" && action == "recalculate") {
    if (!_timeService.valid()) {
      return remember(msgId, nack(msgId, "unsafe_state"));
    }
    if (!_astro.hasLocation()) {
      return remember(msgId, nack(msgId, "invalid_args"));
    }
    _astro.recalculate(true);
    return remember(msgId, ack(msgId, {true, "accepted", nullptr, "{\"astro\":\"recalculated\"}"}));
  }

  if (target == "node" && action == "reboot") {
    _rebootRequested = true;
    return remember(msgId, ack(msgId, {true, "accepted", nullptr, "{\"node\":\"rebooting\"}"}));
  }

  return remember(msgId, nack(msgId, "unknown_action"));
}

bool CommandHandler::takeRebootRequested() {
  const bool requested = _rebootRequested;
  _rebootRequested = false;
  return requested;
}

bool CommandHandler::declared(const String& target, const String& action) const {
  if (target == "relay" && (action == "set" || action == "toggle" || action == "set_mode" || action == "override" || action == "resume_schedule")) {
    return true;
  }
  if (target == "schedule" && (action == "set" || action == "enable" || action == "disable")) {
    return true;
  }
  if (target == "astro" && (action == "set_location" || action == "set_rules" || action == "recalculate")) {
    return true;
  }
  return target == "node" && action == "reboot";
}

String CommandHandler::remember(const String& msgId, const String& response) {
  if (msgId.length() > 0) {
    _lastMsgId = msgId;
    _lastAck = response;
  }
  return response;
}

String CommandHandler::ack(const String& refMsgId, const CommandResult& result) const {
  if (!result.ok) {
    return nack(refMsgId, result.error ? result.error : "invalid_args");
  }
  return String("{\"eco\":\"easysystems\",\"type\":\"ack\",\"ref_msg_id\":") + quote(refMsgId) + ",\"ok\":true,\"status\":" + quote(result.status ? result.status : "accepted") + ",\"result\":" + (result.resultJson.length() ? result.resultJson : "{}") + "}";
}

String CommandHandler::nack(const String& refMsgId, const char* error) const {
  return String("{\"eco\":\"easysystems\",\"type\":\"ack\",\"ref_msg_id\":") + quote(refMsgId) + ",\"ok\":false,\"status\":\"rejected\",\"error\":" + quote(error ? error : "invalid_args") + "}";
}

bool CommandHandler::persistConfig(const EzLightConfig& candidate, String& error) {
  if (!_config.validate(candidate, error)) {
    return false;
  }

  JsonDocument doc;
  doc["node_id"] = candidate.nodeId;
  doc["node_type"] = candidate.nodeType;
  doc["profile"] = candidate.profile;
  doc["timezone"] = candidate.timezone;
  doc["time_source"] = candidate.timeSource;
  doc["rtc_required"] = candidate.rtcRequired;
  doc["astro_convention"] = candidate.astroConvention;

  JsonArray relays = doc["relays"].to<JsonArray>();
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    JsonObject relay = relays.add<JsonObject>();
    relay["id"] = candidate.relays[i].id;
    relay["label"] = candidate.relays[i].label;
    relay["gpio"] = candidate.relays[i].gpio;
    relay["active_low"] = candidate.relays[i].activeLow;
    relay["mode"] = relayModeToString(candidate.modes[i]);
  }

  JsonObject schedule = doc["schedule"].to<JsonObject>();
  schedule["enabled"] = candidate.scheduleGloballyEnabled;
  schedule["storage"] = "littlefs_json";
  JsonArray scheduleRules = schedule["rules"].to<JsonArray>();
  for (uint8_t i = 0; i < candidate.scheduleRuleCount; ++i) {
    const FixedScheduleRule& rule = candidate.scheduleRules[i];
    JsonObject outputRule = scheduleRules.add<JsonObject>();
    outputRule["relay_id"] = candidate.relays[rule.relayIndex].id;
    outputRule["on"] = minuteToText(rule.onMinute);
    outputRule["off"] = minuteToText(rule.offMinute);
  }

  JsonObject astro = doc["astro"].to<JsonObject>();
  astro["location_set"] = candidate.astroLocationSet;
  if (candidate.astroLocationSet) {
    astro["latitude"] = candidate.latitude;
    astro["longitude"] = candidate.longitude;
  }
  JsonArray astroRules = astro["rules"].to<JsonArray>();
  for (uint8_t i = 0; i < candidate.astroRuleCount; ++i) {
    const AstroRule& rule = candidate.astroRules[i];
    JsonObject outputRule = astroRules.add<JsonObject>();
    outputRule["relay_id"] = candidate.relays[rule.relayIndex].id;
    outputRule["on"] = astroEventToString(rule.onEvent);
    outputRule["off"] = astroEventToString(rule.offEvent);
  }

  File tempFile = LittleFS.open("/config.tmp", "w");
  if (!tempFile) {
    error = "config_open_failed";
    return false;
  }
  if (serializeJson(doc, tempFile) == 0) {
    tempFile.close();
    LittleFS.remove("/config.tmp");
    error = "config_write_failed";
    return false;
  }
  tempFile.flush();
  tempFile.close();

  LittleFS.remove("/config.bak");
  const bool hadExisting = LittleFS.exists("/config.json");
  if (hadExisting && !LittleFS.rename("/config.json", "/config.bak")) {
    LittleFS.remove("/config.tmp");
    error = "config_backup_failed";
    return false;
  }
  if (!LittleFS.rename("/config.tmp", "/config.json")) {
    if (hadExisting) {
      LittleFS.rename("/config.bak", "/config.json");
    }
    error = "config_replace_failed";
    return false;
  }
  LittleFS.remove("/config.bak");

  if (!_config.applyPersistedIfValid(candidate, error)) {
    return false;
  }
  return true;
}

void CommandHandler::applyConfigToRuntime(const EzLightConfig& config) {
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    _relays.setMode(config.relays[i].id, config.modes[i]);
  }
  _schedule.configure(config);
  _astro.configure(config);
}

bool CommandHandler::parseTimeOfDay(const String& value, uint16_t& minuteOfDay) const {
  if (value.length() != 5 || value.charAt(2) != ':') {
    return false;
  }
  for (uint8_t i = 0; i < 5; ++i) {
    if (i == 2) {
      continue;
    }
    if (!isDigit(value.charAt(i))) {
      return false;
    }
  }
  const int hour = value.substring(0, 2).toInt();
  const int minute = value.substring(3, 5).toInt();
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
    return false;
  }
  minuteOfDay = static_cast<uint16_t>((hour * 60) + minute);
  return true;
}

int CommandHandler::relayIndex(const String& relayId, const EzLightConfig& config) const {
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    if (relayId == config.relays[i].id) {
      return i;
    }
  }
  return -1;
}
