#pragma once

#include <stdint.h>
#include <stddef.h>

// Compute a 4-byte hash of the UUID string for nonce construction.
// Uses standard CRC32 (same polynomial as PHP's crc32() function).
uint32_t uuidHash(const char* uuid);

// Build a deterministic 16-byte nonce for AES-128-CTR.
// Layout: [4B CRC32(UUID) LE] [4B fcnt LE] [8B zeros]
void buildNonce(const char* uuid, uint32_t fcnt, uint8_t nonce[16]);

// Encrypt (or decrypt) a payload using AES-128-CTR with hardware acceleration.
// Ciphertext will have exactly the same length as plaintext (no padding).
// Returns true on success.
bool encryptPayload(const uint8_t* plaintext, size_t len,
                    const uint8_t key[16], const uint8_t nonce[16],
                    uint8_t* ciphertext);
