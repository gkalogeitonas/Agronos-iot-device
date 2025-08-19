# Agronos WiFi Sensor

Lightweight ESP32 firmware that reads sensors and posts their measurements to an IoT server.

Overview
- Runs on ESP32 (PlatformIO).
- Reads configured sensors and sends JSON payloads to: `BASE_URL`/api/v1/device/data.
- Captive Wi‑Fi portal for provisioning, persistent storage for Wi‑Fi creds and auth token.

Provisioning & Authentication

- If no Wi‑Fi credentials are present in persistent storage, the device starts a captive portal (AP SSID defined by `AP_SSID`) to let a user connect and supply credentials. Once credentials are saved the device will attempt to connect to the configured Wi‑Fi network.

- As soon as the device has a network connection it automatically attempts to authenticate with the backend to obtain an access token. The token is stored in persistent storage and used by `DataSender` to authorize HTTP POST requests. Authentication is retried periodically if it fails.

Repository structure (important files)
- `platformio.ini` — build configuration (PlatformIO).
- `include/config.h` — single place to configure device, server and the sensor list (`SENSOR_CONFIGS`).
- `src/main.cpp` — program entrypoint (Wi‑Fi, portal, auth, periodic reads and send).
- Sensor abstraction
  - `include/sensor.h` — `SensorBase` interface and registration API.
  - `src/sensor_factory.cpp` — registry and factory that builds sensors from `SENSOR_CONFIGS`.
  - Sensor implementations (examples): `src/dht11_temp.cpp`, `src/dht11_hum.cpp`, `src/simulated.cpp`.
  - `include/dht_shared.h` / `src/dht_shared.cpp` — shared DHT instance per pin (prevents read conflicts).
  - `include/sensor_creator.h` — templated helper to register creator functions.
- Data transport
  - `include/data_sender.h`, `src/data_sender.cpp` — JSON builder and HTTP POST logic.
- Wi‑Fi portal / storage / auth
  - `wifi_portal.*`, `storage.*`, `auth.*` — provisioning and authentication helpers.

Build & flash
1. Install PlatformIO (VS Code recommended).
2. Open project folder and run the provided VS Code task: "Build Agronos WiFi Sensor" or use `pio run` from the command line.
3. Upload with PlatformIO (e.g. `pio run -t upload`).

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
- If HTTP POST fails, check `BASE_URL` in `include/config.h` and verify network/auth token in storage.



