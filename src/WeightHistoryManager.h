#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <RTClib.h>
#include <FS.h>
#include <LittleFS.h>

// 7 days at 1-minute intervals = 10080 points (~79 KB RAM, ~81 KB flash)
#define MAX_HISTORY_POINTS 10080
#define HISTORY_INTERVAL_MS 60000UL        // 1 minute sample
#define HISTORY_FLUSH_INTERVAL_MS 900000UL // 15 minute flash save
#define HISTORY_FILE "/weight_history.bin"

struct WeightPoint {
  uint32_t epoch;    // Unix timestamp (UTC)
  float weightG;
};

class WeightHistoryManager {
public:
  void begin(RTC_DS3231 *rtc);
  void update(float currentWeightG);
  void toJson(JsonArray arr, uint32_t sinceEpoch = 0, uint16_t maxPoints = 0) const;
  uint16_t count() const { return _count; }
  uint32_t currentEpoch() const;

private:
  WeightPoint *_points = nullptr;  // heap-allocated, ~81 KB
  uint16_t _count = 0;
  uint16_t _head = 0;
  uint32_t _lastSampleMs = 0;
  uint32_t _lastFlushMs = 0;
  RTC_DS3231 *_rtc = nullptr;

  void loadFromFlash();
  void flushToFlash();
};
