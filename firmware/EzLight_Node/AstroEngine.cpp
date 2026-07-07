#include "AstroEngine.h"

AstroEngine::AstroEngine() : _hasLocation(false), _latitude(0.0), _longitude(0.0), _duskTime(""), _dawnTime(""), _warning("location_not_set") {}

bool AstroEngine::setLocation(double latitude, double longitude) {
  if (latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0) {
    _warning = "invalid_location";
    return false;
  }
  _latitude = latitude;
  _longitude = longitude;
  _hasLocation = true;
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
  // TODO: Replace placeholder with civil dusk/civil dawn calculation.
  _duskTime = "civil_dusk_pending";
  _dawnTime = "civil_dawn_pending";
  _warning = "astro_algorithm_pending";
}

const String& AstroEngine::duskTime() const { return _duskTime; }
const String& AstroEngine::dawnTime() const { return _dawnTime; }
const String& AstroEngine::warning() const { return _warning; }
bool AstroEngine::hasLocation() const { return _hasLocation; }
