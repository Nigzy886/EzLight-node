#include "TimeService.h"
#include <time.h>

namespace {
const char* NZ_TIMEZONE_POSIX = "NZST-12NZDT,M9.5.0,M4.1.0/3";

String normalizeTimezone(const char* timezone) {
  const String requested = timezone == nullptr ? String() : String(timezone);
  if (requested.length() == 0 || requested == "Pacific/Auckland" || requested == "NZ" || requested == "NZST") {
    return String(NZ_TIMEZONE_POSIX);
  }
  return requested;
}
}  // namespace

TimeService::TimeService()
  : _status{false, false, false, "time_not_synced"},
    _timezone(NZ_TIMEZONE_POSIX) {}

void TimeService::beginNtp(const char* timezone) {
  _timezone = normalizeTimezone(timezone);
  configTzTime(_timezone.c_str(), "pool.ntp.org", "time.nist.gov");
}

void TimeService::update() {
  struct tm timeInfo;
  const bool nowValid = getLocalTime(&timeInfo, 50);
  if (nowValid) {
    _status.valid = true;
    _status.wasEverValid = true;
    _status.lostAfterBoot = false;
    _status.warning = "";
    return;
  }
  _status.valid = false;
  if (_status.wasEverValid) {
    _status.lostAfterBoot = true;
    _status.warning = "time_lost";
  } else {
    _status.warning = "time_not_synced";
  }
}

const TimeStatus& TimeService::status() const { return _status; }
bool TimeService::valid() const { return _status.valid; }
const String& TimeService::timezone() const { return _timezone; }

String TimeService::isoNow() const {
  if (!_status.valid) {
    return "";
  }
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo, 50)) {
    return "";
  }
  char buffer[32];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S%z", &timeInfo);
  return String(buffer);
}
