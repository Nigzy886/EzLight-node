#include <Arduino.h>
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
#include "ProvisioningManager.h"
#include "WebRoutes.h"

RelayDriver relays;
ConfigStore configStore;
TimeService timeService;
ScheduleEngine scheduleEngine;
AstroEngine astroEngine;
OverrideManager overrideManager;
EzHubCapabilities capabilities;
TelemetryReporter telemetryReporter;
ProvisioningManager provisioning;
WebServer server(80);
CommandHandler commandHandler(relays, overrideManager, astroEngine, configStore, scheduleEngine, timeService);
WebRoutes webRoutes(server, capabilities, telemetryReporter, commandHandler, relays, scheduleEngine, astroEngine, timeService, configStore, provisioning);

bool ntpStarted = false;

void setup() {
  // Absolute first hardware action: all relay pins are written OFF before OUTPUT.
  relays.beginSafe();

  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println("EzLight-node managed boot with relays forced OFF");

  const bool configStoreOk = configStore.begin();
  EzLightConfig candidate;
  const bool configLoadOk = configStoreOk && configStore.load(candidate);
  String configError;
  if (configLoadOk && !configStore.applyIfValid(candidate, configError)) {
    Serial.print("Config rejected before apply: ");
    Serial.println(configError);
  } else if (!configLoadOk && configStore.lastError().length() > 0) {
    Serial.print("Config rejected before apply: ");
    Serial.println(configStore.lastError());
  }

  const EzLightConfig& activeConfig = configStore.current();
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    relays.setMode(activeConfig.relays[i].id, activeConfig.modes[i]);
  }
  scheduleEngine.configure(activeConfig);
  astroEngine.configure(activeConfig);

  provisioning.begin();

  Serial.print("Node ID: ");
  Serial.println(provisioning.nodeId());
  Serial.print("Provisioned: ");
  Serial.println(provisioning.provisioned() ? "true" : "false");

  if (provisioning.connected()) {
    timeService.beginNtp(activeConfig.timezone.c_str());
    ntpStarted = true;
    webRoutes.begin();
  } else if (!provisioning.provisioned()) {
    Serial.println("Waiting for EzHub pair_beacon and directed wifi_cfg");
  }

  timeService.update();
  scheduleEngine.update(timeService.valid(), relays);
  astroEngine.update(timeService.valid(), relays);
}

void loop() {
  provisioning.update();

  if (provisioning.connected()) {
    if (!ntpStarted) {
      timeService.beginNtp(configStore.current().timezone.c_str());
      ntpStarted = true;
    }
    if (!webRoutes.started()) {
      webRoutes.begin();
    }
  }

  timeService.update();
  overrideManager.update(relays);
  scheduleEngine.update(timeService.valid(), relays);
  astroEngine.update(timeService.valid(), relays);
  webRoutes.handleClient();
  delay(10);
}
