#pragma once

#include "sensor.h" // SensorConfig is defined in the sensor API header
//#include "Test-Device-2.h" // Device-specific credentials + sensor list
#include "Test-Device-LoRa-Battery.h" // Device-specific credentials + sensor list

// Network / server
constexpr const char* BASE_URL       = "https://agronos.kalogeitonas.xyz";

// Captive portal AP
constexpr const char* AP_SSID = "ESP_Config";
constexpr const char* AP_PASS = ""; // optional

// Button configuration
constexpr unsigned long BUTTON_LONG_PRESS_MS = 10000; // 10 seconds to trigger reset

#ifdef LORA_NODE
// On TTGO LoRa32 V2.1, GPIO 14 is used by LoRa RST.
// Use GPIO 0 (BOOT button) for factory reset.
constexpr int BUTTON_PIN = 0;
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
// On ESP32-C6, deep sleep wakeup is supported only on GPIO 0-7 (RTC/VDDPST1 pins).
// GPIO 14 is not supported for deep sleep wakeup on C6.
// We select GPIO 7 (D11 on FireBeetle 2 C6) as the button pin.
constexpr int BUTTON_PIN = 7;
#else
// On standard ESP32, GPIO 14 is an RTC pin and supports wakeup.
constexpr int BUTTON_PIN = 14; 
#endif

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
constexpr unsigned long SENSORS_READ_INTERVAL_MS = 10 * 60 * 1000; // 10 minutes

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

// ==================== LoRa CONFIGURATION (TTGO LoRa32 V2.1) ====================
#ifdef LORA_NODE

// LoRa SPI pins (TTGO LoRa32 V2.1)
constexpr int LORA_SCK_PIN   = 5;
constexpr int LORA_MISO_PIN  = 19;
constexpr int LORA_MOSI_PIN  = 27;
constexpr int LORA_SS_PIN    = 18;
constexpr int LORA_RST_PIN   = 14;
constexpr int LORA_DIO0_PIN  = 26;

// LoRa radio settings (must match gateway defaults)
constexpr long   LORA_FREQUENCY  = 868000000;  // 868 MHz (Europe)
constexpr int    LORA_SF         = 10;          // Spreading Factor 10
constexpr long   LORA_BW         = 125000;      // 125 kHz bandwidth
constexpr int    LORA_CR         = 5;           // Coding Rate 4/5
constexpr uint8_t LORA_SYNC_WORD = 0x12;        // Private network sync word
constexpr int    LORA_TX_POWER   = 20;          // 20 dBm transmit power

// Frame counter management
constexpr uint32_t FCNT_NVS_SAVE_INTERVAL = 100;  // Save to NVS every N transmissions
constexpr uint32_t FCNT_COLD_BOOT_GAP     = 100;  // Jump on cold boot (< backend MAX_FCNT_GAP=10000)

// TTGO LoRa32 V2.1 misc pins
constexpr int LORA_LED_PIN     = 25;  // Built-in LED
constexpr int LORA_BATTERY_PIN = 35;  // Battery ADC (ADC1_CH7)

#endif // LORA_NODE
// ================================================================
