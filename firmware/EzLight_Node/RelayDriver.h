#pragma once

#include <Arduino.h>
#include "EzLightTypes.h"

class RelayDriver {
public:
  RelayDriver();

  void beginSafe();
  bool setRelay(const String& relayId, bool on);
  bool toggleRelay(const String& relayId);
  bool isKnownRelay(const String& relayId) const;
  int indexOf(const String& relayId) const;

  bool logicalState(uint8_t index) const;
  uint8_t physicalLevel(uint8_t index) const;
  RelayMode mode(uint8_t index) const;
  bool setMode(const String& relayId, RelayMode mode);
  const RelayDefinition& definition(uint8_t index) const;

private:
  RelayDefinition _defs[EZLIGHT_RELAY_COUNT];
  RelayRuntimeState _states[EZLIGHT_RELAY_COUNT];

  uint8_t offLevel(const RelayDefinition& def) const;
  uint8_t onLevel(const RelayDefinition& def) const;
  void writePhysical(uint8_t index, bool logicalOn);
};
