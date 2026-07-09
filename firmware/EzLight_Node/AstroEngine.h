#pragma once

#include <Arduino.h>
#include "ConfigStore.h"
#include "RelayDriver.h"

class AstroEngine {
public:
  AstroEngine();
  void configure(const EzLightConfig& config);
  bool setLocation(double latitude, double longitude);
  void recalculate(bool timeValid);
  void update(bool timeValid, RelayDriver& relays);
  const String& duskTime() const;
  const String& dawnTime() const;
  const String& warning() const;
  bool hasLocation() const;

private:
  bool _hasLocation;
  double _latitude;
  double _longitude;
  uint8_t _ruleCount;
  AstroRule _rules[EZLIGHT_MAX_ASTRO_RULES];
  int _lastYear;
  int _lastYday;
  int _dawnMinute;
  int _duskMinute;
  String _duskTime;
  String _dawnTime;
  String _warning;

  bool calculateForLocalDate(const tm& localTime);
  bool calculateCivilEvent(const tm& localTime, bool dawn, int offsetMinutes, int& minuteOfDay) const;
  bool ruleIsActive(const AstroRule& rule, int nowMinute) const;
  int minuteForEvent(AstroEvent event) const;
  int timezoneOffsetMinutes() const;
  String formatMinute(int minuteOfDay) const;
  double normalizeDegrees(double value) const;
  double normalizeHours(double value) const;
  double degToRad(double degrees) const;
  double radToDeg(double radians) const;
};
