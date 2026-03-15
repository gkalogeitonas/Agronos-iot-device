// LoRa-only entry point for TTGO LoRa32 V2.1
// Reads sensors, serializes to binary, encrypts with AES-128-CTR,
// transmits via LoRa radio, then enters deep sleep.
// WiFi/MQTT/HTTP are not used — the LoRa Gateway handles internet forwarding.

#include <Arduino.h>
#include <esp_sleep.h>
#include <cmath>

#include "config.h"
#include "storage.h"
#include "sensor.h"
#include "data_sender.h"   // for SensorReading struct
#include "lora_payload.h"
#include "lora_crypto.h"
#include "lora_fcnt.h"
#include "lora_radio.h"

// Max sensors — 256-byte LoRa packets: (256 - 8 header) / 6 = 41 sensors max
static constexpr size_t MAX_PAYLOAD_SIZE = 246;

Storage storage;

static void printHex(const char* label, const uint8_t* data, size_t len) {
    Serial.print(label);
    Serial.print(": ");
    for (size_t i = 0; i < len; ++i) {
        if (data[i] < 0x10) Serial.print('0');
        Serial.print(data[i], HEX);
    }
    Serial.println();
}

// Sensor instances created via factory
static std::vector<std::unique_ptr<SensorBase>> sensors;

// Check if button is held for long press to reset all storage
static void checkButtonReset() {
    if (digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("Button pressed, checking for long press...");
        unsigned long pressStart = millis();

        while (digitalRead(BUTTON_PIN) == LOW) {
            if (millis() - pressStart >= BUTTON_LONG_PRESS_MS) {
                Serial.println("Long press detected! Wiping all storage data...");
                storage.clearAll();
                Serial.println("Storage cleared. Restarting device...");
                delay(1000);
                ESP.restart();
            }
            delay(100);
        }
        Serial.println("Button released before threshold");
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("================================");
    Serial.println(" Agronos LoRa Node");
    Serial.println("================================");

    // Button setup & factory reset check
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    checkButtonReset();

    // LED indicator
    pinMode(LORA_LED_PIN, OUTPUT);
    digitalWrite(LORA_LED_PIN, HIGH); // LED on during active cycle

    // 1. Initialize frame counter (RTC or NVS cold-boot recovery)
    fcntInit(storage);

    // 2. Initialize LoRa radio
    if (!loraRadioInit()) {
        Serial.println("FATAL: LoRa radio init failed. Sleeping...");
        esp_sleep_enable_timer_wakeup((uint64_t)SENSORS_READ_INTERVAL_MS * 1000ULL);
        esp_deep_sleep_start();
    }

    // 3. Create sensors from config (reuses factory pattern)
    sensors = createSensors();
    Serial.printf("Created %u sensors\n", sensors.size());

    // 4. Read all sensors
    std::vector<SensorReading> readings;
    readings.reserve(sensors.size());

    for (size_t i = 0; i < sensors.size(); ++i) {
        float value;
        if (sensors[i]->read(value)) {
            float rounded = roundf(value * 100.0f) / 100.0f;
            readings.push_back({ sensors[i]->uuid(), rounded });
            Serial.printf("  %s = %.2f\n", sensors[i]->uuid(), rounded);
        } else {
            Serial.printf("  %s = FAILED\n", sensors[i]->uuid());
        }
    }

    if (readings.empty()) {
        Serial.println("No sensor readings. Sleeping...");
        loraRadioSleep();
        digitalWrite(LORA_LED_PIN, LOW);
        esp_sleep_enable_timer_wakeup((uint64_t)SENSORS_READ_INTERVAL_MS * 1000ULL);
        esp_deep_sleep_start();
    }

    // 5. Serialize to binary payload (N × 6 bytes)
    uint8_t plaintext[MAX_PAYLOAD_SIZE];
    size_t payloadLen = serializeReadings(readings.data(), readings.size(),
                                          plaintext, sizeof(plaintext));
    if (payloadLen == 0) {
        Serial.println("Serialization failed. Sleeping...");
        loraRadioSleep();
        digitalWrite(LORA_LED_PIN, LOW);
        esp_sleep_enable_timer_wakeup((uint64_t)SENSORS_READ_INTERVAL_MS * 1000ULL);
        esp_deep_sleep_start();
    }
    Serial.printf("Payload: %u bytes (%u sensors)\n", payloadLen, readings.size());
    printHex("Raw payload", plaintext, payloadLen);

    // 6. Get next frame counter
    uint32_t fcnt = fcntNext(storage);

    // 7. Build nonce and encrypt
    uint8_t nonce[16];
    buildNonce(DEFAULT_UUID, fcnt, nonce);

    uint8_t ciphertext[MAX_PAYLOAD_SIZE];
    if (!encryptPayload(plaintext, payloadLen, LORA_AES_KEY, nonce, ciphertext)) {
        Serial.println("Encryption failed. Sleeping...");
        loraRadioSleep();
        digitalWrite(LORA_LED_PIN, LOW);
        esp_sleep_enable_timer_wakeup((uint64_t)SENSORS_READ_INTERVAL_MS * 1000ULL);
        esp_deep_sleep_start();
    }

    printHex("Encrypted payload", ciphertext, payloadLen);

    // Print all transited data for backend test purposes
    Serial.printf("Transmit meta: uuid=%s, fcnt=%u, payloadLen=%u\n", DEFAULT_UUID, fcnt, payloadLen);
    printHex("Transmit ciphertext", ciphertext, payloadLen);

    // 8. Transmit via LoRa
    if (loraTransmit(DEFAULT_UUID, fcnt, ciphertext, payloadLen)) {
        Serial.printf("TX success: uuid=%s fcnt=%u\n", DEFAULT_UUID, fcnt);
    } else {
        Serial.println("TX failed");
    }

    // 9. Prepare for deep sleep
    loraRadioSleep();
    digitalWrite(LORA_LED_PIN, LOW);

    uint64_t sleepUs = (uint64_t)SENSORS_READ_INTERVAL_MS * 1000ULL;
    Serial.printf("Entering deep sleep for %lu ms\n", SENSORS_READ_INTERVAL_MS);

    esp_sleep_enable_timer_wakeup(sleepUs);
    esp_deep_sleep_start();
}

void loop() {
    // Never reached — device always deep-sleeps after setup()
}
