#pragma once

#include <Arduino.h>

class AstroEngine {
public:
  AstroEngine();
  bool setLocation(double latitude, double longitude);
  void recalculate(bool timeValid);
  const String& duskTime() const;
  const String& dawnTime() const;
  const String& warning() const;
  bool hasLocation() const;

private:
  bool _hasLocation;
  double _latitude;
  double _longitude;
  String _duskTime;
  String _dawnTime;
  String _warning;
};
