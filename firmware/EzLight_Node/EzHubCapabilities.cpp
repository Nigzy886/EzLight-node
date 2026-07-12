#include "EzHubCapabilities.h"
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
}  // namespace

String EzHubCapabilities::helloJson(const String& nodeId, const String& ipAddress, const String& hostName, const String& friendlyName, bool provisioned) const {
  String json = "{";
  json += "\"eco\":\"easysystems\",\"type\":\"hello\",";
  json += "\"node_id\":" + quote(nodeId) + ",";
  json += "\"node_type\":" + quote(EZLIGHT_NODE_TYPE) + ",";
  json += "\"model\":" + quote(EZLIGHT_MODEL) + ",";
  json += "\"fw\":" + quote(EZLIGHT_FW) + ",";
  json += "\"ver\":" + quote(EZLIGHT_VERSION) + ",";
  json += "\"profile\":" + quote(EZLIGHT_PROFILE) + ",";
  json += "\"transport\":\"wifi_primary\",";
  json += "\"ip\":" + quote(ipAddress) + ",";
  json += "\"host\":" + quote(hostName) + ",";
  json += "\"provisioned\":" + String(provisioned ? "true" : "false") + ",";
  json += "\"friendly_name\":" + quote(friendlyName);
  json += "}";
  return json;
}

String EzHubCapabilities::capabilitiesJson(const String& nodeId, const String& ipAddress, const String& hostName, bool provisioned) const {
  String json = "{";
  json += "\"eco\":\"easysystems\",\"type\":\"capabilities\",";
  json += "\"node_id\":" + quote(nodeId) + ",";
  json += "\"model\":" + quote(EZLIGHT_MODEL) + ",";
  json += "\"fw\":" + quote(EZLIGHT_FW) + ",";
  json += "\"ver\":" + quote(EZLIGHT_VERSION) + ",";
  json += "\"profile\":" + quote(EZLIGHT_PROFILE) + ",";
  json += "\"transport\":\"wifi_primary\",";
  json += "\"ip\":" + quote(ipAddress) + ",";
  json += "\"host\":" + quote(hostName) + ",";
  json += "\"provisioned\":" + String(provisioned ? "true" : "false") + ",";

  json += "\"controls\":[";
  json += "{\"id\":\"path_lights\",\"kind\":\"relay\",\"label\":\"Path lights\",\"modes\":[\"manual\",\"schedule\",\"astro\",\"disabled\"]},";
  json += "{\"id\":\"driveway_lights\",\"kind\":\"relay\",\"label\":\"Driveway lights\",\"modes\":[\"manual\",\"schedule\",\"astro\",\"disabled\"]},";
  json += "{\"id\":\"entrance_lights\",\"kind\":\"relay\",\"label\":\"Entrance lights\",\"modes\":[\"manual\",\"schedule\",\"astro\",\"disabled\"]},";
  json += "{\"id\":\"spare_lights\",\"kind\":\"relay\",\"label\":\"Spare lights\",\"modes\":[\"manual\",\"schedule\",\"astro\",\"disabled\"]}";
  json += "],";

  json += "\"commands\":[";
  json += "{\"id\":\"relay.set\",\"target\":\"relay\",\"action\":\"set\"},";
  json += "{\"id\":\"relay.toggle\",\"target\":\"relay\",\"action\":\"toggle\"},";
  json += "{\"id\":\"relay.set_mode\",\"target\":\"relay\",\"action\":\"set_mode\",\"modes\":[\"manual\",\"disabled\"]},";
  json += "{\"id\":\"relay.override\",\"target\":\"relay\",\"action\":\"override\",\"modes\":[\"on_for_5_min\",\"on_for_15_min\",\"on_for_30_min\",\"on_for_1_hour\",\"off_until_next_event\",\"manual_on\",\"manual_off\"]},";
  json += "{\"id\":\"relay.resume_schedule\",\"target\":\"relay\",\"action\":\"resume_schedule\"},";
  json += "{\"id\":\"schedule.set\",\"target\":\"schedule\",\"action\":\"set\"},";
  json += "{\"id\":\"schedule.enable\",\"target\":\"schedule\",\"action\":\"enable\"},";
  json += "{\"id\":\"schedule.disable\",\"target\":\"schedule\",\"action\":\"disable\"},";
  json += "{\"id\":\"astro.set_location\",\"target\":\"astro\",\"action\":\"set_location\"},";
  json += "{\"id\":\"astro.set_rules\",\"target\":\"astro\",\"action\":\"set_rules\",\"events\":[\"civil_dusk\",\"civil_dawn\"]},";
  json += "{\"id\":\"astro.recalculate\",\"target\":\"astro\",\"action\":\"recalculate\"},";
  json += "{\"id\":\"node.reboot\",\"target\":\"node\",\"action\":\"reboot\"}";
  json += "],";

  json += "\"routes\":[\"/\",\"/status\",\"/api/hello\",\"/api/capabilities\",\"/api/state\",\"/api/cmd\"],";
  json += "\"reporting\":{\"state_fields\":[\"relay_state\",\"schedule_state\",\"time_valid\",\"current_time\",\"timezone\",\"next_event\",\"dusk_time\",\"dawn_time\",\"uptime\",\"provisioned\",\"wifi_rssi\"],\"state_interval_seconds\":30},";
  json += "\"rendering\":{\"dashboard\":\"outdoor_lighting\"},";
  json += "\"safety\":{\"boot_relays_forced_off\":true,\"default_relay_active_low\":true,\"active_low_off_level\":\"GPIO_HIGH\",\"schedules_require_valid_time\":true,\"undeclared_commands_rejected\":true,\"disabled_relays_reject_manual_commands\":true,\"mode_changes_force_relay_off\":true,\"reboot_ack_before_restart\":true},";
  json += "\"extensions\":{\"ezsystems.ezlight\":{\"board\":\"ESP32-WROOM DevKit / ESP32 Dev Module\",\"build_system\":\"arduino-cli / Arduino framework\",\"relay_count\":4,\"time_source\":\"ntp\",\"rtc_required\":false,\"astro_convention\":\"civil_dusk_civil_dawn\",\"config_storage\":\"littlefs_json\",\"commissioning\":\"esp_now_v1\",\"standards_baseline\":\"v1.1.5\"}}";
  json += "}";
  return json;
}
