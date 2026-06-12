#include "HX711Manager.h"
#include "config.h"

void HX711Manager::begin(ConfigManager *cfg) {
  config = cfg;
  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
  applyConfig();
}

void HX711Manager::applyConfig() {
  const AppConfig &c = config->get();
  scale.set_offset(c.tareOffset);
  scale.set_scale(c.calibrationFactor);
  sampleCount = constrain(c.filterSamples, (uint8_t)1, (uint8_t)50);
  sampleIndex = 0;
  filled = false;
  firstSample = true;
}

void HX711Manager::update() {
  const AppConfig &c = config->get();
  if (millis() - lastSampleMs < c.sampleIntervalMs) return;
  lastSampleMs = millis();

  if (!scale.is_ready()) {
    valid = false;
    stable = false;
    err = "HX711 not ready";
    return;
  }

  long r = scale.read();
  float g = (r - c.tareOffset) / c.calibrationFactor;
  long previousRaw = lastRaw;
  lastRaw = r;

  if (isnan(g) || isinf(g) || abs(r) < 10) {
    valid = false;
    stable = false;
    err = "Invalid scale reading";
    return;
  }

  if (!firstSample && abs(r - previousRaw) > 10000000L) {
    valid = false;
    stable = false;
    err = "Unrealistic scale jump";
    return;
  }

  firstSample = false;
  valid = true;
  err = "";

  samples[sampleIndex] = g;
  sampleIndex = (sampleIndex + 1) % sampleCount;
  if (sampleIndex == 0) filled = true;

  uint8_t n = filled ? sampleCount : sampleIndex;
  if (n == 0) n = 1;
  float sum = 0;
  for (uint8_t i = 0; i < n; i++) sum += samples[i];
  float newFiltered = sum / n;

  if (abs(newFiltered - filtered) < 5.0f) {
    if (stableSinceMs == 0) stableSinceMs = millis();
  } else {
    stableSinceMs = millis();
  }
  filtered = newFiltered;
  stable = (millis() - stableSinceMs) >= 3000UL;
}

bool HX711Manager::isValid() const { return valid; }
bool HX711Manager::isStable() const { return valid && stable; }
float HX711Manager::weightG() const { return filtered; }
long HX711Manager::raw() const { return lastRaw; }
String HX711Manager::error() const { return err; }

bool HX711Manager::tare(uint8_t times) {
  if (!scale.is_ready()) return false;
  long total = 0;
  for (uint8_t i = 0; i < times; i++) {
    while (!scale.is_ready()) delay(5);
    total += scale.read();
    delay(5);
  }
  config->get().tareOffset = total / times;
  scale.set_offset(config->get().tareOffset);
  return config->save();
}

bool HX711Manager::calibrateWithKnownWeight(float knownWeightG, uint8_t times) {
  if (knownWeightG <= 0 || !scale.is_ready()) return false;
  long total = 0;
  for (uint8_t i = 0; i < times; i++) {
    while (!scale.is_ready()) delay(5);
    total += scale.read();
    delay(5);
  }
  long avg = total / times;
  long net = avg - config->get().tareOffset;
  if (net == 0) return false;
  config->get().calibrationFactor = (float)net / knownWeightG;
  scale.set_scale(config->get().calibrationFactor);
  return config->save();
}
