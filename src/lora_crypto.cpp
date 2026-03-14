#include "lora_crypto.h"
#include <cstring>
#include <mbedtls/aes.h>

// Build a deterministic 16-byte nonce for AES-128-CTR.
// Layout must match Laravel LoRaCryptoService::decrypt():
//   Bytes 0-3:  deviceId in Little-Endian
//   Bytes 4-7:  fcnt in Little-Endian
//   Bytes 8-15: zeros
void buildNonce(uint32_t deviceId, uint32_t fcnt, uint8_t nonce[16]) {
    memset(nonce, 0, 16);
    // ESP32 is natively Little-Endian, so memcpy produces LE bytes
    memcpy(nonce, &deviceId, 4);
    memcpy(nonce + 4, &fcnt, 4);
}

// AES-128-CTR encryption using ESP32's hardware-accelerated mbedtls.
// CTR mode: ciphertext length == plaintext length, no padding needed.
bool encryptPayload(const uint8_t* plaintext, size_t len,
                    const uint8_t key[16], const uint8_t nonce[16],
                    uint8_t* ciphertext) {
    if (!plaintext || len == 0 || !key || !nonce || !ciphertext) {
        return false;
    }

    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);

    int ret = mbedtls_aes_setkey_enc(&ctx, key, 128);
    if (ret != 0) {
        mbedtls_aes_free(&ctx);
        return false;
    }

    // mbedtls_aes_crypt_ctr modifies nonce_counter and stream_block internally,
    // so we need mutable copies
    uint8_t nonceCounter[16];
    memcpy(nonceCounter, nonce, 16);
    uint8_t streamBlock[16] = {0};
    size_t ncOffset = 0;

    ret = mbedtls_aes_crypt_ctr(&ctx, len,
                                 &ncOffset, nonceCounter, streamBlock,
                                 plaintext, ciphertext);

    mbedtls_aes_free(&ctx);
    return (ret == 0);
}
