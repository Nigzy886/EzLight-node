#include "AstroEngine.h"
#include <math.h>
#include <time.h>

static const double CIVIL_ZENITH_DEGREES = 96.0;

AstroEngine::AstroEngine()
  : _hasLocation(false), _latitude(0.0), _longitude(0.0), _ruleCount(0), _lastYear(-1), _lastYday(-1), _dawnMinute(-1), _duskMinute(-1), _duskTime(""), _dawnTime(""), _warning("location_not_set") {}

void AstroEngine::configure(const EzLightConfig& config) {
  _ruleCount = config.astroRuleCount;
  for (uint8_t i = 0; i < _ruleCount; ++i) {
    _rules[i] = config.astroRules[i];
  }
  for (uint8_t i = _ruleCount; i < EZLIGHT_MAX_ASTRO_RULES; ++i) {
    _rules[i] = {0, AstroEvent::CivilDusk, AstroEvent::CivilDawn};
  }
  if (config.astroLocationSet) {
    setLocation(config.latitude, config.longitude);
  } else {
    _hasLocation = false;
    _dawnMinute = -1;
    _duskMinute = -1;
    _dawnTime = "";
    _duskTime = "";
    _warning = "location_not_set";
  }
}

bool AstroEngine::setLocation(double latitude, double longitude) {
  if (latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0) {
    _warning = "invalid_location";
    return false;
  }
  _latitude = latitude;
  _longitude = longitude;
  _hasLocation = true;
  _lastYear = -1;
  _lastYday = -1;
  _warning = "";
  return true;
}

void AstroEngine::recalculate(bool timeValid) {
  if (!timeValid) {
    _warning = "time_invalid";
    return;
  }
  if (!_hasLocation) {
    _warning = "location_not_set";
    return;
  }
  struct tm localTime;
  if (!getLocalTime(&localTime, 50)) {
    _warning = "time_invalid";
    return;
  }
  calculateForLocalDate(localTime);
}

void AstroEngine::update(bool timeValid, RelayDriver& relays) {
  if (!timeValid) {
    _warning = "time_invalid";
    return;
  }
  if (!_hasLocation) {
    _warning = "location_not_set";
    return;
  }

  struct tm localTime;
  if (!getLocalTime(&localTime, 50)) {
    _warning = "time_invalid";
    return;
  }
  if (localTime.tm_year != _lastYear || localTime.tm_yday != _lastYday) {
    calculateForLocalDate(localTime);
  }
  if (_dawnMinute < 0 || _duskMinute < 0) {
    return;
  }

  const int nowMinute = (localTime.tm_hour * 60) + localTime.tm_min;
  for (uint8_t i = 0; i < _ruleCount; ++i) {
    const AstroRule& rule = _rules[i];
    if (rule.relayIndex >= EZLIGHT_RELAY_COUNT || relays.mode(rule.relayIndex) != RelayMode::Astro) {
      continue;
    }
    relays.setRelay(relays.definition(rule.relayIndex).id, ruleIsActive(rule, nowMinute));
  }
}

const String& AstroEngine::duskTime() const { return _duskTime; }
const String& AstroEngine::dawnTime() const { return _dawnTime; }
const String& AstroEngine::warning() const { return _warning; }
bool AstroEngine::hasLocation() const { return _hasLocation; }

bool AstroEngine::calculateForLocalDate(const tm& localTime) {
  const int offsetMinutes = timezoneOffsetMinutes();
  int dawnMinute = -1;
  int duskMinute = -1;
  if (!calculateCivilEvent(localTime, true, offsetMinutes, dawnMinute) || !calculateCivilEvent(localTime, false, offsetMinutes, duskMinute)) {
    _dawnMinute = -1;
    _duskMinute = -1;
    _dawnTime = "";
    _duskTime = "";
    _warning = "civil_twilight_unavailable";
    return false;
  }
  _dawnMinute = dawnMinute;
  _duskMinute = duskMinute;
  _dawnTime = formatMinute(_dawnMinute);
  _duskTime = formatMinute(_duskMinute);
  _lastYear = localTime.tm_year;
  _lastYday = localTime.tm_yday;
  _warning = "";
  return true;
}

bool AstroEngine::calculateCivilEvent(const tm& localTime, bool dawn, int offsetMinutes, int& minuteOfDay) const {
  const double dayOfYear = static_cast<double>(localTime.tm_yday + 1);
  const double lngHour = _longitude / 15.0;
  const double approxTime = dayOfYear + (((dawn ? 6.0 : 18.0) - lngHour) / 24.0);
  const double meanAnomaly = (0.9856 * approxTime) - 3.289;
  double trueLongitude = meanAnomaly + (1.916 * sin(degToRad(meanAnomaly))) + (0.020 * sin(degToRad(2.0 * meanAnomaly))) + 282.634;
  trueLongitude = normalizeDegrees(trueLongitude);

  double rightAscension = radToDeg(atan(0.91764 * tan(degToRad(trueLongitude))));
  rightAscension = normalizeDegrees(rightAscension);
  const double longitudeQuadrant = floor(trueLongitude / 90.0) * 90.0;
  const double rightAscensionQuadrant = floor(rightAscension / 90.0) * 90.0;
  rightAscension = (rightAscension + longitudeQuadrant - rightAscensionQuadrant) / 15.0;

  const double sinDec = 0.39782 * sin(degToRad(trueLongitude));
  const double cosDec = cos(asin(sinDec));
  const double cosHour = (cos(degToRad(CIVIL_ZENITH_DEGREES)) - (sinDec * sin(degToRad(_latitude)))) / (cosDec * cos(degToRad(_latitude)));
  if (cosHour > 1.0 || cosHour < -1.0) {
    return false;
  }

  double hourAngle = dawn ? (360.0 - radToDeg(acos(cosHour))) : radToDeg(acos(cosHour));
  hourAngle /= 15.0;
  const double localMeanTime = hourAngle + rightAscension - (0.06571 * approxTime) - 6.622;
  const double utcHour = normalizeHours(localMeanTime - lngHour);
  const double localHour = normalizeHours(utcHour + (static_cast<double>(offsetMinutes) / 60.0));
  minuteOfDay = static_cast<int>(floor((localHour * 60.0) + 0.5)) % 1440;
  return true;
}

bool AstroEngine::ruleIsActive(const AstroRule& rule, int nowMinute) const {
  const int onMinute = minuteForEvent(rule.onEvent);
  const int offMinute = minuteForEvent(rule.offEvent);
  if (onMinute < 0 || offMinute < 0 || onMinute == offMinute) {
    return false;
  }
  if (onMinute < offMinute) {
    return nowMinute >= onMinute && nowMinute < offMinute;
  }
  return nowMinute >= onMinute || nowMinute < offMinute;
}

int AstroEngine::minuteForEvent(AstroEvent event) const {
  switch (event) {
    case AstroEvent::CivilDusk: return _duskMinute;
    case AstroEvent::CivilDawn: return _dawnMinute;
  }
  return -1;
}

int AstroEngine::timezoneOffsetMinutes() const {
  const time_t now = time(nullptr);
  struct tm localTime;
  struct tm utcTime;
  localtime_r(&now, &localTime);
  gmtime_r(&now, &utcTime);
  return static_cast<int>(difftime(mktime(&localTime), mktime(&utcTime)) / 60);
}

String AstroEngine::formatMinute(int minuteOfDay) const {
  char buffer[6];
  snprintf(buffer, sizeof(buffer), "%02d:%02d", minuteOfDay / 60, minuteOfDay % 60);
  return String(buffer);
}

double AstroEngine::normalizeDegrees(double value) const {
  while (value < 0.0) { value += 360.0; }
  while (value >= 360.0) { value -= 360.0; }
  return value;
}

double AstroEngine::normalizeHours(double value) const {
  while (value < 0.0) { value += 24.0; }
  while (value >= 24.0) { value -= 24.0; }
  return value;
}

double AstroEngine::degToRad(double degrees) const { return degrees * PI / 180.0; }
double AstroEngine::radToDeg(double radians) const { return radians * 180.0 / PI; }
