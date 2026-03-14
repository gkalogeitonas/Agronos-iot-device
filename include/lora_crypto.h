#pragma once

#include <stdint.h>
#include <stddef.h>

// Build a deterministic 16-byte nonce for AES-128-CTR.
// Layout: [4B deviceId LE] [4B fcnt LE] [8B zeros]
void buildNonce(uint32_t deviceId, uint32_t fcnt, uint8_t nonce[16]);

// Encrypt (or decrypt) a payload using AES-128-CTR with hardware acceleration.
// Ciphertext will have exactly the same length as plaintext (no padding).
// Returns true on success.
bool encryptPayload(const uint8_t* plaintext, size_t len,
                    const uint8_t key[16], const uint8_t nonce[16],
                    uint8_t* ciphertext);
