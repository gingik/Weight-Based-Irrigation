# ESP32 Weight-Based Irrigation Controller

An ESP32 irrigation controller that uses a load cell and HX711 amplifier to trigger watering from pot weight. The ESP32 hosts a local web dashboard where you can calibrate the scale, set irrigation thresholds, manually run the pump, and monitor safety states.

This project is designed for indoor growing systems where pot weight is used to estimate water loss and dry-back. It is built to keep the irrigation decision local to the ESP32, so watering logic can continue without Node-RED, MQTT, cloud services, or an internet connection.

> **Version:** V1 prototype firmware  
> **Board:** ESP32-WROOM DevKit  
> **Framework:** Arduino for ESP32  
> **Build system:** PlatformIO

---

## Features

- HX711 load-cell reading
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
- REST API for status, settings, calibration, pump control, and logs
- Non-blocking `millis()`-based control loop

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

---

## Software Requirements

Install:

- [Visual Studio Code](https://code.visualstudio.com/)
- [PlatformIO](https://platformio.org/)

The project uses Arduino framework for ESP32.

---

## Setup

1. Clone the repository:

git clone https://github.com/YOUR_USERNAME/esp32-weight-irrigation.git
cd esp32-weight-irrigation


2. Open the folder in VS Code with PlatformIO.

3. Edit Wi-Fi credentials in:

src/config.h

Set:

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"


4. Connect your ESP32 by USB.

5. Build the firmware:


pio run


6. Upload to the ESP32:

pio run -t upload


7. Open the serial monitor:

pio device monitor


8. Look for the ESP32 IP address in the serial output.

9. Open the dashboard in your browser:

http://<esp32-ip>/


Optional mDNS address, if supported on your network:


http://irrigation.local/


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

GET /api/logs

Returns recent events such as boot, settings changes, calibration changes, irrigation start/stop, safety stops, and errors.

---

## Project Structure


.
├── platformio.ini
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
    └── LogManager.cpp

### Module Overview

| Module | Responsibility |
|---|---|
| `main.cpp` | Boot, Wi-Fi, service initialization, main loop |
| `ConfigManager` | Load/save settings from Preferences/NVS |
| `HX711Manager` | Read, filter, tare, and calibrate the scale |
| `PumpManager` | Safe relay control |
| `IrrigationController` | State machine and irrigation decisions |
| `WebServerManager` | Dashboard and REST API |
| `LogManager` | In-memory event logs |

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

- HX711 reading
- Weight calibration
- Local dashboard
- Absolute threshold mode
- Dry-back percentage mode
- Pump relay control
- Max runtime safety
- Minimum gap safety
- Manual pump control
- Emergency stop
- Settings persistence
- Basic logs
- Sensor error handling

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
- OTA firmware updates
- Wi-Fi captive portal setup
- Graphing weight over time
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
