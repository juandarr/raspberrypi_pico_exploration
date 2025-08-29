// Time synchronization state machine (Wi‑Fi/NTP + HTTP fallback)
#pragma once

#include <Arduino.h>
#include <uRTCLib.h>

// Initialize the state machine and bind the RTC instance.
// Call this from setup() after I2C/RTC are ready.
void timeSyncSetup(uRTCLib &rtc);

// Start an immediate sync attempt (equivalent to setting nextSyncAtMillis=now
// and entering warmup in the original code). Call at boot.
void timeSyncStart();

// Tick the state machine periodically from loop(). Non-blocking.
void timeSyncTick();

// Returns millis() when RTC was last synced.
uint32_t timeSyncLastSyncMillis();

// Returns number of successful updates performed so far.
uint32_t timeSyncUpdates();

