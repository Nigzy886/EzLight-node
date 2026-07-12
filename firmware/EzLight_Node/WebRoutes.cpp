#include "WebRoutes.h"

WebRoutes::WebRoutes(WebServer& server, EzHubCapabilities& capabilities, TelemetryReporter& telemetry, CommandHandler& commands, RelayDriver& relays, ScheduleEngine& schedule, AstroEngine& astro, TimeService& timeService, ConfigStore& configStore, ProvisioningManager& provisioning)
  : _server(server),
    _capabilities(capabilities),
    _telemetry(telemetry),
    _commands(commands),
    _relays(relays),
    _schedule(schedule),
    _astro(astro),
    _timeService(timeService),
    _configStore(configStore),
    _provisioning(provisioning),
    _started(false) {}

void WebRoutes::begin() {
  if (_started) {
    return;
  }

  _server.on("/", HTTP_GET, [this]() { sendStatusPage(); });
  _server.on("/status", HTTP_GET, [this]() { sendStatusPage(); });
  _server.on("/api/hello", HTTP_GET, [this]() {
    sendJson(200, _capabilities.helloJson(_provisioning.nodeId(), _provisioning.ipAddress(), _provisioning.hostName(), _provisioning.friendlyName(), _provisioning.provisioned()));
  });
  _server.on("/api/capabilities", HTTP_GET, [this]() {
    sendJson(200, _capabilities.capabilitiesJson(_provisioning.nodeId(), _provisioning.ipAddress(), _provisioning.hostName(), _provisioning.provisioned()));
  });
  _server.on("/api/state", HTTP_GET, [this]() {
    sendJson(200, _telemetry.stateJson(_provisioning.nodeId(), _provisioning.provisioned(), _provisioning.ipAddress(), _provisioning.hostName(), _provisioning.wifiRssi(), _relays, _schedule, _astro, _timeService));
  });
  _server.on("/api/cmd", HTTP_POST, [this]() { handleCommand(); });
  _server.onNotFound([this]() { sendJson(404, "{\"error\":\"not_found\"}"); });
  _server.begin();
  _started = true;
}

void WebRoutes::handleClient() {
  if (_started) {
    _server.handleClient();
  }
}

bool WebRoutes::started() const { return _started; }

void WebRoutes::handleCommand() {
  const String response = _commands.handle(_server.arg("plain"), _provisioning.nodeId());
  const bool rebootRequested = _commands.takeRebootRequested();

  if (rebootRequested) {
    _relays.allOff();
  }

  sendJson(200, response);

  if (rebootRequested) {
    delay(250);
    ESP.restart();
  }
}

void WebRoutes::sendStatusPage() {
  String html = F("<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>EzLight-node Status</title></head><body><h1>EzLight-node status</h1>");
  html += F("<p>ESP32 controls low-voltage relay/control inputs only. It does not directly switch mains wiring.</p>");
  html += "<p>node_id=" + _provisioning.nodeId() + "</p>";
  html += "<p>provisioned=" + String(_provisioning.provisioned() ? "true" : "false") + "</p>";
  html += "<p>ip=" + _provisioning.ipAddress() + "</p>";
  html += "<p>host=" + _provisioning.hostName() + "</p>";
  html += "<p>wifi_channel=" + String(_provisioning.wifiChannel()) + "</p>";
  html += F("<h2>Relays</h2><ul>");
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    html += "<li>" + String(_relays.definition(i).id) + ": logical=" + (_relays.logicalState(i) ? "on" : "off") + ", physical_level=" + String(_relays.physicalLevel(i)) + ", mode=" + relayModeToString(_relays.mode(i)) + "</li>";
  }
  html += F("</ul>");
  html += "<p>config_loaded=" + String(_configStore.configLoaded() ? "true" : "false") + "</p>";
  html += "<p>config_source=" + _configStore.configSource() + "</p>";
  html += "<p>config_error=" + _configStore.lastError() + "</p>";
  html += "<p>time_valid=" + String(_timeService.valid() ? "true" : "false") + "</p>";
  html += "<p>next_event=" + _schedule.nextEvent() + "</p>";
  html += "<p>dusk_time=" + _astro.duskTime() + "</p>";
  html += "<p>dawn_time=" + _astro.dawnTime() + "</p>";
  html += F("<p>No local schedule editor is provided. EzHub uses the declared command API.</p></body></html>");
  _server.sendHeader("Cache-Control", "no-store");
  _server.send(200, "text/html", html);
}

void WebRoutes::sendJson(uint16_t code, const String& json) {
  _server.sendHeader("Cache-Control", "no-store");
  _server.send(code, "application/json", json);
}
