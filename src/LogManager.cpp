#include "LogManager.h"

void LogManager::begin() {
  count = 0;
  head = 0;
  add("BOOT", "System boot");
}

void LogManager::add(const String &type, const String &message) {
  entries[head] = { millis() / 1000UL, type, message };
  Serial.printf("[%lus] %s - %s\n", entries[head].uptimeSec, type.c_str(), message.c_str());
  head = (head + 1) % MAX_LOGS;
  if (count < MAX_LOGS) count++;
}

void LogManager::toJson(JsonArray arr) const {
  for (uint8_t i = 0; i < count; i++) {
    uint8_t idx = (head + MAX_LOGS - count + i) % MAX_LOGS;
    JsonObject o = arr.add<JsonObject>();
    o["uptimeSec"] = entries[idx].uptimeSec;
    o["type"] = entries[idx].type;
    o["message"] = entries[idx].message;
  }
}

String LogManager::latestMessage() const {
  if (count == 0) return "";
  uint8_t idx = (head + MAX_LOGS - 1) % MAX_LOGS;
  return entries[idx].type + ": " + entries[idx].message;
}
