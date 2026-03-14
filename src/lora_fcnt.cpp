#include "lora_fcnt.h"
#include "storage.h"
#include "config.h"
#include <Arduino.h>
#include <esp_sleep.h>

// Frame counter in RTC memory — survives deep sleep, lost on power cycle
RTC_DATA_ATTR static uint32_t rtcFcnt = 0;
RTC_DATA_ATTR static bool rtcInitialized = false;

// Transmission counter for lazy NVS saves
RTC_DATA_ATTR static uint32_t txSinceLastSave = 0;

void fcntInit(Storage& storage) {
    if (rtcInitialized) {
        // Waking from deep sleep: RTC value is valid, nothing to do
        Serial.printf("[FCNT] Deep-sleep wake, fcnt=%u\n", rtcFcnt);
        return;
    }

    // Cold boot: read last saved value from NVS and add safety gap
    uint32_t nvsFcnt = storage.getLoraFcnt();
    rtcFcnt = nvsFcnt + FCNT_COLD_BOOT_GAP;
    txSinceLastSave = 0;
    rtcInitialized = true;

    // Persist the jumped value immediately so the next cold boot starts even higher
    storage.setLoraFcnt(rtcFcnt);

    Serial.printf("[FCNT] Cold boot: NVS=%u, starting at %u (gap=%u)\n",
                  nvsFcnt, rtcFcnt, FCNT_COLD_BOOT_GAP);
}

uint32_t fcntNext(Storage& storage) {
    rtcFcnt++;
    txSinceLastSave++;

    // Lazy save: write to NVS only every N transmissions to reduce flash wear
    if (txSinceLastSave >= FCNT_NVS_SAVE_INTERVAL) {
        storage.setLoraFcnt(rtcFcnt);
        txSinceLastSave = 0;
        Serial.printf("[FCNT] Saved to NVS: %u\n", rtcFcnt);
    }

    return rtcFcnt;
}
