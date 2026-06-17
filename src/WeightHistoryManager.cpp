#include "WeightHistoryManager.h"

void WeightHistoryManager::begin(RTC_DS3231 *rtc) {
  _rtc = rtc;
  _count = 0;
  _head = 0;
  _lastSampleMs = millis();
  _lastFlushMs = millis();

  _points = (WeightPoint*)malloc(sizeof(WeightPoint) * MAX_HISTORY_POINTS);
  if (!_points) {
    Serial.println("[History] FATAL: malloc failed for history buffer");
    return;
  }
  Serial.printf("[History] Allocated %u bytes on heap for %u points\n",
    sizeof(WeightPoint) * MAX_HISTORY_POINTS, MAX_HISTORY_POINTS);

  if (!LittleFS.begin(true)) {
    Serial.println("[History] LittleFS mount failed — no flash persistence");
  } else {
    Serial.printf("[History] LittleFS: %u / %u bytes used\n",
      LittleFS.usedBytes(), LittleFS.totalBytes());
    loadFromFlash();
  }

  Serial.printf("[History] Initialized — 7d max, 1-min intervals, %u points loaded\n", _count);
}

void WeightHistoryManager::update(float currentWeightG) {
  uint32_t now = millis();

  if (now - _lastSampleMs < HISTORY_INTERVAL_MS) return;
  _lastSampleMs = now;

  // Get epoch from RTC. If RTC lost power (epoch < 1000000000 = Sep 2001), skip.
  uint32_t epoch = currentEpoch();
  if (epoch < 1000000000UL) {
    Serial.println("[History] RTC time invalid, skipping sample");
    return;
  }

  WeightPoint pt;
  pt.epoch = epoch;
  pt.weightG = currentWeightG;

  _points[_head] = pt;
  _head = (_head + 1) % MAX_HISTORY_POINTS;
  if (_count < MAX_HISTORY_POINTS) _count++;

#ifdef HISTORY_DEBUG
  Serial.printf("[History] Recorded #%u: epoch=%u weight=%.1fg\n",
    _count, pt.epoch, pt.weightG);
#endif

  // Flush to flash every 15 min
  if (now - _lastFlushMs >= HISTORY_FLUSH_INTERVAL_MS) {
    _lastFlushMs = now;
    flushToFlash();
  }
}

uint32_t WeightHistoryManager::currentEpoch() const {
  if (!_rtc) return 0;
  DateTime now = _rtc->now();
  return (now.isValid()) ? now.unixtime() : 0;
}

void WeightHistoryManager::toJson(JsonArray arr, uint32_t sinceEpoch, uint16_t maxPoints) const {
  // First pass: count matching points
  uint16_t matchCount = 0;
  for (uint16_t i = 0; i < _count; i++) {
    uint16_t idx = (_head + MAX_HISTORY_POINTS - _count + i) % MAX_HISTORY_POINTS;
    if (sinceEpoch == 0 || _points[idx].epoch >= sinceEpoch) matchCount++;
  }

  // Calculate step size for downsampling
  uint16_t step = 1;
  if (maxPoints > 0 && matchCount > maxPoints) {
    step = (matchCount + maxPoints - 1) / maxPoints;
    if (step < 2) step = 2;
  }

  uint16_t matched = 0, emitted = 0;
  uint16_t n = _count;
  for (uint16_t i = 0; i < n; i++) {
    uint16_t idx = (_head + MAX_HISTORY_POINTS - n + i) % MAX_HISTORY_POINTS;
    const WeightPoint &pt = _points[idx];
    if (sinceEpoch > 0 && pt.epoch < sinceEpoch) continue;

    bool isLast = (matched == matchCount - 1);
    bool shouldEmit = (emitted == 0) || isLast || (matched % step == 0);
    if (shouldEmit) {
      JsonObject obj = arr.add<JsonObject>();
      obj["epoch"] = pt.epoch;
      obj["weightG"] = pt.weightG;
      emitted++;
    }
    matched++;
  }
}

// --- LittleFS persistence ---

void WeightHistoryManager::loadFromFlash() {
  File f = LittleFS.open(HISTORY_FILE, "r");
  if (!f) {
    Serial.println("[History] No saved history file found");
    return;
  }

  size_t fileSize = f.size();
  // File format: 2-byte count (uint16 LE) + count * 8 bytes WeightPoint
  if (fileSize < 2) { f.close(); return; }

  uint8_t countBuf[2];
  if (f.read(countBuf, 2) != 2) { f.close(); return; }
  uint16_t flashCount = countBuf[0] | (countBuf[1] << 8);

  if (flashCount == 0 || flashCount > MAX_HISTORY_POINTS) { f.close(); return; }

  size_t expectedSize = 2 + (size_t)flashCount * 8;
  if (fileSize < expectedSize) { f.close(); return; }

  uint16_t loaded = 0;
  while (loaded < flashCount && f.available() >= 8) {
    WeightPoint pt;
    uint8_t buf[8];
    if (f.read(buf, 8) != 8) break;

    pt.epoch    = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8)
               | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
    uint32_t wg;
    memcpy(&wg, buf + 4, 4);
    pt.weightG  = *((float*)&wg);

    // Only load points with valid epoch
    if (pt.epoch >= 1000000000UL) {
      _points[_head] = pt;
      _head = (_head + 1) % MAX_HISTORY_POINTS;
      if (_count < MAX_HISTORY_POINTS) _count++;
    }
    loaded++;
  }
  f.close();
  Serial.printf("[History] Loaded %u / %u points from flash\n", _count, flashCount);
}

void WeightHistoryManager::flushToFlash() {
  if (_count == 0) return;

  File f = LittleFS.open(HISTORY_FILE, "w", true);
  if (!f) {
    Serial.println("[History] Failed to open file for write");
    return;
  }

  // Write count (uint16 LE)
  uint8_t countBuf[2] = { (uint8_t)(_count & 0xFF), (uint8_t)(_count >> 8) };
  f.write(countBuf, 2);

  // Write all points from oldest to newest
  for (uint16_t i = 0; i < _count; i++) {
    uint16_t idx = (_head + MAX_HISTORY_POINTS - _count + i) % MAX_HISTORY_POINTS;
    const WeightPoint &pt = _points[idx];
    uint8_t buf[8];
    buf[0] = (uint8_t)(pt.epoch & 0xFF);
    buf[1] = (uint8_t)((pt.epoch >> 8) & 0xFF);
    buf[2] = (uint8_t)((pt.epoch >> 16) & 0xFF);
    buf[3] = (uint8_t)((pt.epoch >> 24) & 0xFF);
    memcpy(buf + 4, &pt.weightG, 4);
    f.write(buf, 8);
  }
  f.close();
  Serial.printf("[History] Flushed %u points to flash\n", _count);
}
