// UI module for TFT rendering of date, time, temperature and update info
#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <uRTCLib.h>

// Initializes static UI labels and prepares fonts/colors.
void uiSetup(TFT_eSPI &tft);

// Updates the dynamic UI sections (date, time, temp, since-update and count).
// - rtc: DS3231 instance
// - sinceSyncMs: millis elapsed since last successful sync
// - updates: number of successful updates performed
void uiTick(TFT_eSPI &tft, uRTCLib &rtc, uint32_t sinceSyncMs, uint32_t updates);

