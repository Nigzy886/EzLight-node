#include "RelayDriver.h"

RelayDriver::RelayDriver()
  : _defs{{"path_lights", "Path lights", 14, true}, {"driveway_lights", "Driveway lights", 27, true}, {"entrance_lights", "Entrance lights", 26, true}, {"spare_lights", "Spare lights", 25, true}}, _states{} {
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    _states[i].logicalOn = false;
    _states[i].physicalLevel = offLevel(_defs[i]);
    _states[i].mode = RelayMode::Manual;
    _states[i].overrideMode = OverrideMode::None;
    _states[i].overrideExpiresAtMs = 0;
  }
}

void RelayDriver::beginSafe() {
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    const uint8_t level = offLevel(_defs[i]);
    digitalWrite(_defs[i].gpio, level); // Required: write OFF before OUTPUT.
    pinMode(_defs[i].gpio, OUTPUT);
    digitalWrite(_defs[i].gpio, level);
    _states[i].logicalOn = false;
    _states[i].physicalLevel = level;
  }
}

bool RelayDriver::setRelay(const String& relayId, bool on) {
  const int index = indexOf(relayId);
  if (index < 0 || _states[index].mode == RelayMode::Disabled) {
    return false;
  }
  writePhysical(static_cast<uint8_t>(index), on);
  return true;
}

bool RelayDriver::toggleRelay(const String& relayId) {
  const int index = indexOf(relayId);
  if (index < 0) {
    return false;
  }
  return setRelay(relayId, !_states[index].logicalOn);
}

bool RelayDriver::isKnownRelay(const String& relayId) const { return indexOf(relayId) >= 0; }

int RelayDriver::indexOf(const String& relayId) const {
  for (uint8_t i = 0; i < EZLIGHT_RELAY_COUNT; ++i) {
    if (relayId == _defs[i].id) {
      return i;
    }
  }
  return -1;
}

bool RelayDriver::logicalState(uint8_t index) const { return index < EZLIGHT_RELAY_COUNT ? _states[index].logicalOn : false; }
uint8_t RelayDriver::physicalLevel(uint8_t index) const { return index < EZLIGHT_RELAY_COUNT ? _states[index].physicalLevel : LOW; }
RelayMode RelayDriver::mode(uint8_t index) const { return index < EZLIGHT_RELAY_COUNT ? _states[index].mode : RelayMode::Disabled; }

bool RelayDriver::setMode(const String& relayId, RelayMode mode) {
  const int index = indexOf(relayId);
  if (index < 0) {
    return false;
  }
  _states[index].mode = mode;
  if (mode == RelayMode::Disabled) {
    writePhysical(static_cast<uint8_t>(index), false);
  }
  return true;
}

const RelayDefinition& RelayDriver::definition(uint8_t index) const {
  return _defs[index < EZLIGHT_RELAY_COUNT ? index : 0];
}

uint8_t RelayDriver::offLevel(const RelayDefinition& def) const { return def.activeLow ? HIGH : LOW; }
uint8_t RelayDriver::onLevel(const RelayDefinition& def) const { return def.activeLow ? LOW : HIGH; }

void RelayDriver::writePhysical(uint8_t index, bool logicalOn) {
  if (index >= EZLIGHT_RELAY_COUNT) {
    return;
  }
  const uint8_t level = logicalOn ? onLevel(_defs[index]) : offLevel(_defs[index]);
  digitalWrite(_defs[index].gpio, level);
  _states[index].logicalOn = logicalOn;
  _states[index].physicalLevel = level;
}
