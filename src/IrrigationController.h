#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "ConfigManager.h"
#include "HX711Manager.h"
#include "PumpManager.h"
#include "LogManager.h"

enum SystemState : uint8_t {
  BOOTING,
  IDLE,
  WAITING_FOR_STABLE_READING,
  BELOW_THRESHOLD,
  IRRIGATING,
  COOLDOWN,
  CALIBRATION,
  SENSOR_ERROR,
  TANK_EMPTY,
  LEAK_DETECTED,
  CONFIG_ERROR,
  MANUAL_MODE,
  EMERGENCY_STOP
};

class IrrigationController {
public:
  void begin(ConfigManager *config, HX711Manager *hx, PumpManager *pump, LogManager *log);
  void update();
  SystemState state() const;
  const char *stateName() const;
  bool tankEmpty() const;
  bool leakDetected() const;
  bool emergencyStop() const;
  bool inManualMode() const;
  void manualOn(uint32_t runSec = 0);
  void manualOff(const String &reason = "Manual stop");
  void setEmergencyStop(bool active);
  void clearError();
  float triggerWeight() const;
  float stopWeight() const;
  uint32_t lastIrrigationAtMs() const;
private:
  ConfigManager *config = nullptr;
  HX711Manager *hx = nullptr;
  PumpManager *pump = nullptr;
  LogManager *log = nullptr;
  SystemState currentState = BOOTING;
  uint32_t belowSinceMs = 0;
  uint32_t lastIrrigationMs = 0;
  uint32_t manualStopAtMs = 0;
  bool eStop = false;
  bool manual = false;

  void transition(SystemState s, const String &msg = "");
  bool safetyInputsOk();
  bool canStartIrrigation();
  bool cooldownComplete() const;
  bool timeWindowOk() const;
  void stopWithState(SystemState s, const String &reason);
  const char *nameFor(SystemState s) const;
};
