#include "EzHubCapabilities.h"
#include "EzLightTypes.h"

String EzHubCapabilities::helloJson(const String& ipAddress, const String& hostName) const {
  String json = "{";
  json += "\"eco\":\"easysystems\",\"type\":\"hello\",";
  json += "\"node_id\":\"" + String(EZLIGHT_NODE_ID) + "\",";
  json += "\"node_type\":\"" + String(EZLIGHT_NODE_TYPE) + "\",";
  json += "\"model\":\"" + String(EZLIGHT_MODEL) + "\",";
  json += "\"fw\":\"" + String(EZLIGHT_FW) + "\",";
  json += "\"ver\":\"" + String(EZLIGHT_VERSION) + "\",";
  json += "\"profile\":\"" + String(EZLIGHT_PROFILE) + "\",";
  json += "\"transport\":\"wifi_primary\",";
  json += "\"ip\":\"" + ipAddress + "\",\"host\":\"" + hostName + "\",";
  json += "\"provisioned\":true,\"friendly_name\":\"Outdoor Lights\"}";
  return json;
}

String EzHubCapabilities::capabilitiesJson() const {
  return F("{\"eco\":\"easysystems\",\"type\":\"capabilities\",\"node_id\":\"ezlight_001\",\"model\":\"ESP32-4Relay-EzLight\",\"fw\":\"ezlight-node\",\"profile\":\"experimental.ezlight.v1\",\"transport\":\"wifi_primary\",\"controls\":[{\"id\":\"path_lights\",\"kind\":\"relay\",\"label\":\"Path lights\",\"modes\":[\"manual\",\"schedule\",\"astro\",\"disabled\"]},{\"id\":\"driveway_lights\",\"kind\":\"relay\",\"label\":\"Driveway lights\",\"modes\":[\"manual\",\"schedule\",\"astro\",\"disabled\"]},{\"id\":\"entrance_lights\",\"kind\":\"relay\",\"label\":\"Entrance lights\",\"modes\":[\"manual\",\"schedule\",\"astro\",\"disabled\"]},{\"id\":\"spare_lights\",\"kind\":\"relay\",\"label\":\"Spare lights\",\"modes\":[\"manual\",\"schedule\",\"astro\",\"disabled\"]}],\"commands\":[{\"id\":\"relay.set\",\"target\":\"relay\",\"action\":\"set\"},{\"id\":\"relay.toggle\",\"target\":\"relay\",\"action\":\"toggle\"},{\"id\":\"relay.override\",\"target\":\"relay\",\"action\":\"override\"},{\"id\":\"relay.resume_schedule\",\"target\":\"relay\",\"action\":\"resume_schedule\"},{\"id\":\"schedule.set\",\"target\":\"schedule\",\"action\":\"set\"},{\"id\":\"schedule.enable\",\"target\":\"schedule\",\"action\":\"enable\"},{\"id\":\"schedule.disable\",\"target\":\"schedule\",\"action\":\"disable\"},{\"id\":\"astro.set_location\",\"target\":\"astro\",\"action\":\"set_location\"},{\"id\":\"astro.recalculate\",\"target\":\"astro\",\"action\":\"recalculate\"},{\"id\":\"node.reboot\",\"target\":\"node\",\"action\":\"reboot\"}],\"routes\":[\"/\",\"/status\",\"/api/hello\",\"/api/capabilities\",\"/api/state\",\"/api/cmd\",\"/api/reboot\"],\"reporting\":{\"state_fields\":[\"relay_state\",\"schedule_state\",\"time_valid\",\"next_event\",\"dusk_time\",\"dawn_time\",\"civil_dusk\",\"civil_dawn\",\"uptime\"],\"state_interval_seconds\":30},\"rendering\":{\"dashboard\":\"outdoor_lighting\"},\"safety\":{\"boot_relays_forced_off\":true,\"default_relay_active_low\":true,\"active_low_off_level\":\"GPIO_HIGH\",\"schedules_require_valid_time\":true,\"undeclared_commands_rejected\":true,\"disabled_relays_reject_manual_commands\":true},\"extensions\":{\"ezsystems.ezlight\":{\"board\":\"ESP32-WROOM DevKit / ESP32 Dev Module\",\"build_system\":\"arduino-cli / Arduino framework\",\"relay_count\":4,\"time_source\":\"ntp\",\"rtc_required\":false,\"astro_convention\":\"civil_dusk_civil_dawn\",\"config_storage\":\"littlefs_json\",\"local_web_ui\":\"status_debug_only\"}}}");
}
