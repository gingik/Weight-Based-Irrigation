#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"
#include "HX711Manager.h"
#include "PumpManager.h"
#include "IrrigationController.h"
#include "LogManager.h"

class WebServerManager {
public:
  void begin(ConfigManager *config, HX711Manager *hx, PumpManager *pump, IrrigationController *controller, LogManager *log);
  void handle();
private:
  WebServer server{80};
  ConfigManager *config = nullptr;
  HX711Manager *hx = nullptr;
  PumpManager *pump = nullptr;
  IrrigationController *controller = nullptr;
  LogManager *log = nullptr;

  void setupRoutes();
  void sendJsonStatus();
  void sendJsonSettings();
  void handleSaveSettings();
  void handleTare();
  void handleKnownWeight();
  void handleResetCalibration();
  void handlePumpOn();
  void handlePumpRun();
  void handlePumpOff();
  void handleEmergencyStop();
  void handleClearError();
  void handleLogs();
  void handleRoot();
  bool argBool(const String &name, bool fallback);
  uint16_t parseMinutes(const String &hhmm, uint16_t fallback);
};
