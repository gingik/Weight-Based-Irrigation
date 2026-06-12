#include "IrrigationController.h"
#include "config.h"
#include <time.h>

void IrrigationController::begin(ConfigManager *cfg, HX711Manager *h, PumpManager *p, LogManager *lg) {
  config = cfg;
  hx = h;
  pump = p;
  log = lg;
  pinMode(MANUAL_BUTTON_PIN, INPUT_PULLUP);
  pinMode(TANK_EMPTY_PIN, INPUT_PULLUP);
  pinMode(LEAK_SENSOR_PIN, INPUT_PULLUP);
  transition(IDLE, "Controller ready");
}

void IrrigationController::update() {
  const AppConfig &c = config->get();

  if (eStop) {
    if (pump->isOn()) pump->setPump(false, "Emergency stop active");
    if (currentState != EMERGENCY_STOP) transition(EMERGENCY_STOP, "Emergency stop active");
    return;
  }

  if (!safetyInputsOk()) return;

  if (c.stopOnWifiLoss && WiFi.status() != WL_CONNECTED && pump->isOn()) {
    stopWithState(COOLDOWN, "Wi-Fi lost");
    return;
  }

  if (!hx->isValid()) {
    if (pump->isOn()) pump->setPump(false, "Sensor error");
    if (currentState != SENSOR_ERROR) transition(SENSOR_ERROR, hx->error());
    return;
  }

  if (manual) {
    if (manualStopAtMs > 0 && millis() >= manualStopAtMs) manualOff("Manual runtime finished");
    return;
  }

  if (pump->isOn()) {
    uint32_t runSec = (millis() - pump->startedAtMs()) / 1000UL;
    if (hx->weightG() >= stopWeight()) {
      stopWithState(COOLDOWN, "Stop weight reached");
      return;
    }
    if (runSec >= c.maxRuntimeSec) {
      stopWithState(COOLDOWN, "Max runtime reached");
      return;
    }
  }

  switch (currentState) {
    case IDLE:
    case COOLDOWN:
      if (currentState == COOLDOWN && !cooldownComplete()) return;
      if (currentState == COOLDOWN && cooldownComplete()) transition(IDLE, "Cooldown complete");
      if (canStartIrrigation()) {
        belowSinceMs = millis();
        transition(BELOW_THRESHOLD, "Weight below trigger");
      }
      break;

    case BELOW_THRESHOLD:
      if (!canStartIrrigation()) {
        transition(IDLE, "Start condition no longer true");
        break;
      }
      if (millis() - belowSinceMs >= c.stableDurationSec * 1000UL) {
        pump->setPump(true, "Irrigation started by weight threshold");
        transition(IRRIGATING, "Pump on");
      }
      break;

    case IRRIGATING:
      // Stop conditions handled above.
      break;

    case SENSOR_ERROR:
      if (hx->isValid() && hx->isStable()) transition(IDLE, "Sensor recovered");
      break;

    default:
      break;
  }
}

bool IrrigationController::safetyInputsOk() {
  const AppConfig &c = config->get();
  bool tank = tankEmpty();
  bool leak = leakDetected();
  if (tank) {
    if (pump->isOn()) pump->setPump(false, "Tank empty");
    if (currentState != TANK_EMPTY) transition(TANK_EMPTY, "Tank empty sensor active");
    return false;
  }
  if (leak) {
    if (pump->isOn()) pump->setPump(false, "Leak detected");
    if (currentState != LEAK_DETECTED) transition(LEAK_DETECTED, "Leak sensor active");
    return false;
  }
  if (currentState == TANK_EMPTY || currentState == LEAK_DETECTED) transition(IDLE, "Safety input cleared");
  return true;
}

bool IrrigationController::canStartIrrigation() {
  if (!hx->isValid() || !hx->isStable()) return false;
  if (pump->isOn()) return false;
  if (!cooldownComplete()) return false;
  if (!timeWindowOk()) return false;
  if (currentState == SENSOR_ERROR || currentState == CONFIG_ERROR || currentState == EMERGENCY_STOP || currentState == CALIBRATION) return false;
  return hx->weightG() <= triggerWeight();
}

bool IrrigationController::cooldownComplete() const {
  if (lastIrrigationMs == 0) return true;
  uint32_t gapMs = config->get().minGapMin * 60UL * 1000UL;
  return millis() - lastIrrigationMs >= gapMs;
}

bool IrrigationController::timeWindowOk() const {
  const AppConfig &c = config->get();
  if (!c.timeWindowEnabled) return true;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 0)) return c.allowIrrigationWithoutValidTime;
  uint16_t nowMin = timeinfo.tm_hour * 60 + timeinfo.tm_min;
  if (c.windowStartMin == c.windowEndMin) return true;
  if (c.windowStartMin < c.windowEndMin) return nowMin >= c.windowStartMin && nowMin <= c.windowEndMin;
  return nowMin >= c.windowStartMin || nowMin <= c.windowEndMin;
}

void IrrigationController::stopWithState(SystemState s, const String &reason) {
  pump->setPump(false, reason);
  lastIrrigationMs = millis();
  transition(s, reason);
}

void IrrigationController::manualOn(uint32_t runSec) {
  if (eStop || !safetyInputsOk()) return;
  manual = true;
  transition(MANUAL_MODE, "Manual mode");
  uint32_t cap = runSec == 0 ? MAX_MANUAL_RUNTIME_SEC : min(runSec, (uint32_t)MAX_MANUAL_RUNTIME_SEC);
  manualStopAtMs = millis() + cap * 1000UL;
  pump->setPump(true, "Manual pump on");
}

void IrrigationController::manualOff(const String &reason) {
  if (pump->isOn()) pump->setPump(false, reason);
  manual = false;
  manualStopAtMs = 0;
  lastIrrigationMs = millis();
  transition(COOLDOWN, reason);
}

void IrrigationController::setEmergencyStop(bool active) {
  eStop = active;
  if (active) {
    if (pump->isOn()) pump->setPump(false, "Emergency stop");
    manual = false;
    transition(EMERGENCY_STOP, "Emergency stop set");
  } else {
    transition(IDLE, "Emergency stop cleared");
  }
}

void IrrigationController::clearError() {
  if (currentState == SENSOR_ERROR || currentState == TANK_EMPTY || currentState == LEAK_DETECTED || currentState == CONFIG_ERROR) {
    transition(IDLE, "Error cleared by user");
  }
}

void IrrigationController::transition(SystemState s, const String &msg) {
  if (currentState == s) return;
  currentState = s;
  if (log) log->add("STATE", String(nameFor(s)) + (msg.length() ? (" - " + msg) : ""));
}

SystemState IrrigationController::state() const { return currentState; }
const char *IrrigationController::stateName() const { return nameFor(currentState); }
bool IrrigationController::emergencyStop() const { return eStop; }
bool IrrigationController::inManualMode() const { return manual; }
float IrrigationController::triggerWeight() const { return config->computedTriggerWeight(config->get()); }
float IrrigationController::stopWeight() const { return config->computedStopWeight(config->get()); }
uint32_t IrrigationController::lastIrrigationAtMs() const { return lastIrrigationMs; }

bool IrrigationController::tankEmpty() const {
  const AppConfig &c = config->get();
  if (!c.tankSensorEnabled) return false;
  int v = digitalRead(TANK_EMPTY_PIN);
  return c.tankEmptyActiveLow ? v == LOW : v == HIGH;
}

bool IrrigationController::leakDetected() const {
  const AppConfig &c = config->get();
  if (!c.leakSensorEnabled) return false;
  int v = digitalRead(LEAK_SENSOR_PIN);
  return c.leakActiveLow ? v == LOW : v == HIGH;
}

const char *IrrigationController::nameFor(SystemState s) const {
  switch (s) {
    case BOOTING: return "BOOTING";
    case IDLE: return "IDLE";
    case WAITING_FOR_STABLE_READING: return "WAITING_FOR_STABLE_READING";
    case BELOW_THRESHOLD: return "BELOW_THRESHOLD";
    case IRRIGATING: return "IRRIGATING";
    case COOLDOWN: return "COOLDOWN";
    case CALIBRATION: return "CALIBRATION";
    case SENSOR_ERROR: return "SENSOR_ERROR";
    case TANK_EMPTY: return "TANK_EMPTY";
    case LEAK_DETECTED: return "LEAK_DETECTED";
    case CONFIG_ERROR: return "CONFIG_ERROR";
    case MANUAL_MODE: return "MANUAL_MODE";
    case EMERGENCY_STOP: return "EMERGENCY_STOP";
    default: return "UNKNOWN";
  }
}
