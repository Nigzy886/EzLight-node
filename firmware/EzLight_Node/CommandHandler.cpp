#include "CommandHandler.h"
#include "EzLightTypes.h"

CommandHandler::CommandHandler(RelayDriver& relays, OverrideManager& overrides, AstroEngine& astro, ConfigStore& config, TimeService& timeService) : _relays(relays), _overrides(overrides), _astro(astro), _config(config), _timeService(timeService) {}

String CommandHandler::handle(const String& body) {
  const String msgId = extractString(body, "msg_id");
  const String nodeId = extractString(body, "node_id");
  const String target = extractString(body, "target");
  const String action = extractString(body, "action");
  if (msgId.length() == 0) { return nack("", "invalid_args"); }
  if (nodeId.length() > 0 && nodeId != EZLIGHT_NODE_ID) { return nack(msgId, "wrong_target"); }
  if (!declared(target, action)) { return nack(msgId, action.length() == 0 ? "unknown_action" : "not_declared"); }
  if (target == "relay" && action == "set") {
    const String relayId = extractString(body, "relay_id");
    const bool state = extractBool(body, "state", false);
    const int index = _relays.indexOf(relayId);
    if (index < 0) { return nack(msgId, "invalid_args"); }
    if (_relays.mode(index) == RelayMode::Disabled) { return nack(msgId, "unsafe_state"); }
    if (!_relays.setRelay(relayId, state)) { return nack(msgId, "invalid_args"); }
    return ack(msgId, {true, "accepted", nullptr, String("{\"relay_id\":\"") + relayId + "\",\"relay_state\":" + (state ? "true" : "false") + "}"});
  }
  if (target == "relay" && action == "toggle") {
    const String relayId = extractString(body, "relay_id");
    const int index = _relays.indexOf(relayId);
    if (index < 0) { return nack(msgId, "invalid_args"); }
    if (_relays.mode(index) == RelayMode::Disabled) { return nack(msgId, "unsafe_state"); }
    _relays.toggleRelay(relayId);
    return ack(msgId, {true, "accepted", nullptr, String("{\"relay_id\":\"") + relayId + "\",\"relay_state\":" + (_relays.logicalState(index) ? "true" : "false") + "}"});
  }
  if (target == "relay" && action == "override") { return ack(msgId, _overrides.applyOverride(extractString(body, "relay_id"), extractString(body, "override_mode"), _relays)); }
  if (target == "relay" && action == "resume_schedule") { return ack(msgId, _overrides.resumeSchedule(extractString(body, "relay_id"), _relays)); }
  if (target == "astro" && action == "set_location") { return nack(msgId, "invalid_args"); }
  if (target == "astro" && action == "recalculate") {
    if (!_timeService.valid()) {
      return nack(msgId, "unsafe_state");
    }
    if (!_astro.hasLocation()) {
      return nack(msgId, "invalid_args");
    }
    _astro.recalculate(true);
    return ack(msgId, {true, "accepted", nullptr, "{\"astro\":\"recalculate_requested\"}"});
  }
  if (target == "schedule" && (action == "set" || action == "enable" || action == "disable")) { return ack(msgId, {true, "accepted", nullptr, String("{\"schedule\":\"") + action + "_pending\"}"}); }
  if (target == "node" && action == "reboot") { return ack(msgId, {true, "accepted", nullptr, "{\"node\":\"reboot_pending\"}"}); }
  return nack(msgId, "unknown_action");
}

bool CommandHandler::declared(const String& target, const String& action) const {
  if (target == "relay" && (action == "set" || action == "toggle" || action == "override" || action == "resume_schedule")) { return true; }
  if (target == "schedule" && (action == "set" || action == "enable" || action == "disable")) { return true; }
  if (target == "astro" && (action == "set_location" || action == "recalculate")) { return true; }
  if (target == "node" && action == "reboot") { return true; }
  return false;
}

String CommandHandler::extractString(const String& body, const String& key) const {
  const String needle = String("\"") + key + "\"";
  int pos = body.indexOf(needle);
  if (pos < 0) { return ""; }
  pos = body.indexOf(':', pos);
  pos = body.indexOf('"', pos);
  if (pos < 0) { return ""; }
  const int end = body.indexOf('"', pos + 1);
  if (end < 0) { return ""; }
  return body.substring(pos + 1, end);
}

bool CommandHandler::extractBool(const String& body, const String& key, bool defaultValue) const {
  const String needle = String("\"") + key + "\"";
  int pos = body.indexOf(needle);
  if (pos < 0) { return defaultValue; }
  pos = body.indexOf(':', pos);
  if (pos < 0) { return defaultValue; }
  return body.substring(pos + 1).indexOf("true") >= 0;
}

String CommandHandler::ack(const String& refMsgId, const CommandResult& result) const {
  if (!result.ok) { return nack(refMsgId, result.error ? result.error : "invalid_args"); }
  return String("{\"type\":\"ack\",\"ref_msg_id\":\"") + refMsgId + "\",\"ok\":true,\"status\":\"" + result.status + "\",\"result\":" + (result.resultJson.length() ? result.resultJson : "{}") + "}";
}

String CommandHandler::nack(const String& refMsgId, const char* error) const {
  return String("{\"type\":\"ack\",\"ref_msg_id\":\"") + refMsgId + "\",\"ok\":false,\"status\":\"rejected\",\"error\":\"" + error + "\"}";
}
