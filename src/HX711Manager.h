#pragma once
#include <Arduino.h>
#include <HX711.h>
#include "ConfigManager.h"

class HX711Manager {
public:
  void begin(ConfigManager *config);
  void update();
  bool isValid() const;
  bool isStable() const;
  float weightG() const;
  long raw() const;
  String error() const;
  bool tare(uint8_t times = 20);
  bool calibrateWithKnownWeight(float knownWeightG, uint8_t times = 20);
  void applyConfig();
private:
  void reinit();
  ConfigManager *config = nullptr;
  HX711 scale;
  bool valid = false;
  bool stable = false;
  String err;
  uint32_t lastSampleMs = 0;
  long lastRaw = 0;
  float filtered = 0.0f;
  float _ema = 0.0f;
  float _emaAlpha = 1.0f;
  bool firstSample = true;
  uint32_t stableSinceMs = 0;
  uint32_t _startupMs = 0;
  uint8_t _errorCount = 0;
  uint32_t _lastReinitMs = 0;
  bool _persistentFault = false;
  uint32_t _lastErrorLogMs = 0;
};
