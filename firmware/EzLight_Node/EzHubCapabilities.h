#pragma once

#include <Arduino.h>

class EzHubCapabilities {
public:
  String helloJson(const String& nodeId, const String& ipAddress, const String& hostName, const String& friendlyName, bool provisioned) const;
  String capabilitiesJson(const String& nodeId, const String& ipAddress, const String& hostName, bool provisioned) const;
};
