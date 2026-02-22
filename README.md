# Agronos WiFi Sensor

Lightweight ESP32 firmware that reads sensors and sends their measurements to an IoT server via MQTT or HTTP.

Overview
- Runs on ESP32 (PlatformIO).
- Reads configured sensors and sends JSON payloads via MQTT (preferred) or HTTP fallback.
- MQTT support with automatic credential provisioning.
- Captive Wi‑Fi portal for provisioning, persistent storage for Wi‑Fi creds, auth token, and MQTT credentials.

Provisioning & Authentication

- If no Wi‑Fi credentials are present in persistent storage, the device starts a captive portal (AP SSID defined by `AP_SSID`) to let a user connect and supply credentials. Once credentials are saved the device will attempt to connect to the configured Wi‑Fi network.

- As soon as the device has a network connection it automatically attempts to authenticate with the backend to obtain an access token. The token is stored in persistent storage and used by `DataSender` to authorize requests. Authentication is retried periodically if it fails.

- After successful authentication, if MQTT is enabled (`MQTT_ENABLED` in `config.h`), the device automatically fetches MQTT credentials (broker address, username, password) from the backend using the JWT token and stores them in persistent storage for subsequent use.

Button Functionality

- A physical button connected to GPIO 14 provides factory reset and wake-up capabilities.
- **Factory Reset**: Hold the button during device boot for 10 seconds (configurable via `BUTTON_LONG_PRESS_MS` in `config.h`) to wipe all stored data:
  - WiFi credentials
  - JWT authentication token
  - MQTT credentials
  
  After the reset, the device automatically restarts and enters provisioning mode.

- **Wake from Deep Sleep**: Pressing the button while the device is in deep sleep will wake it up immediately, allowing for on-demand sensor readings without waiting for the timer.

- **Hardware Connection**: Connect one leg of a momentary push button to GPIO 14 and the other to GND. The internal pull-up resistor is enabled in software.

Repository structure (important files)
- `platformio.ini` — build configuration (PlatformIO).
- `include/config.h` — single place to configure device, server, MQTT settings, and the sensor list (`SENSOR_CONFIGS`).
- `src/main.cpp` — program entrypoint (Wi‑Fi, portal, auth, MQTT provisioning, periodic reads and send).
- Sensor abstraction
  - `include/sensor.h` — `SensorBase` interface and registration API.
  - `src/sensor_factory.cpp` — registry and factory that builds sensors from `SENSOR_CONFIGS`.
  - Sensor implementations (examples): `src/dht11_temp.cpp`, `src/dht11_hum.cpp`, `src/soil_moisture.cpp`, `src/simulated.cpp`.
  - `include/dht_shared.h` / `src/dht_shared.cpp` — shared DHT instance per pin (prevents read conflicts).
  - `include/sensor_creator.h` — templated helper to register creator functions.
- Data transport
  - `include/data_sender.h`, `src/data_sender.cpp` —  MQTT-first with HTTP fallback and payload builder.
  - `include/mqtt_client.h`, `src/mqtt_client.cpp` — MQTT client wrapper for publishing sensor data.
- Wi‑Fi portal / storage / auth
  - `wifi_portal.*`, `storage.*`, `auth.*` — provisioning and authentication helpers.
- Architecture documentation
  - `MQTT_ARCHITECTURE.md` — detailed MQTT architecture, flow diagrams, and decision trees.

Build & flash
1. Install PlatformIO (VS Code recommended).
2. Open project folder and run the provided VS Code task: "Build Agronos WiFi Sensor" or use `pio run` from the command line.
3. Upload with PlatformIO (e.g. `pio run -t upload`).

MQTT Support

The device supports MQTT as the primary protocol for sending sensor data with automatic HTTP fallback:

- **Automatic Provisioning**: MQTT credentials (broker, username, password) are fetched from the backend after JWT authentication.
- **Configurable**: Enable/disable MQTT via `MQTT_ENABLED` in `config.h`.
- **Topics**: Publishes to `devices/{device-uuid}/sensors` with QoS 1 for reliability.
- **Persistent Storage**: MQTT credentials stored in NVS for persistence across reboots.

MQTT configuration options in `config.h`:
- `MQTT_ENABLED` — enable/disable MQTT (default: true)
- `AGRONOS_MQTT_PORT` — broker port (1883 plain, 8883 TLS)
- `AGRONOS_MQTT_USE_TLS` — use secure connection
- `AGRONOS_MQTT_KEEPALIVE` — keep-alive interval (seconds)
- `AGRONOS_MQTT_QOS_DATA` — QoS level for sensor data (default: 1)

See `MQTT_ARCHITECTURE.md` for detailed flow diagrams and architecture.

How to configure sensors
All sensor configuration is in `include/config.h`. Example entry:

```
constexpr SensorConfig SENSOR_CONFIGS[] = {
    { "DHT11TemperatureReader", 21, "Device-1-Temp" },
    { "DHT11HumidityReader",    21, "Device-1-Hum" }
};
```
- `type` should match the sensor class name that is registered with the factory (or an existing alias).
- `pin` is the GPIO used by the sensor (or -1 if not applicable).
- `uuid` is the sensor UUID used by the server.

Soil moisture sensor (SEN0193)

This firmware includes support for a capacitive soil moisture sensor (DFRobot SEN0193) implemented in `src/soil_moisture.cpp`.

- **Wiring**: connect the sensor analog output to an ESP32 ADC pin (default config uses GPIO32).
- **Config**: add an entry like `{ "SoilMoistureSensor", 32, "Soil-Moisture-1" }` in `SENSOR_CONFIGS`.
- **Output**: the firmware converts the raw ADC reading into a percentage $0\%$ (dry) to $100\%$ (wet).
- **Calibration**: set `SOIL_MOISTURE_AIR_VALUE` (dry/air reading) and `SOIL_MOISTURE_WATER_VALUE` (wet/water reading) in `include/config.h` based on calibration  measurements in  dry and wet environment.

Adding a new sensor (no factory changes required)
1. Implement a class derived from `SensorBase` (prefer signature: `YourSensor(int pin, const char* uuid)`).
2. In the same `.cpp` file register it with: `static bool _reg = registerSensorFactory("YourSensor", create_sensor_impl<YourSensor>);` (see `include/sensor_creator.h`).
3. Add a `SensorConfig` entry to `include/config.h` using the class name as `type`.
4. Build and flash.

Notes and best practices
- DHT sensors sharing the same pin use a shared DHT instance (`dht_shared`) to avoid conflicts; prefer declaring temperature and humidity as separate sensor entries that reference the same pin.
- Factory now returns `std::vector<std::unique_ptr<SensorBase>>` — ownership is RAII-managed.
- For multi-value sensors (e.g., sensors that produce temperature + humidity in one device) consider extending the measurement model (future work).

Troubleshooting
- If a sensor returns `NaN` readings, check wiring and pin numbers in `config.h` and ensure only one physical DHT object per pin (the code uses shared instances).
- If data transmission fails:
  - Check `BASE_URL` in `include/config.h` and verify network/auth token in storage.
  - For MQTT issues, verify `MQTT_ENABLED` is true and MQTT credentials were provisioned (check serial logs).
  - The device automatically falls back to HTTP if MQTT connection fails.
  - MQTT credentials are fetched from `BASE_URL/api/v1/device/mqtt-credentials` using the JWT token.
- If MQTT connection repeatedly fails, check broker availability and ensure the backend returns valid MQTT credentials.



