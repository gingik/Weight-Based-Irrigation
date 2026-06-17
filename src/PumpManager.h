#pragma once
#include <Arduino.h>
#include "ConfigManager.h"
#include "LogManager.h"

class PumpManager {
public:
  void begin(ConfigManager *config, LogManager *log);
  void setPump(bool on, const String &reason = "");
  bool isOn() const;
  uint32_t startedAtMs() const;
  uint32_t lastStopAtMs() const;
  uint32_t lastDurationSec() const;
private:
  ConfigManager *config = nullptr;
  LogManager *log = nullptr;
  bool pumpOn = false;
  uint32_t startMs = 0;
  uint32_t stopMs = 0;
  uint32_t lastDuration = 0;
  uint32_t _lastChangeMs = 0;
  void writeRelay(bool on);
};
