#pragma once

#include <Arduino.h>

static const char* EZLIGHT_NODE_ID = "ezlight_001";
static const char* EZLIGHT_NODE_TYPE = "ezlight";
static const char* EZLIGHT_PROFILE = "experimental.ezlight.v1";
static const char* EZLIGHT_MODEL = "ESP32-4Relay-EzLight";
static const char* EZLIGHT_FW = "ezlight-node";
static const char* EZLIGHT_VERSION = "0.2.0-managed";
static const uint8_t EZLIGHT_RELAY_COUNT = 4;
static const uint8_t EZLIGHT_MAX_SCHEDULE_RULES = 8;
static const uint8_t EZLIGHT_MAX_ASTRO_RULES = 4;

struct RelayDefinition {
  const char* id;
  const char* label;
  uint8_t gpio;
  bool activeLow;
};

enum class RelayMode : uint8_t { Manual, Schedule, Astro, Disabled };
enum class OverrideMode : uint8_t { None, OnFor5Min, OnFor15Min, OnFor30Min, OnFor1Hour, OffUntilNextEvent, ManualOn, ManualOff };
enum class AstroEvent : uint8_t { CivilDusk, CivilDawn };

struct RelayRuntimeState {
  bool logicalOn;
  uint8_t physicalLevel;
  RelayMode mode;
  OverrideMode overrideMode;
  unsigned long overrideExpiresAtMs;
};

struct CommandResult {
  bool ok;
  const char* status;
  const char* error;
  String resultJson;
};

struct TimeStatus {
  bool valid;
  bool wasEverValid;
  bool lostAfterBoot;
  String warning;
};

struct FixedScheduleRule {
  uint8_t relayIndex;
  uint16_t onMinute;
  uint16_t offMinute;
};

struct AstroRule {
  uint8_t relayIndex;
  AstroEvent onEvent;
  AstroEvent offEvent;
};

inline const char* relayModeToString(RelayMode mode) {
  switch (mode) {
    case RelayMode::Manual: return "manual";
    case RelayMode::Schedule: return "schedule";
    case RelayMode::Astro: return "astro";
    case RelayMode::Disabled: return "disabled";
  }
  return "unknown";
}

inline const char* overrideModeToString(OverrideMode mode) {
  switch (mode) {
    case OverrideMode::None: return "none";
    case OverrideMode::OnFor5Min: return "on_for_5_min";
    case OverrideMode::OnFor15Min: return "on_for_15_min";
    case OverrideMode::OnFor30Min: return "on_for_30_min";
    case OverrideMode::OnFor1Hour: return "on_for_1_hour";
    case OverrideMode::OffUntilNextEvent: return "off_until_next_event";
    case OverrideMode::ManualOn: return "manual_on";
    case OverrideMode::ManualOff: return "manual_off";
  }
  return "unknown";
}
