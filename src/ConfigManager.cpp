#include "ConfigManager.h"

void ConfigManager::begin() {
  prefs.begin("irrigation", false);
  applyDefaults();

  String name = prefs.getString("name", DEFAULT_DEVICE_NAME);
  name.toCharArray(config.deviceName, sizeof(config.deviceName));
  String ssid = prefs.getString("ssid", WIFI_SSID);
  ssid.toCharArray(config.wifiSsid, sizeof(config.wifiSsid));
  String pwd = prefs.getString("pwd", WIFI_PASSWORD);
  pwd.toCharArray(config.wifiPassword, sizeof(config.wifiPassword));
  config.triggerMode = (TriggerMode)prefs.getUChar("mode", TRIGGER_ABSOLUTE);
  config.calibrationFactor = prefs.getFloat("cal", 1.0f);
  config.tareOffset = prefs.getLong("tare", 0);
  config.triggerWeightG = prefs.getFloat("trigG", DEFAULT_TRIGGER_WEIGHT_G);
  config.stopWeightG = prefs.getFloat("stopG", DEFAULT_STOP_WEIGHT_G);
  config.fullyWetWeightG = prefs.getFloat("wetG", DEFAULT_FULLY_WET_WEIGHT_G);
  config.dryBackTriggerPercent = prefs.getFloat("dbTrig", DEFAULT_DRYBACK_TRIGGER_PERCENT);
  config.dryBackStopPercent = prefs.getFloat("dbStop", DEFAULT_DRYBACK_STOP_PERCENT);
  config.maxRuntimeSec = prefs.getUInt("maxRun", DEFAULT_MAX_RUNTIME_SEC);
  config.minGapMin = prefs.getUInt("minGap", DEFAULT_MIN_GAP_MIN);
  config.stableDurationSec = prefs.getUInt("stable", DEFAULT_STABLE_DURATION_SEC);
  config.sampleIntervalMs = prefs.getUInt("sampMs", DEFAULT_SAMPLE_INTERVAL_MS);
  config.filterSamples = prefs.getUChar("samples", DEFAULT_FILTER_SAMPLES);
  config.relayActiveHigh = prefs.getBool("relayHi", DEFAULT_RELAY_ACTIVE_HIGH);
  config.tankSensorEnabled = prefs.getBool("tankEn", DEFAULT_TANK_SENSOR_ENABLED);
  config.leakSensorEnabled = prefs.getBool("leakEn", DEFAULT_LEAK_SENSOR_ENABLED);
  config.tankEmptyActiveLow = prefs.getBool("tankLow", DEFAULT_TANK_EMPTY_ACTIVE_LOW);
  config.leakActiveLow = prefs.getBool("leakLow", DEFAULT_LEAK_ACTIVE_LOW);
  config.stopOnWifiLoss = prefs.getBool("wifiStop", DEFAULT_STOP_ON_WIFI_LOSS);
  config.timeWindowEnabled = prefs.getBool("winEn", DEFAULT_TIME_WINDOW_ENABLED);
  config.windowStartMin = prefs.getUShort("winStart", 7 * 60);
  config.windowEndMin = prefs.getUShort("winEnd", 19 * 60);
  config.allowIrrigationWithoutValidTime = prefs.getBool("allowNoT", DEFAULT_ALLOW_WITHOUT_VALID_TIME);
}

void ConfigManager::applyDefaults() {
  memset(&config, 0, sizeof(config));
  strncpy(config.deviceName, DEFAULT_DEVICE_NAME, sizeof(config.deviceName) - 1);
  strncpy(config.wifiSsid, WIFI_SSID, sizeof(config.wifiSsid) - 1);
  strncpy(config.wifiPassword, WIFI_PASSWORD, sizeof(config.wifiPassword) - 1);
  config.triggerMode = TRIGGER_ABSOLUTE;
  config.calibrationFactor = 1.0f;
  config.tareOffset = 0;
  config.triggerWeightG = DEFAULT_TRIGGER_WEIGHT_G;
  config.stopWeightG = DEFAULT_STOP_WEIGHT_G;
  config.fullyWetWeightG = DEFAULT_FULLY_WET_WEIGHT_G;
  config.dryBackTriggerPercent = DEFAULT_DRYBACK_TRIGGER_PERCENT;
  config.dryBackStopPercent = DEFAULT_DRYBACK_STOP_PERCENT;
  config.maxRuntimeSec = DEFAULT_MAX_RUNTIME_SEC;
  config.minGapMin = DEFAULT_MIN_GAP_MIN;
  config.stableDurationSec = DEFAULT_STABLE_DURATION_SEC;
  config.sampleIntervalMs = DEFAULT_SAMPLE_INTERVAL_MS;
  config.filterSamples = DEFAULT_FILTER_SAMPLES;
  config.relayActiveHigh = DEFAULT_RELAY_ACTIVE_HIGH;
  config.tankSensorEnabled = DEFAULT_TANK_SENSOR_ENABLED;
  config.leakSensorEnabled = DEFAULT_LEAK_SENSOR_ENABLED;
  config.tankEmptyActiveLow = DEFAULT_TANK_EMPTY_ACTIVE_LOW;
  config.leakActiveLow = DEFAULT_LEAK_ACTIVE_LOW;
  config.stopOnWifiLoss = DEFAULT_STOP_ON_WIFI_LOSS;
  config.timeWindowEnabled = DEFAULT_TIME_WINDOW_ENABLED;
  config.windowStartMin = 7 * 60;
  config.windowEndMin = 19 * 60;
  config.allowIrrigationWithoutValidTime = DEFAULT_ALLOW_WITHOUT_VALID_TIME;
}

AppConfig &ConfigManager::get() { return config; }
const AppConfig &ConfigManager::get() const { return config; }

bool ConfigManager::validate(const AppConfig &cfg, String &error) const {
  if (strlen(cfg.deviceName) == 0) { error = "Device name cannot be empty"; return false; }
  if (cfg.calibrationFactor == 0.0f || isnan(cfg.calibrationFactor)) { error = "Calibration factor cannot be zero"; return false; }
  if (cfg.maxRuntimeSec == 0) { error = "Max runtime must be greater than zero"; return false; }
  if (cfg.filterSamples < 1 || cfg.filterSamples > 50) { error = "Filter sample count must be 1-50"; return false; }
  if (cfg.sampleIntervalMs < 100 || cfg.sampleIntervalMs > 10000) { error = "Sample interval must be 100-10000ms"; return false; }
  if (cfg.stableDurationSec > 3600) { error = "Stable duration is too long"; return false; }
  if (cfg.triggerMode == TRIGGER_ABSOLUTE) {
    if (cfg.stopWeightG <= cfg.triggerWeightG) { error = "Stop weight must be higher than trigger weight"; return false; }
  } else {
    if (cfg.fullyWetWeightG <= 0) { error = "Fully wet weight must be greater than zero"; return false; }
    if (cfg.dryBackTriggerPercent < 0 || cfg.dryBackTriggerPercent > 100) { error = "Trigger dry-back must be 0-100%"; return false; }
    if (cfg.dryBackStopPercent < 0 || cfg.dryBackStopPercent > 100) { error = "Stop dry-back must be 0-100%"; return false; }
    if (cfg.dryBackStopPercent >= cfg.dryBackTriggerPercent) { error = "Stop dry-back must be lower than trigger dry-back"; return false; }
  }
  if (cfg.windowStartMin > 1439 || cfg.windowEndMin > 1439) { error = "Time window values must be inside a day"; return false; }
  return true;
}

float ConfigManager::computedTriggerWeight(const AppConfig &cfg) const {
  if (cfg.triggerMode == TRIGGER_ABSOLUTE) return cfg.triggerWeightG;
  return cfg.fullyWetWeightG * (1.0f - cfg.dryBackTriggerPercent / 100.0f);
}

float ConfigManager::computedStopWeight(const AppConfig &cfg) const {
  if (cfg.triggerMode == TRIGGER_ABSOLUTE) return cfg.stopWeightG;
  return cfg.fullyWetWeightG * (1.0f - cfg.dryBackStopPercent / 100.0f);
}

bool ConfigManager::save() {
  String err;
  if (!validate(config, err)) {
    Serial.printf("Config validation failed: %s\n", err.c_str());
    return false;
  }
  prefs.putString("name", config.deviceName);
  prefs.putString("ssid", config.wifiSsid);
  prefs.putString("pwd", config.wifiPassword);
  prefs.putUChar("mode", (uint8_t)config.triggerMode);
  prefs.putFloat("cal", config.calibrationFactor);
  prefs.putLong("tare", config.tareOffset);
  prefs.putFloat("trigG", config.triggerWeightG);
  prefs.putFloat("stopG", config.stopWeightG);
  prefs.putFloat("wetG", config.fullyWetWeightG);
  prefs.putFloat("dbTrig", config.dryBackTriggerPercent);
  prefs.putFloat("dbStop", config.dryBackStopPercent);
  prefs.putUInt("maxRun", config.maxRuntimeSec);
  prefs.putUInt("minGap", config.minGapMin);
  prefs.putUInt("stable", config.stableDurationSec);
  prefs.putUInt("sampMs", config.sampleIntervalMs);
  prefs.putUChar("samples", config.filterSamples);
  prefs.putBool("relayHi", config.relayActiveHigh);
  prefs.putBool("tankEn", config.tankSensorEnabled);
  prefs.putBool("leakEn", config.leakSensorEnabled);
  prefs.putBool("tankLow", config.tankEmptyActiveLow);
  prefs.putBool("leakLow", config.leakActiveLow);
  prefs.putBool("wifiStop", config.stopOnWifiLoss);
  prefs.putBool("winEn", config.timeWindowEnabled);
  prefs.putUShort("winStart", config.windowStartMin);
  prefs.putUShort("winEnd", config.windowEndMin);
  prefs.putBool("allowNoT", config.allowIrrigationWithoutValidTime);
  return true;
}

void ConfigManager::resetDefaults() {
  prefs.clear();
  applyDefaults();
  save();
}
