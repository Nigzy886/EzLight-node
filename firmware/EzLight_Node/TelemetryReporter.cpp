#include "TelemetryReporter.h"

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
}  // namespace

String TelemetryReporter::stateJson(const String& nodeId, bool provisioned, const String& ipAddress, const String& hostName, int32_t wifiRssi, const RelayDriver& relays, const ScheduleEngine& schedule, const AstroEngine& astro, const TimeService& timeService) const {
  String json = "{\"eco\":\"easysystems\",\"type\":\"state\",\"node_id\":" + quote(nodeId) + ",";
  json += "\"relay_state\":{";
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    if (i > 0) {
      json += ",";
    }
    json += quote(relays.definition(i).id) + ":" + (relays.logicalState(i) ? "true" : "false");
  }
  json += "},\"schedule_state\":{";
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    if (i > 0) {
      json += ",";
    }
    json += quote(relays.definition(i).id) + ":" + quote(schedule.stateFor(i));
  }
  json += "},";
  json += "\"time_valid\":" + String(timeService.valid() ? "true" : "false") + ",";
  json += "\"current_time\":" + quote(timeService.isoNow()) + ",";
  json += "\"timezone\":" + quote(timeService.timezone()) + ",";
  json += "\"next_event\":" + quote(schedule.nextEvent()) + ",";
  json += "\"dusk_time\":" + quote(astro.duskTime()) + ",";
  json += "\"dawn_time\":" + quote(astro.dawnTime()) + ",";
  json += "\"civil_dusk\":" + quote(astro.duskTime()) + ",";
  json += "\"civil_dawn\":" + quote(astro.dawnTime()) + ",";
  json += "\"uptime\":" + String(millis() / 1000UL) + ",";
  json += "\"provisioned\":" + String(provisioned ? "true" : "false") + ",";
  json += "\"ip\":" + quote(ipAddress) + ",";
  json += "\"host\":" + quote(hostName) + ",";
  json += "\"wifi_rssi\":" + String(wifiRssi);
  if (timeService.status().warning.length() > 0) {
    json += ",\"time_warning\":" + quote(timeService.status().warning);
  }
  if (astro.warning().length() > 0) {
    json += ",\"astro_warning\":" + quote(astro.warning());
  }
  json += "}";
  return json;
}
