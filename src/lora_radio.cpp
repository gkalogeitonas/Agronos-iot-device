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

bool loraTransmit(uint32_t deviceId, uint32_t fcnt,
                  const uint8_t* ciphertext, size_t cipherLen) {
    if (!ciphertext || cipherLen == 0) return false;

    // Build packet: [4B deviceId LE][4B fcnt LE][ciphertext]
    // ESP32 is natively LE so memcpy produces correct byte order
    uint8_t header[8];
    memcpy(header, &deviceId, 4);
    memcpy(header + 4, &fcnt, 4);

    LoRa.beginPacket();
    LoRa.write(header, 8);
    LoRa.write(ciphertext, cipherLen);
    LoRa.endPacket(true); // true = blocking, wait for TX complete

    Serial.printf("[LoRa] TX: devId=%u fcnt=%u payload=%u bytes (total=%u)\n",
                  deviceId, fcnt, cipherLen, 8 + cipherLen);
    return true;
}

void loraRadioSleep() {
    LoRa.sleep();
    Serial.println("[LoRa] Radio entering sleep");
}
