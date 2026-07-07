#include "ConfigStore.h"
#include <LittleFS.h>

ConfigStore::ConfigStore() : _current(defaults()), _lastError("") {}

bool ConfigStore::begin() {
  if (!LittleFS.begin(true)) {
    _lastError = "littlefs_mount_failed";
    return false;
  }
  return true;
}

EzLightConfig ConfigStore::defaults() const {
  EzLightConfig config;
  config.nodeId = EZLIGHT_NODE_ID;
  config.relays[0] = {"path_lights", "Path lights", 14, true};
  config.relays[1] = {"driveway_lights", "Driveway lights", 27, true};
  config.relays[2] = {"entrance_lights", "Entrance lights", 26, true};
  config.relays[3] = {"spare_lights", "Spare lights", 25, true};
  config.timezone = "NZST-12NZDT,M9.5.0,M4.1.0/3";
  config.astroLocationSet = false;
  config.latitude = 0.0;
  config.longitude = 0.0;
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    config.modes[i] = RelayMode::Manual;
    config.scheduleEnabled[i] = false;
  }
  return config;
}

bool ConfigStore::load(EzLightConfig& outConfig) {
  outConfig = defaults();
  if (!LittleFS.exists("/config.json")) {
    return true;
  }
  // TODO: Parse LittleFS JSON after the config schema is locked in this repo.
  return true;
}

bool ConfigStore::validate(const EzLightConfig& config, String& error) const {
  if (config.nodeId.length() == 0) {
    error = "missing_node_id";
    return false;
  }
  if (!validateRelayDefaults(config, error)) {
    return false;
  }
  if (config.timezone.length() == 0) {
    error = "missing_timezone";
    return false;
  }
  if (config.astroLocationSet && (config.latitude < -90.0 || config.latitude > 90.0 || config.longitude < -180.0 || config.longitude > 180.0)) {
    error = "invalid_astro_location";
    return false;
  }
  return true;
}

bool ConfigStore::applyIfValid(const EzLightConfig& candidate, String& error) {
  if (!validate(candidate, error)) {
    _lastError = error;
    return false;
  }
  _current = candidate;
  _lastError = "";
  return true;
}

const EzLightConfig& ConfigStore::current() const { return _current; }
const String& ConfigStore::lastError() const { return _lastError; }

bool ConfigStore::validateRelayDefaults(const EzLightConfig& config, String& error) const {
  const char* expectedIds[EZLIGHT_RELAY_COUNT] = {"path_lights", "driveway_lights", "entrance_lights", "spare_lights"};
  const uint8_t expectedGpios[EZLIGHT_RELAY_COUNT] = {14, 27, 26, 25};
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    if (String(config.relays[i].id) != expectedIds[i]) {
      error = "invalid_relay_id";
      return false;
    }
    if (config.relays[i].gpio != expectedGpios[i]) {
      error = "invalid_relay_gpio";
      return false;
    }
    if (!config.relays[i].activeLow) {
      error = "invalid_relay_polarity";
      return false;
    }
  }
  return true;
}
