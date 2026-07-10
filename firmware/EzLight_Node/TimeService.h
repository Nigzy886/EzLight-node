#pragma once

#include <Arduino.h>
#include "EzLightTypes.h"

class TimeService {
public:
  TimeService();
  void beginNtp(const char* timezone);
  void update();
  const TimeStatus& status() const;
  bool valid() const;
  String isoNow() const;

private:
  TimeStatus _status;
};
