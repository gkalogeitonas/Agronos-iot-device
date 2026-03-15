#include "lora_radio.h"
#include "config.h"
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <cstring>

bool loraRadioInit() {
    SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_SS_PIN);
    LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);

    if (!LoRa.begin(LORA_FREQUENCY)) {
        Serial.println("[LoRa] Radio init failed");
        return false;
    }

    LoRa.setSpreadingFactor(LORA_SF);
    LoRa.setSignalBandwidth(LORA_BW);
    LoRa.setCodingRate4(LORA_CR);
    LoRa.setSyncWord(LORA_SYNC_WORD);
    LoRa.setTxPower(LORA_TX_POWER);

    Serial.printf("[LoRa] Radio init OK: freq=%ld SF=%d BW=%ld CR=4/%d SW=0x%02X TX=%ddBm\n",
                  LORA_FREQUENCY, LORA_SF, LORA_BW, LORA_CR, LORA_SYNC_WORD, LORA_TX_POWER);
    return true;
}

bool loraTransmit(const char* uuid, uint32_t fcnt,
                  const uint8_t* ciphertext, size_t cipherLen) {
    if (!uuid || !ciphertext || cipherLen == 0) return false;

    uint8_t uuidLen = (uint8_t)strlen(uuid);

    // Build packet: [1B uuid_len][UUID bytes][4B fcnt LE][ciphertext]
    uint8_t fcntBytes[4];
    memcpy(fcntBytes, &fcnt, 4); // ESP32 is natively LE

    LoRa.beginPacket();
    LoRa.write(uuidLen);
    LoRa.write((const uint8_t*)uuid, uuidLen);
    LoRa.write(fcntBytes, 4);
    LoRa.write(ciphertext, cipherLen);
    LoRa.endPacket(true); // true = blocking, wait for TX complete

    Serial.printf("[LoRa] TX: uuid=%s fcnt=%u payload=%u bytes (total=%u)\n",
                  uuid, fcnt, cipherLen, 1 + uuidLen + 4 + cipherLen);
    return true;
}

void loraRadioSleep() {
    LoRa.sleep();
    Serial.println("[LoRa] Radio entering sleep");
}
