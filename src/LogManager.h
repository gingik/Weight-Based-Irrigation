#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

struct LogEntry {
  uint32_t uptimeSec;
  String type;
  String message;
};

class LogManager {
public:
  static const uint8_t MAX_LOGS = 50;
  void begin();
  void add(const String &type, const String &message);
  void toJson(JsonArray arr) const;
  String latestMessage() const;
private:
  LogEntry entries[MAX_LOGS];
  uint8_t count = 0;
  uint8_t head = 0;
};
