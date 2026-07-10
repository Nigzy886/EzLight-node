#include "TelemetryReporter.h"
#include "EzLightTypes.h"

String TelemetryReporter::stateJson(const RelayDriver& relays, const ScheduleEngine& schedule, const AstroEngine& astro, const TimeService& timeService) const {
  String json = "{\"eco\":\"easysystems\",\"type\":\"state\",\"node_id\":\"" + String(EZLIGHT_NODE_ID) + "\",";
  json += "\"relay_state\":{";
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    if (i > 0) { json += ","; }
    json += "\"" + String(relays.definition(i).id) + "\":" + (relays.logicalState(i) ? "true" : "false");
  }
  json += "},\"schedule_state\":{";
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    if (i > 0) { json += ","; }
    json += "\"" + String(relays.definition(i).id) + "\":\"" + schedule.stateFor(i) + "\"";
  }
  json += "},";
  json += "\"time_valid\":" + String(timeService.valid() ? "true" : "false") + ",";
  json += "\"next_event\":\"" + schedule.nextEvent() + "\",";
  json += "\"dusk_time\":\"" + astro.duskTime() + "\",";
  json += "\"dawn_time\":\"" + astro.dawnTime() + "\",";
  json += "\"civil_dusk\":\"" + astro.duskTime() + "\",";
  json += "\"civil_dawn\":\"" + astro.dawnTime() + "\",";
  json += "\"uptime\":" + String(millis() / 1000UL);
  if (timeService.status().warning.length() > 0) { json += ",\"time_warning\":\"" + timeService.status().warning + "\""; }
  if (astro.warning().length() > 0) { json += ",\"astro_warning\":\"" + astro.warning() + "\""; }
  json += "}";
  return json;
}
