#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <time.h>
#include <RTClib.h>
#include <Wire.h>
#include "config.h"
#include "ConfigManager.h"
#include "HX711Manager.h"
#include "PumpManager.h"
#include "IrrigationController.h"
#include "WebServerManager.h"
#include "LogManager.h"
#include "WeightHistoryManager.h"

ConfigManager configManager;
LogManager logManager;
HX711Manager hx711Manager;
PumpManager pumpManager;
IrrigationController irrigationController;
WebServerManager webServerManager;
WeightHistoryManager weightHistoryManager;
RTC_DS3231 rtc;

uint32_t lastWifiAttemptMs = 0;
static bool ntpSynced = false;

void setupOTA();

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  if (millis() - lastWifiAttemptMs < 15000UL && lastWifiAttemptMs != 0) return;
  lastWifiAttemptMs = millis();

  const AppConfig &cfg = configManager.get();
  Serial.printf("Connecting to Wi-Fi SSID: %s\n", cfg.wifiSsid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg.wifiSsid, cfg.wifiPassword);
}

void handleWiFiStatus() {
  static wl_status_t previous = WL_NO_SHIELD;
  static bool otaStarted = false;
  wl_status_t current = WiFi.status();
  if (current != previous) {
    previous = current;
    if (current == WL_CONNECTED) {
      logManager.add("WIFI", "Connected: " + WiFi.localIP().toString());
      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
      MDNS.begin("irrigation");
      if (!otaStarted) {
        otaStarted = true;
        setupOTA();
      }
    } else {
      logManager.add("WIFI", "Disconnected");
    }
  }
  if (current != WL_CONNECTED) connectWiFi();

  // Once NTP syncs, set the RTC (one-shot)
  if (!ntpSynced && current == WL_CONNECTED) {
    time_t now;
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) {
      time(&now);
      rtc.adjust(DateTime(now));
      ntpSynced = true;
      Serial.printf("[RTC] Synced from NTP: %s", ctime(&now));
      logManager.add("RTC", "Synced from NTP");
    }
  }
}

void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  if (strlen(OTA_PASSWORD) > 0) {
    ArduinoOTA.setPassword(OTA_PASSWORD);
  }

  ArduinoOTA
    .onStart([]() {
      String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
      Serial.printf("OTA Start: %s\n", type.c_str());
      // Stop pump and irrigation during update
      pumpManager.setPump(false, "OTA update");
      irrigationController.setEmergencyStop(true);
    })
    .onEnd([]() {
      Serial.println("\nOTA End");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("OTA Progress: %u%%\r", (progress * 100) / total);
    })
    .onError([](ota_error_t error) {
      Serial.printf("OTA Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR)      Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR)    Serial.println("End Failed");
    });

  ArduinoOTA.begin();
  logManager.add("OTA", "ArduinoOTA ready, hostname: " + String(OTA_HOSTNAME));
}

void setup() {
  // Force pump OFF as early as possible. This assumes default active-high relay.
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, DEFAULT_RELAY_ACTIVE_HIGH ? HIGH : LOW);

  Serial.begin(115200);
  delay(200);
  Serial.println("\nESP32 Weight Irrigation Controller v" FIRMWARE_VERSION);

  // Init RTC
  Wire.begin(RTC_SDA_PIN, RTC_SCL_PIN);
  if (!rtc.begin()) {
    Serial.println("[RTC] DS3231 not detected! History will not be recorded.");
    logManager.add("RTC", "ERROR: DS3231 not found");
  } else {
    DateTime now = rtc.now();
    if (now.isValid() && now.unixtime() > 1000000000UL) {
      Serial.printf("[RTC] Current time: %04d-%02d-%02d %02d:%02d:%02d UTC\n",
        now.year(), now.month(), now.day(),
        now.hour(), now.minute(), now.second());
    } else {
      Serial.println("[RTC] Time not set — waiting for NTP sync");
    }
  }

  configManager.begin();
  logManager.begin();
  pumpManager.begin(&configManager, &logManager);
  hx711Manager.begin(&configManager);
  irrigationController.begin(&configManager, &hx711Manager, &pumpManager, &logManager);
  weightHistoryManager.begin(&rtc);

  connectWiFi();
  webServerManager.begin(&configManager, &hx711Manager, &pumpManager, &irrigationController, &logManager, &weightHistoryManager);
}

void loop() {
  ArduinoOTA.handle();
  handleWiFiStatus();
  webServerManager.handle();
  hx711Manager.update();
  weightHistoryManager.update(hx711Manager.weightG());
  irrigationController.update();
  yield();
}
