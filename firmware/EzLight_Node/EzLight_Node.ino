#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "RelayDriver.h"
#include "ConfigStore.h"
#include "TimeService.h"
#include "ScheduleEngine.h"
#include "AstroEngine.h"
#include "OverrideManager.h"
#include "EzHubCapabilities.h"
#include "CommandHandler.h"
#include "TelemetryReporter.h"
#include "WebRoutes.h"

RelayDriver relays;
ConfigStore configStore;
TimeService timeService;
ScheduleEngine scheduleEngine;
AstroEngine astroEngine;
OverrideManager overrideManager;
EzHubCapabilities capabilities;
TelemetryReporter telemetryReporter;
WebServer server(80);
CommandHandler commandHandler(relays, overrideManager, astroEngine, configStore, timeService);
WebRoutes webRoutes(server, capabilities, telemetryReporter, commandHandler, relays, scheduleEngine, astroEngine, timeService);

const char* WIFI_SSID = ""; // TODO: Load Wi-Fi credentials from EzHub provisioning or local secure config.
const char* WIFI_PASS = "";

void connectWifiIfConfigured() {
  if (String(WIFI_SSID).length() == 0) {
    Serial.println("Wi-Fi not configured; status page starts after credentials are provided.");
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("ezlight-001");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  const unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < 10000UL) {
    delay(100);
  }
}

void setup() {
  // Absolute first hardware action: all relay pins are written OFF before OUTPUT.
  relays.beginSafe();

  Serial.begin(115200);
  Serial.println("EzLight-node v0.1 skeleton booting with relays forced OFF");

  configStore.begin();
  EzLightConfig candidate;
  configStore.load(candidate);
  String configError;
  if (!configStore.applyIfValid(candidate, configError)) {
    Serial.print("Config rejected before apply: ");
    Serial.println(configError);
  }

  connectWifiIfConfigured();
  timeService.beginNtp(configStore.current().timezone.c_str());
  timeService.update();
  scheduleEngine.update(timeService.valid(), relays);
  astroEngine.recalculate(timeService.valid());
  webRoutes.begin();
}

void loop() {
  timeService.update();
  overrideManager.update(relays);
  scheduleEngine.update(timeService.valid(), relays);
  if (timeService.valid()) {
    astroEngine.recalculate(true);
  }
  webRoutes.handleClient();
  delay(10);
}
