#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <time.h>
#include "config.h"
#include "ConfigManager.h"
#include "HX711Manager.h"
#include "PumpManager.h"
#include "IrrigationController.h"
#include "WebServerManager.h"
#include "LogManager.h"

ConfigManager configManager;
LogManager logManager;
HX711Manager hx711Manager;
PumpManager pumpManager;
IrrigationController irrigationController;
WebServerManager webServerManager;

uint32_t lastWifiAttemptMs = 0;

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  if (millis() - lastWifiAttemptMs < 15000UL && lastWifiAttemptMs != 0) return;
  lastWifiAttemptMs = millis();

  Serial.printf("Connecting to Wi-Fi SSID: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void handleWiFiStatus() {
  static wl_status_t previous = WL_NO_SHIELD;
  wl_status_t current = WiFi.status();
  if (current != previous) {
    previous = current;
    if (current == WL_CONNECTED) {
      logManager.add("WIFI", "Connected: " + WiFi.localIP().toString());
      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
      MDNS.begin("irrigation");
    } else {
      logManager.add("WIFI", "Disconnected");
    }
  }
  if (current != WL_CONNECTED) connectWiFi();
}

void setup() {
  // Force pump OFF as early as possible. This assumes default active-high relay.
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, DEFAULT_RELAY_ACTIVE_HIGH ? LOW : HIGH);

  Serial.begin(115200);
  delay(200);
  Serial.println("\nESP32 Weight Irrigation Controller v" FIRMWARE_VERSION);

  configManager.begin();
  logManager.begin();
  pumpManager.begin(&configManager, &logManager);
  hx711Manager.begin(&configManager);
  irrigationController.begin(&configManager, &hx711Manager, &pumpManager, &logManager);

  connectWiFi();
  webServerManager.begin(&configManager, &hx711Manager, &pumpManager, &irrigationController, &logManager);
}

void loop() {
  handleWiFiStatus();
  webServerManager.handle();
  hx711Manager.update();
  irrigationController.update();
  yield();
}
