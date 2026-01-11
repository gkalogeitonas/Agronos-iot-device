# Agronos WiFi Sensor - AI Agent Instructions

> **Important**: If you make changes that alter any architecture patterns, workflows, or conventions documented here, prompt the user to review and update this file to keep it current.

## Project Overview
ESP32 firmware that reads sensors and publishes data via MQTT (primary) or HTTP (fallback). Built with PlatformIO + Arduino framework.

## Architecture Patterns

### Sensor Registration System
- **Self-registering factory pattern**: Each sensor implementation registers itself using `registerSensorFactory()` with a static initializer
- Example from [dht11_temp.cpp](../src/dht11_temp.cpp#L24):
  ```cpp
  static bool _reg = registerSensorFactory("DHT11TemperatureReader", create_sensor_impl<DHT11TemperatureReader>);
  ```
- **Key insight**: Factory code (`sensor_factory.cpp`) never needs modification when adding sensors
- All sensors derive from `SensorBase` interface with `read(float &out)` method
- Ownership: Factory returns `std::vector<std::unique_ptr<SensorBase>>` for RAII memory management

### Shared DHT Instances
- **Critical**: Multiple sensors (temp/humidity) on the same GPIO pin MUST share a single DHT object
- Use `getSharedDht(pin)` from [dht_shared.h](../include/dht_shared.h) - never create DHT instances directly
- Pattern prevents read conflicts and `NaN` values on multi-value sensors

### Smart Data Routing
- [data_sender.cpp](../src/data_sender.cpp): MQTT-first with automatic HTTP fallback
- Flow: Try MQTT → on failure fallback to HTTP → both use same JSON payload structure
- Single payload format ensures protocol transparency to sensor layer
- MQTT topics follow `devices/{device-uuid}/sensors` pattern for ACL compliance

### Persistent Storage (NVS)
- [storage.cpp](../src/storage.cpp) wraps ESP32 Preferences API with namespaced keys
- Three namespaces: `wifi`, `auth`, `mqtt` - keep separation when adding new storage
- Always check if values changed before writing to minimize NVS wear

### Captive Portal Provisioning
- [wifi_portal.cpp](../src/wifi_portal.cpp): DNS + HTTP server for WiFi credential capture
- **Trigger logic** in [main.cpp](../src/main.cpp#L52-L67): Portal starts if no saved credentials OR connection fails
- **DNS hijacking**: `dnsServer` redirects ALL DNS queries to ESP32 IP (192.168.4.1)
- **Multi-platform detection**: Handles Android `/generate_204`, iOS `/hotspot-detect.html`, Windows `/ncsi.txt`
- **Form submission**: POST to `/save` → credentials stored → automatic ESP.restart()
- **Auto-stop**: Portal halts when WiFi connects (checked in `handle()` loop)
- Configuration: `AP_SSID` and `AP_PASS` in [config.h](../include/config.h.example)

### Button Reset Functionality
- **Hardware**: Physical button connected to GPIO 14 (configurable in `config.h`)
- **Long Press Reset**: Holding button for 10+ seconds (defined by `BUTTON_LONG_PRESS_MS`) during boot wipes all storage:
  - WiFi credentials
  - JWT authentication token
  - MQTT credentials
- **Deep Sleep Wake**: Button press wakes device from deep sleep (configured via `esp_sleep_enable_ext0_wakeup()`)
- **Implementation**: `checkButtonReset()` in [main.cpp](../src/main.cpp) runs early in `setup()` before WiFi connection
- Pattern: Provides factory reset capability without reflashing firmware

### Authentication Flow
1. Device boots → WiFi connect (or captive portal if no credentials)
2. HTTP POST `/api/v1/device/login` → JWT token saved to NVS
3. If `MQTT_ENABLED`: GET `/api/v1/device/mqtt-credentials` (Bearer JWT) → store in NVS
4. MQTT connect using fetched credentials
5. See [MQTT_ARCHITECTURE.md](../MQTT_ARCHITECTURE.md) for complete flow diagrams

## Configuration Patterns

### Configuration File Structure
The project uses a modular configuration system with separate files for different concerns:
- **[config.h](../include/config.h)**: Hardware pins, MQTT settings, network URLs, calibration constants - **tracked in git**
- **[device_profile.h](../include/device_profile.h)**: Device credentials (`DEFAULT_UUID`, `DEFAULT_SECRET`) and `SENSOR_CONFIGS` for this specific device - **gitignored**

**Initial Setup Workflow:**
1. Copy `device_profile.h.example` → `device_profile.h` and update with your device credentials and sensor list
2. Modify `config.h` directly if you need to adjust hardware pins, MQTT settings, or calibration constants

### Single Source of Truth: config.h
- **Hardware configuration** lives in [config.h](../include/config.h) (tracked in git)
- **Device-specific profile** lives in [device_profile.h](../include/device_profile.h.example) (gitignored)
To add/remove sensors for a specific device: modify `SENSOR_CONFIGS` in `device_profile.h` — no factory changes needed
MQTT settings: `MQTT_ENABLED`, `AGRONOS_MQTT_*` constants
Button settings: `BUTTON_LONG_PRESS_MS` (duration for factory reset trigger)
Never hardcode URLs or pins outside `config.h`

### Adding New Sensors (3-step process)
1. Create class derived from `SensorBase` (constructor signature: `int pin, const char* uuid`)
2. In same `.cpp` file: `static bool _reg = registerSensorFactory("YourSensorName", create_sensor_impl<YourSensorName>);`
3. Add entry to `SENSOR_CONFIGS[]` in `config.h` using the registered name as `type`

Example: [soil_moisture.cpp](../src/soil_moisture.cpp) for capacitive sensor implementation pattern

### Calibration Constants
- Sensor-specific calibration (e.g., `SOIL_MOISTURE_AIR_VALUE`, `SOIL_MOISTURE_WATER_VALUE`) goes in `config.h`
- Document calibration procedure in comments near constants

## Build & Development

### PlatformIO Commands
- Build: `pio run` (or use VS Code task: "Build Agronos WiFi Sensor")
- Upload: `pio run -t upload`
- Monitor: `pio run -t monitor` (115200 baud)
- Dependencies managed in [platformio.ini](../platformio.ini):
  - Adafruit DHT sensor library
  - ArduinoJson (v6.19.5+)
  - PubSubClient (MQTT)

### Testing Workflow
- Clear WiFi credentials for portal testing: uncomment `storage.setWifiCreds("", "")` in [main.cpp](../src/main.cpp#L82)
- Clear MQTT credentials: uncomment `storage.clearMqttCredentials()` in [main.cpp](../src/main.cpp#L119)
- Serial debugging: Check for "Attempting to send data via MQTT..." vs HTTP fallback messages

### Common Issues
- **NaN readings**: DHT pin conflict - ensure using `getSharedDht()` not direct `DHT` instantiation
- **MQTT connection failures**: Verify broker address, check ACL rules match topic pattern `devices/<username>/#`
- **HTTP/MQTT credential mismatch**: JWT token must be valid when fetching MQTT credentials

## Key Files Reference
- [main.cpp](../src/main.cpp): Boot sequence, provisioning flow, main loop
- [sensor_factory.cpp](../src/sensor_factory.cpp): Registry implementation and factory logic
- [data_sender.cpp](../src/data_sender.cpp): Protocol routing (MQTT/HTTP) and payload building
- [mqtt_client.cpp](../src/mqtt_client.cpp): MQTT connection management and publishing
- [MQTT_ARCHITECTURE.md](../MQTT_ARCHITECTURE.md): Complete system diagrams and decision trees

## Code Style
- Use `constexpr` for compile-time constants in `config.h`
- Prefer `std::unique_ptr` for sensor ownership, avoid raw `new`/`delete`
- Follow existing naming: PascalCase for classes, camelCase for methods, SCREAMING_SNAKE_CASE for config constants
- Minimal dynamic allocation on ESP32 - prefer stack or static storage where possible
