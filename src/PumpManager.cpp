#include "PumpManager.h"
#include "config.h"

void PumpManager::begin(ConfigManager *cfg, LogManager *lg) {
  config = cfg;
  log = lg;
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  writeRelay(false); // Pump OFF immediately on boot.
  pumpOn = false;
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
}

void PumpManager::writeRelay(bool on) {
  bool level = config ? (config->get().relayActiveHigh ? on : !on) : on;
  digitalWrite(PUMP_RELAY_PIN, level ? HIGH : LOW);
  digitalWrite(STATUS_LED_PIN, on ? HIGH : LOW);
}

void PumpManager::setPump(bool on, const String &reason) {
  if (on == pumpOn) return;
  pumpOn = on;
  writeRelay(on);
  if (on) {
    startMs = millis();
    if (log) log->add("PUMP_ON", reason.length() ? reason : "Pump started");
  } else {
    stopMs = millis();
    if (startMs > 0) lastDuration = (stopMs - startMs) / 1000UL;
    if (log) log->add("PUMP_OFF", reason.length() ? reason : "Pump stopped");
  }
}

bool PumpManager::isOn() const { return pumpOn; }
uint32_t PumpManager::startedAtMs() const { return startMs; }
uint32_t PumpManager::lastStopAtMs() const { return stopMs; }
uint32_t PumpManager::lastDurationSec() const { return lastDuration; }
