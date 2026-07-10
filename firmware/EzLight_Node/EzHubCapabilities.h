#pragma once

#include <Arduino.h>

class EzHubCapabilities {
public:
  String helloJson(const String& ipAddress, const String& hostName) const;
  String capabilitiesJson() const;
};
