#pragma once

#include <Arduino.h>

#define FIRMWARE_VERSION "1.0.0"
#define DEFAULT_DEVICE_NAME "ESP32 Weight Irrigation"

// Replace these with your Wi-Fi credentials before uploading.
#define WIFI_SSID "ARRIS-D83A_EXT"
#define WIFI_PASSWORD "7C4C85BE7CFD4A5D"

// Default GPIO mapping. Change here if your wiring differs.
#define HX711_DOUT_PIN 32
#define HX711_SCK_PIN  33

#define PUMP_RELAY_PIN 25
#define MANUAL_BUTTON_PIN 27
#define TANK_EMPTY_PIN 26
#define LEAK_SENSOR_PIN 14
#define STATUS_LED_PIN 2

// Safety defaults.
#define DEFAULT_RELAY_ACTIVE_HIGH true
#define DEFAULT_TRIGGER_WEIGHT_G 4300.0f
#define DEFAULT_STOP_WEIGHT_G 4800.0f
#define DEFAULT_FULLY_WET_WEIGHT_G 5000.0f
#define DEFAULT_DRYBACK_TRIGGER_PERCENT 10.0f
#define DEFAULT_DRYBACK_STOP_PERCENT 2.0f
#define DEFAULT_MAX_RUNTIME_SEC 60
#define DEFAULT_MIN_GAP_MIN 20
#define DEFAULT_STABLE_DURATION_SEC 30
#define DEFAULT_SAMPLE_INTERVAL_MS 500
#define DEFAULT_FILTER_SAMPLES 10
#define DEFAULT_TIME_WINDOW_ENABLED false
#define DEFAULT_ALLOW_WITHOUT_VALID_TIME true
#define DEFAULT_TANK_SENSOR_ENABLED false
#define DEFAULT_LEAK_SENSOR_ENABLED false
#define DEFAULT_STOP_ON_WIFI_LOSS false

// Input polarity defaults. Many float/leak switches are wired to GND with INPUT_PULLUP.
#define DEFAULT_TANK_EMPTY_ACTIVE_LOW true
#define DEFAULT_LEAK_ACTIVE_LOW true
#define DEFAULT_BUTTON_ACTIVE_LOW true

// Manual run safety cap.
#define MAX_MANUAL_RUNTIME_SEC 120
