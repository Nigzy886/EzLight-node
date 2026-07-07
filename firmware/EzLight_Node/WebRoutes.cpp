#include "WebRoutes.h"
#include <WiFi.h>

WebRoutes::WebRoutes(WebServer& server, EzHubCapabilities& capabilities, TelemetryReporter& telemetry, CommandHandler& commands, RelayDriver& relays, ScheduleEngine& schedule, AstroEngine& astro, TimeService& timeService)
  : _server(server), _capabilities(capabilities), _telemetry(telemetry), _commands(commands), _relays(relays), _schedule(schedule), _astro(astro), _timeService(timeService) {}

void WebRoutes::begin() {
  _server.on("/", HTTP_GET, [this]() { sendStatusPage(); });
  _server.on("/status", HTTP_GET, [this]() { sendStatusPage(); });
  _server.on("/api/hello", HTTP_GET, [this]() { sendJson(200, _capabilities.helloJson(WiFi.localIP().toString(), WiFi.getHostname() ? WiFi.getHostname() : "ezlight-001")); });
  _server.on("/api/capabilities", HTTP_GET, [this]() { sendJson(200, _capabilities.capabilitiesJson()); });
  _server.on("/api/state", HTTP_GET, [this]() { sendJson(200, _telemetry.stateJson(_relays, _schedule, _astro, _timeService)); });
  _server.on("/api/cmd", HTTP_POST, [this]() { sendJson(200, _commands.handle(_server.arg("plain"))); });
  _server.on("/api/reboot", HTTP_POST, [this]() { sendJson(200, "{\"type\":\"ack\",\"ref_msg_id\":\"local-reboot\",\"ok\":true,\"status\":\"accepted\",\"result\":{\"node\":\"reboot_pending\"}}"); });
  _server.begin();
}

void WebRoutes::handleClient() { _server.handleClient(); }

void WebRoutes::sendStatusPage() {
  String html = F("<!doctype html><html><head><meta charset='utf-8'><title>EzLight-node Status</title></head><body><h1>EzLight-node status</h1>");
  html += F("<p>ESP32 controls low-voltage relay/control inputs only. It does not directly switch mains wiring.</p><h2>Relays</h2><ul>");
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    html += "<li>" + String(_relays.definition(i).id) + ": logical=" + (_relays.logicalState(i) ? "on" : "off") + ", physical_level=" + String(_relays.physicalLevel(i)) + ", mode=" + relayModeToString(_relays.mode(i)) + "</li>";
  }
  html += F("</ul>");
  html += "<p>time_valid=" + String(_timeService.valid() ? "true" : "false") + "</p>";
  html += "<p>next_event=" + _schedule.nextEvent() + "</p>";
  html += "<p>dusk_time=" + _astro.duskTime() + "</p>";
  html += "<p>dawn_time=" + _astro.dawnTime() + "</p>";
  html += F("<p>No schedule editor or config editor is provided in v0.1.</p></body></html>");
  _server.send(200, "text/html", html);
}

void WebRoutes::sendJson(uint16_t code, const String& json) { _server.send(code, "application/json", json); }
