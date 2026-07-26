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
  _emaAlpha = 2.0f / (constrain(c.filterSamples, (uint8_t)1, (uint8_t)50) + 1);
  _ema = 0.0f;
  filtered = 0.0f;
  firstSample = true;
  Serial.printf("[HX711] Config applied — tare: %ld, cal: %.4f, samples: %d, interval: %lums\n",
    c.tareOffset, c.calibrationFactor, c.filterSamples, c.sampleIntervalMs);
}

void HX711Manager::update() {
  const AppConfig &c = config->get();

  if (millis() - lastSampleMs < c.sampleIntervalMs) return;
  lastSampleMs = millis();

  // Give HX711 a startup warmup period (resets on begin, not wall-clock)
  if (millis() - _startupMs < 5000 && !scale.is_ready()) {
    valid = false;
    stable = false;
    err = "HX711 warming up";
    return;
  }

  if (!scale.is_ready()) {
    valid = false;
    stable = false;
    err = "HX711 not ready";
    _errorCount++;
    if (!_persistentFault && _errorCount >= 5) reinit();
    return;
  }

  long r = scale.read();
  float g = (r - c.tareOffset) / c.calibrationFactor;
  long previousRaw = lastRaw;
  lastRaw = r;

  if (isnan(g) || isinf(g) || abs(r) < 10) {
    if (_errorCount % 10 == 0)
      Serial.printf("[HX711] ERROR: invalid reading — raw=%ld g=%.2f\n", r, g);
    valid = false;
    stable = false;
    err = "Invalid scale reading";
    _errorCount++;
    if (!_persistentFault && _errorCount >= 5) reinit();
    return;
  }

  if (!firstSample && abs(r - previousRaw) > 500000L) {
    Serial.printf("[HX711] ERROR: unrealistic jump — prev=%ld cur=%ld delta=%ld\n",
      previousRaw, r, abs(r - previousRaw));
    valid = false;
    stable = false;
    err = "Unrealistic scale jump";
    _errorCount++;
    if (_errorCount >= 5) reinit();
    return;
  }

  valid = true;
  err = "";
  _errorCount = 0;

  if (firstSample) {
    _ema = g;
    firstSample = false;
  } else {
    _ema = _emaAlpha * g + (1.0f - _emaAlpha) * _ema;
  }
  float newFiltered = _ema;
  float previousFiltered = filtered;

  bool wasStable = stable;
  if (abs(newFiltered - filtered) < 5.0f) {
    if (stableSinceMs == 0) stableSinceMs = millis();
  } else {
    stableSinceMs = 0;
  }
  filtered = newFiltered;
  stable = stableSinceMs != 0 && (millis() - stableSinceMs) >= 3000UL;

  // Log every sample at DEBUG verbosity — gate behind a compile-time flag
  // to avoid flooding serial at normal operation
#ifdef HX711_DEBUG_VERBOSE
  Serial.printf("[HX711] raw=%ld  g=%.2f  filt=%.2f  stable=%d\n",
    r, g, filtered, stable);
#endif

  // Log stability transitions
  float delta = abs(newFiltered - previousFiltered);
  if (!wasStable && stable)
    Serial.printf("[HX711] STABLE — filtered=%.2fg\n", filtered);
  else if (wasStable && !stable)
    Serial.printf("[HX711] UNSTABLE — delta=%.2fg from previous\n", delta);
}

void HX711Manager::reinit() {
  uint32_t now = millis();
  // Cooldown: at least 30 s between reinit attempts
  if (now - _lastReinitMs < 30000UL) return;
  _lastReinitMs = now;

  if (_persistentFault) return;

  Serial.println("[HX711] Re-initializing after consecutive errors...");
  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
  uint32_t t = millis();
  while (!scale.is_ready() && millis() - t < 2000) delay(10);
  if (scale.is_ready()) {
    // If is_ready() returns instantly (0ms) and error count is high,
    // DOUT is likely floating — mark as persistent fault.
    if ((millis() - t) < 5 && _errorCount >= 20) {
      _persistentFault = true;
      Serial.println("[HX711] HARDWARE FAULT — DOUT pin stuck high; check wiring/power");
      return;
    }
    Serial.printf("[HX711] Re-initialized OK after %lums\n", millis() - t);
    scale.read();
    applyConfig();
    _errorCount = 0;
    firstSample = true;
  } else {
    Serial.println("[HX711] Re-initialize FAILED — HX711 still unresponsive");
    _persistentFault = true;
  }
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
  _ema = 0.0f;
  firstSample = true;
  stableSinceMs = 0;
  filtered = 0.0f;
  lastRaw = newOffset;
  valid = true;
  stable = false;
  err = "";

  // Wait for the next conversion to finish — prevents a spurious
  // "HX711 not ready" -> SENSOR_ERROR transition on the very next update().
  uint32_t t = millis();
  while (!scale.is_ready() && millis() - t < 100) delay(1);

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
  if (abs(net) < 100) {
    Serial.printf("[HX711] Calibrate FAILED: raw delta (%ld) is too small — "
      "load cell not responding. Check wiring (red=E+, black=E-, white=A-, green=A+).\n", net);
    return false;
  }
  float factor = (float)net / knownWeightG;
  config->get().calibrationFactor = factor;
  scale.set_scale(factor);

  // Reset filtered ring buffer so stale readings don't mix with new scale
  _ema = 0.0f;
  firstSample = true;
  stableSinceMs = 0;
  filtered = 0.0f;
  valid = true;
  stable = false;
  err = "";

  // Wait for the next conversion to finish — prevents spurious SENSOR_ERROR.
  uint32_t t = millis();
  while (!scale.is_ready() && millis() - t < 100) delay(1);

  bool ok = config->save();
  Serial.printf("[HX711] Calibrate %s — factor=%.4f\n", ok ? "OK" : "SAVE FAILED", factor);
  return ok;
}