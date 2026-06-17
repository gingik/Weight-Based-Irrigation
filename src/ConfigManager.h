#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

enum TriggerMode : uint8_t {
  TRIGGER_ABSOLUTE = 0,
  TRIGGER_DRYBACK = 1
};

struct AppConfig {
  char deviceName[40];
  char wifiSsid[32];
  char wifiPassword[64];
  TriggerMode triggerMode;

  float calibrationFactor;
  long tareOffset;

  float triggerWeightG;
  float stopWeightG;
  float fullyWetWeightG;
  float dryBackTriggerPercent;
  float dryBackStopPercent;

  uint32_t maxRuntimeSec;
  uint32_t minGapMin;
  uint32_t stableDurationSec;
  uint32_t sampleIntervalMs;
  uint8_t filterSamples;

  bool relayActiveHigh;
  bool tankSensorEnabled;
  bool leakSensorEnabled;
  bool tankEmptyActiveLow;
  bool leakActiveLow;
  bool stopOnWifiLoss;

  bool timeWindowEnabled;
  uint16_t windowStartMin; // minutes after midnight
  uint16_t windowEndMin;
  bool allowIrrigationWithoutValidTime;
};

class ConfigManager {
public:
  void begin();
  AppConfig &get();
  const AppConfig &get() const;
  bool save();
  void resetDefaults();
  bool validate(const AppConfig &cfg, String &error) const;
  float computedTriggerWeight(const AppConfig &cfg) const;
  float computedStopWeight(const AppConfig &cfg) const;

private:
  Preferences prefs;
  AppConfig config;
  void applyDefaults();
};
