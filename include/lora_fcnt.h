#pragma once

#include <stdint.h>

class Storage; // forward declaration

// Initialize the frame counter subsystem.
// On cold boot: reads NVS, adds FCNT_COLD_BOOT_GAP, stores in RTC.
// On deep-sleep wake: RTC value is already valid.
void fcntInit(Storage& storage);

// Increment the frame counter and return the new value.
// Lazy-saves to NVS every FCNT_NVS_SAVE_INTERVAL transmissions.
uint32_t fcntNext(Storage& storage);
