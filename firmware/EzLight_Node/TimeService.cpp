#include "TimeService.h"
#include <time.h>

TimeService::TimeService() : _status{false, false, false, "time_not_synced"} {}

void TimeService::beginNtp(const char* timezone) {
  setenv("TZ", timezone, 1);
  tzset();
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
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
