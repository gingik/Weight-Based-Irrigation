# ESP32 Weight-Based Irrigation Controller

An ESP32 irrigation controller that uses a load cell and HX711 amplifier to trigger watering from pot weight. The ESP32 hosts a local web dashboard where you can calibrate the scale, set irrigation thresholds, manually run the pump, and monitor safety states.

This project is designed for indoor growing systems where pot weight is used to estimate water loss and dry-back. It is built to keep the irrigation decision local to the ESP32, so watering logic can continue without Node-RED, MQTT, cloud services, or an internet connection.

> **Version:** V1 prototype firmware  
> **Board:** ESP32-WROOM DevKit  
> **Framework:** Arduino for ESP32  
> **Build system:** PlatformIO

---

## Features

- HX711 load-cell reading with EMA filtering
- Weight calibration with tare and known weight
- Local ESP32 web dashboard
- Absolute weight trigger mode
- Dry-back percentage trigger mode
- Pump relay control
- Pump OFF on boot
- Max pump runtime safety
- Minimum gap between irrigations
- Tank-empty safety input
- Leak sensor safety input
- Manual pump ON/OFF/timed run
- Emergency stop
- Settings stored in ESP32 Preferences/NVS
- Basic event logs
- REST API for status, settings, calibration, pump control, logs, and history
- Non-blocking `millis()`-based control loop
- Over-the-air (OTA) firmware updates — web upload and ArduinoOTA
- Weight history logging — 7 days at 1-minute intervals, persisted to LittleFS
- DS3231 RTC for accurate timestamps with NTP sync
- Configurable irrigation time window (e.g. only water between certain hours)
- WiFi AP mode fallback — auto-starts access point if no network configured
- mDNS hostname (`irrigation.local`)
- Status LED indicator
- Physical manual button input

---

## Hardware

### Required

| Part | Purpose |
|---|---|
| ESP32 DevKit | Main controller and dashboard host |
| HX711 module | Load-cell amplifier |
| Load cell / platform scale | Pot/container weight measurement |
| Relay module or MOSFET driver | Pump or solenoid switching |
| Pump or solenoid valve | Irrigation output |
| DS3231 RTC module | Real-time clock for weight history timestamps |
| Stable ESP32 power supply | Logic power |
| Pump power supply | Pump/valve power |

### Strongly Recommended Safety Hardware

| Part | Purpose |
|---|---|
| Tank-empty float switch | Prevents pump running dry |
| Leak sensor | Stops pump if water is detected where it should not be |
| Fuse | Electrical protection |
| Manual power cutoff | Physical emergency shutdown |
| Normally-open relay wiring | Keeps pump OFF unless actively enabled |

---

## Default Pinout

The GPIO pins are configurable in `src/config.h`, but the default mapping is:

| Function | ESP32 GPIO |
|---|---:|
| HX711 DOUT / DT | GPIO 32 |
| HX711 SCK / CLK | GPIO 33 |
| Pump relay | GPIO 25 |
| Manual button | GPIO 27 |
| Tank-empty sensor | GPIO 26 |
| Leak sensor | GPIO 14 |
| Status LED | GPIO 2 |

### HX711 Wiring


HX711 DT / DOUT  -> ESP32 GPIO 32
HX711 SCK / CLK  -> ESP32 GPIO 33
HX711 VCC        -> 3.3V or 5V, depending on your HX711 module
HX711 GND        -> ESP32 GND


### Relay / Pump Wiring Notes

Use a relay or driver suitable for your pump or solenoid current. For safety, wire the pump through the **normally-open** relay contact so the pump remains OFF if the ESP32 loses power or crashes.

Avoid relay control on ESP32 boot-strapping pins:

GPIO 0
GPIO 2
GPIO 12
GPIO 15


GPIO 2 is acceptable for the onboard/status LED, but should not be used for pump control.

### Status LED Behavior

The onboard LED on GPIO 2 indicates system status:

| Behavior | Meaning |
|---|---|
| Solid on | System running normally (IDLE, IRRIGATING, COOLDOWN) |
| Slow blink (~1 Hz) | WiFi disconnected, attempting to reconnect |
| Fast blink (~4 Hz) | Emergency stop or error state |
| Off | BOOTING or pump actively running |

### Manual Button

A physical push button (default GPIO 27, active-low with internal pull-up) provides:

- **Short press** — Toggle pump ON/OFF manually
- **Single press while irrigating** — Stop irrigation
- **Single press while in emergency stop** — Clear error and release emergency stop

---

## Software Requirements

Install:

- [Visual Studio Code](https://code.visualstudio.com/)
- [PlatformIO](https://platformio.org/)

The project uses Arduino framework for ESP32.

---

## Libraries / Dependencies

All dependencies are resolved automatically by PlatformIO:

| Library | Version | Purpose |
|---|---|---|
| `bogde/HX711` | ^0.7.5 | HX711 load-cell amplifier driver |
| `bblanchon/ArduinoJson` | ^7.0.4 | JSON serialization for REST API |
| `adafruit/RTClib` | ^2.1.4 | DS3231 real-time clock driver |

## Build Configuration

The project uses a custom partition table (`partitions_custom.csv`) with OTA support:

| Partition | Size | Purpose |
|---|---|---|
| `app0` | 1280 KB | Factory / active firmware slot |
| `app1` | 1280 KB | OTA update slot |
| `spiffs` | 1472 KB | LittleFS filesystem (weight history, web assets) |

## Setup

### Option A: Configure WiFi in source (recommended for first flash)

1. Clone the repository:
   ```
   git clone https://github.com/YOUR_USERNAME/esp32-weight-irrigation.git
   cd esp32-weight-irrigation
   ```

2. Open the folder in VS Code with PlatformIO.

3. Edit Wi-Fi credentials in `src/config.h`:
   ```cpp
   #define WIFI_SSID "YOUR_WIFI_SSID"
   #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
   ```

   Leave them empty (`""`) to use AP mode instead.

4. Connect your ESP32 by USB.

5. Build and upload:
   ```
   pio run -t upload
   ```

6. Open the serial monitor to find the IP address:
   ```
   pio device monitor
   ```

7. Open the dashboard:
   ```
   http://<esp32-ip>/
   ```
   Or via mDNS:
   ```
   http://irrigation.local/
   ```

### Option B: Configure WiFi from the browser (AP mode)

If you leave `WIFI_SSID` empty in `config.h`, the ESP32 starts in **AP mode** on first boot:

1. Connect your phone or laptop to the WiFi network **`ESP32-Irrigation`**.
2. Open `http://192.168.4.1/` and navigate to `/wifi`.
3. Scan for networks, select yours, enter the password, and click Connect.
4. The ESP32 reboots and connects to your network.


---

## Dashboard

The ESP32 serves a lightweight local web dashboard.

The dashboard shows:

- Current weight in grams
- Raw HX711 reading
- Sensor stable/valid status
- Pump ON/OFF state
- Current system state
- Trigger weight
- Stop weight
- Tank-empty status
- Leak sensor status
- Wi-Fi status
- Uptime
- Recent logs

The dashboard also allows:

- Tare / zero scale
- Calibrate with known weight
- Change trigger mode
- Set absolute thresholds
- Set dry-back thresholds
- Change max pump runtime
- Change minimum irrigation gap
- Enable/disable tank and leak sensors
- Set relay active mode
- Manually run the pump
- Trigger emergency stop
- Clear errors

---

## Calibration

Calibrate before connecting the pump to water.

Recommended process:

1. Assemble the load cell and platform.
2. Make sure the platform is empty.
3. Open the dashboard.
4. Click **Tare / Zero**.
5. Place a known weight on the platform.
6. Enter the known weight in grams.
7. Click **Calibrate Known Weight**.
8. Confirm the displayed weight is close to the known weight.
9. Repeat if needed.

For best results:

- Keep load-cell wiring short.
- Keep pump wiring away from HX711 wiring.
- Use stable power.
- Mechanically isolate the scale from vibration.
- Do not trigger irrigation from a single noisy reading.

---

## Irrigation Modes

### Irrigation Time Window

The firmware supports restricting irrigation to a specific time window (e.g. daytime only). Configured via the REST API or dashboard:

- **`timeWindowEnabled`** — Enable/disable the time restriction.
- **`windowStartMin`** — Start time as minutes after midnight (e.g. `480` = 08:00).
- **`windowEndMin`** — End time as minutes after midnight (e.g. `1320` = 22:00).
- **`allowIrrigationWithoutValidTime`** — If true, allows irrigation when the RTC time is not yet set (e.g. before NTP sync).

### 1. Absolute Weight Mode

The pump starts when:


weight <= triggerWeightG


The pump stops when:


weight >= stopWeightG


Example:

Start watering below: 4300 g
Stop watering at:     4800 g


### 2. Dry-Back Percentage Mode

The firmware calculates thresholds from the fully wet pot weight.


triggerWeight = fullyWetWeight * (1 - triggerDryBackPercent / 100)
stopWeight    = fullyWetWeight * (1 - stopDryBackPercent / 100)


Example:


Fully wet weight:       5000 g
Trigger dry-back:       10%
Stop dry-back:          2%

Calculated trigger:     4500 g
Calculated stop:        4900 g


This mode is useful when you want the system to irrigate based on percentage dry-back instead of fixed grams.

---

## Safety Logic

The pump can start only when all required conditions are true:

- Current weight is below the trigger threshold
- Weight reading is valid
- Weight reading is stable
- Pump is not already running
- Minimum gap since last irrigation has passed
- Tank-empty sensor is not active
- Leak sensor is not active
- Emergency stop is not active
- System is not in calibration or error state

The pump stops immediately if any of these happen:

- Stop weight is reached
- Max pump runtime is reached
- Tank-empty sensor becomes active
- Leak sensor becomes active
- HX711 reading becomes invalid
- Manual stop is requested
- Emergency stop is triggered
- Manual timed run expires
- Wi-Fi is lost, if that option is enabled

The pump is forced OFF during boot.

---

## System States

The firmware uses a state machine with these states:

BOOTING
IDLE
WAITING_FOR_STABLE_READING
BELOW_THRESHOLD
IRRIGATING
COOLDOWN
CALIBRATION
SENSOR_ERROR
TANK_EMPTY
LEAK_DETECTED
CONFIG_ERROR
MANUAL_MODE
EMERGENCY_STOP


These states are shown in the dashboard and API responses.

---

## REST API

### Status

http
GET /api/status


Returns current device status, weight, pump state, thresholds, safety inputs, Wi-Fi status, and uptime.

### Settings

http
GET /api/settings
POST /api/settings


Used by the dashboard to read and update configuration.

### Calibration

http
POST /api/calibration/tare
POST /api/calibration/known-weight
POST /api/calibration/reset

Example known-weight calibration request:

json
{
  "knownWeightG": 1000
}


### Pump Control

http
POST /api/pump/on
POST /api/pump/off
POST /api/pump/run
POST /api/emergency-stop
POST /api/clear-error


Example timed run:

json
{
  "seconds": 10
}


### Logs

```http
GET /api/logs
```

Returns recent events such as boot, settings changes, calibration changes, irrigation start/stop, safety stops, and errors.

### Weight History

```http
GET /api/weight-history?range=<hours>
```

Returns logged weight data points. The optional `range` query parameter accepts 1–168 hours (default 24).

### WiFi

```http
GET  /api/wifi/scan
POST /api/wifi/connect
GET  /api/wifi/status
```

`/api/wifi/scan` scans for nearby networks and returns SSID, RSSI, and encryption type.

`/api/wifi/connect` connects to a network:

```json
{
  "ssid": "MyNetwork",
  "password": "secret123"
}
```

`/api/wifi/status` returns the current connection state, SSID, and IP address.

### Firmware Update (OTA)

```http
POST /api/firmware/update
```

Upload a `.bin` firmware file via `multipart/form-data`. On success the ESP32 saves settings, flushes history, and reboots automatically.

### ArduinoOTA

The ESP32 also exposes an **ArduinoOTA** interface on hostname `wb-irrigation`. Supported in PlatformIO and Arduino IDE for wireless firmware upload without a USB cable.

### Page Routes

| Route | Description |
|---|---|
| `/` | Main dashboard |
| `/wifi` | WiFi network setup page |
| `/firmware` | Firmware upload page |

---

## Project Structure


.
├── platformio.ini              # PlatformIO build configuration
├── partitions_custom.csv       # Custom flash partition table (OTA-capable)
├── README.md
└── src
    ├── main.cpp
    ├── config.h
    ├── ConfigManager.h
    ├── ConfigManager.cpp
    ├── HX711Manager.h
    ├── HX711Manager.cpp
    ├── PumpManager.h
    ├── PumpManager.cpp
    ├── IrrigationController.h
    ├── IrrigationController.cpp
    ├── WebServerManager.h
    ├── WebServerManager.cpp
    ├── LogManager.h
    ├── LogManager.cpp
    ├── WeightHistoryManager.h
    └── WeightHistoryManager.cpp

### Module Overview

| Module | Responsibility |
|---|---|
| `main.cpp` | Boot, WiFi, OTA, service initialization, main loop |
| `config.h` | Compile-time configuration (pins, defaults, credentials) |
| `ConfigManager` | Load/save settings from Preferences/NVS |
| `HX711Manager` | Read, filter (EMA), tare, and calibrate the scale |
| `PumpManager` | Safe relay control |
| `IrrigationController` | State machine and irrigation decisions |
| `WebServerManager` | Dashboard, REST API, WiFi setup, firmware upload |
| `LogManager` | In-memory event logs (last 50 entries) |
| `WeightHistoryManager` | Ring-buffer weight logging (7 days, persisted to LittleFS) |

---

## First-Test Checklist

Before connecting the real pump:

- [ ] Upload firmware successfully
- [ ] Confirm ESP32 connects to Wi-Fi
- [ ] Open dashboard in browser
- [ ] Confirm pump relay is OFF after boot
- [ ] Confirm HX711 raw reading changes when weight changes
- [ ] Tare the scale
- [ ] Calibrate with a known weight
- [ ] Set a short max runtime, for example 5 seconds
- [ ] Test manual pump command with pump power disconnected
- [ ] Test emergency stop
- [ ] Test tank-empty input
- [ ] Test leak input
- [ ] Trigger irrigation using test thresholds
- [ ] Confirm relay stops at timeout
- [ ] Confirm settings persist after reboot

---

## Version 1 Scope

Included in V1:

- HX711 reading with EMA filtering
- Weight calibration (tare + known weight)
- Local web dashboard
- Absolute threshold mode
- Dry-back percentage mode
- Irrigation time window
- Pump relay control
- Max runtime safety
- Minimum gap safety
- Manual pump control (dashboard + physical button)
- Emergency stop
- Settings persistence (NVS)
- Event logs
- Weight history (7 days, persisted to LittleFS)
- DS3231 RTC with NTP sync
- OTA firmware updates (web upload + ArduinoOTA)
- WiFi AP mode fallback with browser-based setup
- mDNS (`irrigation.local`)
- Sensor error handling
- Status LED indication

Not included in V1:

- MQTT
- Home Assistant discovery
- OLED display
- Flow meter verification
- Cloud dashboard
- Mobile app
- Multi-zone irrigation

---

## Roadmap

Possible future improvements:

- MQTT integration
- Node-RED integration
- InfluxDB logging
- Home Assistant discovery
- Flow meter verification
- Pump failure detection
- Multi-zone irrigation
- Multi-load-cell support
- OLED display
- Weight / history graphing on dashboard
- Auto dry-back learning
- Daily irrigation volume limits
- EC/pH integration
- VPD-aware irrigation suggestions
- ESP-NOW remote sensors

---

## Safety Warning

This project controls water and may control mains-powered devices depending on your pump setup. Test carefully before connecting a real pump.

Use physical safety measures:

- Normally-open relay wiring
- Fuse
- Manual power cutoff
- Leak sensor
- Tank-empty float switch
- Max runtime limit
- Drip tray or containment

Software safety is not a replacement for physical flood and electrical protection.

The safest failure state must always be:

Pump OFF


## License

MIT License
---

## Disclaimer

Use this firmware at your own risk. Verify all electrical wiring, relay ratings, pump current, sensor behaviour, and safety limits before unattended operation.
