#include "HX711Manager.h"
#include "config.h"

void HX711Manager::begin(ConfigManager *cfg) {
  config = cfg;
  _startupMs = millis();
  Serial.println("[HX711] Initializing...");
  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
  Serial.printf("[HX711] DOUT=%d SCK=%d\n", HX711_DOUT_PIN, HX711_SCK_PIN);

  // Wait for HX711 power-on, max 2s
  uint32_t t = millis();
  while (!scale.is_ready() && millis() - t < 2000) delay(10);

  if (!scale.is_ready()) {
    Serial.println("[HX711] FATAL: no response within 2s — check wiring/power");
  } else {
    Serial.printf("[HX711] Ready after %lums\n", millis() - t);
    // Discard first conversion — often garbage
    scale.read();
    Serial.println("[HX711] First conversion discarded");
  }

  Serial.printf("[HX711] DOUT pin direct read=%d\n", digitalRead(HX711_DOUT_PIN));
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
  Serial.printf("[HX711] Config applied — tare: %ld, cal: %.4f, samples: %d, interval: %lums\n",
    c.tareOffset, c.calibrationFactor, sampleCount, c.sampleIntervalMs);
}

void HX711Manager::update() {
  const AppConfig &c = config->get();

  if (millis() - lastSampleMs < c.sampleIntervalMs) return;
  lastSampleMs = millis();

  // Give HX711 a startup warmup period (resets on begin, not wall-clock)
  if (millis() - _startupMs < 5000 && !scale.is_ready()) {
    valid = true;
    stable = false;
    err = "HX711 warming up";
    return;
  }

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
    Serial.printf("[HX711] ERROR: invalid reading — raw=%ld g=%.2f\n", r, g);
    valid = false;
    stable = false;
    err = "Invalid scale reading";
    return;
  }

  if (!firstSample && abs(r - previousRaw) > 500000L) {
    Serial.printf("[HX711] ERROR: unrealistic jump — prev=%ld cur=%ld delta=%ld\n",
      previousRaw, r, abs(r - previousRaw));
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

  bool wasStable = stable;
  if (abs(newFiltered - filtered) < 5.0f) {
    if (stableSinceMs == 0) stableSinceMs = millis();
  } else {
    stableSinceMs = millis();
  }
  filtered = newFiltered;
  stable = (millis() - stableSinceMs) >= 3000UL;

  // Log every sample at DEBUG verbosity — gate behind a compile-time flag
  // to avoid flooding serial at normal operation
#ifdef HX711_DEBUG_VERBOSE
  Serial.printf("[HX711] raw=%ld  g=%.2f  filt=%.2f  n=%d  stable=%d\n",
    r, g, filtered, n, stable);
#endif

  // Log stability transitions
  if (!wasStable && stable)
    Serial.printf("[HX711] STABLE — filtered=%.2fg\n", filtered);
  else if (wasStable && !stable)
    Serial.printf("[HX711] UNSTABLE — delta=%.2fg from previous\n", abs(newFiltered - filtered));
}

bool HX711Manager::isValid() const { return valid; }
bool HX711Manager::isStable() const { return valid && stable; }
float HX711Manager::weightG() const { return filtered; }
long HX711Manager::raw() const { return lastRaw; }
String HX711Manager::error() const { return err; }


bool HX711Manager::tare(uint8_t times) {
  if (!scale.is_ready()) return false;

  long total = 0;

  Serial.printf("[HX711] Tare start (%u samples)...\n", times);

  for (uint8_t i = 0; i < times; i++) {
    while (!scale.is_ready()) delay(5);

    long sample = scale.read();
    Serial.printf("[HX711] Tare sample %u/%u: %ld\n", i + 1, times, sample);

    total += sample;
    delay(5);
  }

  long newOffset = total / times;

  config->get().tareOffset = newOffset;
  scale.set_offset(newOffset);

  // Clear old filtered values after tare
  sampleIndex = 0;
  filled = false;
  firstSample = true;
  stableSinceMs = 0;
  filtered = 0.0f;
  lastRaw = newOffset;
  valid = true;
  stable = false;
  err = "";

  Serial.printf("[HX711] Tare OK — offset=%ld\n", newOffset);

  return config->save();
}

bool HX711Manager::calibrateWithKnownWeight(float knownWeightG, uint8_t times) {
  Serial.printf("[HX711] Calibrate start — known=%.2fg (%d samples)...\n", knownWeightG, times);
  if (knownWeightG <= 0 || !scale.is_ready()) {
    Serial.printf("[HX711] Calibrate FAILED: knownWeight=%.2f ready=%d\n",
      knownWeightG, scale.is_ready());
    return false;
  }
  long total = 0;
  for (uint8_t i = 0; i < times; i++) {
    while (!scale.is_ready()) delay(5);
    long s = scale.read();
    total += s;
    Serial.printf("[HX711] Cal sample %d/%d: %ld\n", i + 1, times, s);
    delay(5);
  }
  long avg = total / times;
  long net = avg - config->get().tareOffset;
  Serial.printf("[HX711] Cal avg=%ld  net=%ld  tare=%ld\n",
    avg, net, config->get().tareOffset);
  if (net == 0) {
    Serial.println("[HX711] Calibrate FAILED: net=0 (tare not set?)");
    return false;
  }
  float factor = (float)net / knownWeightG;
  config->get().calibrationFactor = factor;
  scale.set_scale(factor);

  // Reset filtered ring buffer so stale readings don't mix with new scale
  sampleIndex = 0;
  filled = false;
  firstSample = true;
  stableSinceMs = 0;
  filtered = 0.0f;
  valid = true;
  stable = false;
  err = "";

  bool ok = config->save();
  Serial.printf("[HX711] Calibrate %s — factor=%.4f\n", ok ? "OK" : "SAVE FAILED", factor);
  return ok;
}