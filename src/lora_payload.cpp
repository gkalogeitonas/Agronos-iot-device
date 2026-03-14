#include "lora_payload.h"
#include "data_sender.h"
#include <cstring>

// Binary payload format per sensor (6 bytes):
//   Bytes 0-3: First 4 ASCII characters of the sensor UUID
//   Bytes 4-5: int16_t Little-Endian of (value * 100)
//
// Total payload size = count * 6 bytes.
// Must match Laravel LoRaCryptoService::deserialize() on the backend.

static constexpr size_t CHUNK_SIZE = 6;

size_t serializeReadings(const SensorReading* readings, size_t count,
                         uint8_t* outBuf, size_t bufSize) {
    size_t needed = count * CHUNK_SIZE;
    if (!readings || count == 0 || !outBuf || bufSize < needed) {
        return 0;
    }

    for (size_t i = 0; i < count; ++i) {
        uint8_t* chunk = outBuf + (i * CHUNK_SIZE);

        // Bytes 0-3: first 4 chars of UUID, zero-padded if shorter
        size_t uuidLen = strlen(readings[i].uuid);
        for (size_t j = 0; j < 4; ++j) {
            chunk[j] = (j < uuidLen) ? static_cast<uint8_t>(readings[i].uuid[j]) : 0;
        }

        // Bytes 4-5: value * 100 as int16_t in Little-Endian
        // ESP32 is natively LE so memcpy works directly
        int16_t scaled = static_cast<int16_t>(readings[i].value * 100.0f);
        memcpy(chunk + 4, &scaled, sizeof(int16_t));
    }

    return needed;
}
