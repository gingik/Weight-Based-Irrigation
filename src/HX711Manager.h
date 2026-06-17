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
  ConfigManager *config = nullptr;
  HX711 scale;
  bool valid = false;
  bool stable = false;
  String err;
  uint32_t lastSampleMs = 0;
  long lastRaw = 0;
  float filtered = 0.0f;
  float samples[50];
  uint8_t sampleCount = 0;
  uint8_t sampleIndex = 0;
  bool filled = false;
  bool firstSample = true;
  uint32_t stableSinceMs = 0;
  uint32_t _startupMs = 0;
};
