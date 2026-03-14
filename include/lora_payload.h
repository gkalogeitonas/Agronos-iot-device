#pragma once

#include <stdint.h>
#include <stddef.h>

struct SensorReading; // forward declaration (defined in data_sender.h)

// Serialize sensor readings into binary format: N × 6 bytes
// Per sensor: [4 bytes UUID prefix (ASCII)] [2 bytes value as int16_t LE (value × 100)]
// Returns number of bytes written, or 0 on error.
size_t serializeReadings(const SensorReading* readings, size_t count,
                         uint8_t* outBuf, size_t bufSize);
