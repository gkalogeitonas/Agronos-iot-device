#pragma once

#include <stdint.h>
#include <stddef.h>

// Initialize the LoRa radio (SPI + LoRa library) with settings from config.h.
// Returns true on success.
bool loraRadioInit();

// Transmit a complete LoRa packet: [1B uuid_len][UUID bytes][4B fcnt LE][ciphertext]
// Returns true on success.
bool loraTransmit(const char* uuid, uint32_t fcnt,
                  const uint8_t* ciphertext, size_t cipherLen);

// Put LoRa radio into sleep mode (call before deep sleep to save power).
void loraRadioSleep();
