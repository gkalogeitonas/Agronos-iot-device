#pragma once

#include "sensor.h" // SensorConfig is defined in the sensor API header
#include "secrets.h" // Device credentials (DEFAULT_UUID, DEFAULT_SECRET)
#include "sensors_config.h" // Sensor configuration (SENSOR_CONFIGS, SENSOR_CONFIG_COUNT)

// Network / server
constexpr const char* BASE_URL       = "https://agronos.kalogeitonas.xyz";

// Captive portal AP
constexpr const char* AP_SSID = "ESP_Config";
constexpr const char* AP_PASS = ""; // optional

// Button configuration
constexpr unsigned long BUTTON_LONG_PRESS_MS = 10000; // 10 seconds to trigger reset

constexpr int BUTTON_PIN = 14;

#if defined(CONFIG_IDF_TARGET_ESP32C6)
constexpr int I2C_SDA = 19;
constexpr int I2C_SCL = 20;
#else
constexpr int I2C_SDA = 21;
constexpr int I2C_SCL = 22;
#endif

// DHT sensor
constexpr int DHT11_PIN = 21;

// non-blocking read interval
constexpr unsigned long SENSORS_READ_INTERVAL_MS = 3 * 60 * 1000; // 3 minute

// Auth manager
constexpr unsigned long AUTH_RETRY_INTERVAL_MS = 30000;


// ==================== MQTT CONFIGURATION ====================
// Enable/Disable MQTT protocol (set to false to use HTTP only)
constexpr bool MQTT_ENABLED = true;

// MQTT connection settings
constexpr uint16_t AGRONOS_MQTT_PORT = 1883;              // 8883 for TLS, 1883 for plain
constexpr bool AGRONOS_MQTT_USE_TLS = false;              // Use secure connection
constexpr uint16_t AGRONOS_MQTT_KEEPALIVE = 60;          // Keep-alive interval (seconds)
constexpr uint16_t AGRONOS_MQTT_RECONNECT_DELAY = 5000;  // Reconnection delay (ms)
constexpr bool AGRONOS_MQTT_CLEAN_SESSION = true;        // Start with clean session

// MQTT topics (will be formatted with device UUID)
// Use topics matching broker ACL: devices/<username>/# so EMQX allows publishes
constexpr const char* AGRONOS_MQTT_TOPIC_DATA = "devices/%s/sensors";
constexpr const char* AGRONOS_MQTT_TOPIC_STATUS = "devices/%s/status";
constexpr const char* AGRONOS_MQTT_TOPIC_COMMAND = "devices/%s/commands";

// MQTT QoS levels
constexpr int AGRONOS_MQTT_QOS_DATA = 1;      // At least once for sensor data
constexpr int AGRONOS_MQTT_QOS_STATUS = 0;    // Fire and forget for status

// NOTE: MQTT server, username, and password are fetched dynamically 
// from the backend using the JWT token after HTTP authentication.
// ============================================================

// Soil moisture sensor calibration (SEN0193)
constexpr int SOIL_MOISTURE_AIR_VALUE = 2941;    // Dry
constexpr int SOIL_MOISTURE_WATER_VALUE = 1324;  // Wet
