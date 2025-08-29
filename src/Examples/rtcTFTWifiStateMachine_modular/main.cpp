#include "Arduino.h"
// ====== Single-core, event-driven state machine (Pico W) ======
// UI + I2C run every loop; Wi‑Fi/NTP is non‑blocking via states.
// Adds robust DNS resolution, multiple NTP fallbacks, and HTTP fallback
// (for networks that block UDP/123). Verbose serial logs at each step.

// ====== RTC DS3231 ======
#define URTCLIB_WIRE Wire1 // bind uRTCLib to Wire1
#include <Wire.h>
#include <uRTCLib.h>

// ====== TFT ILI9341 ======
#include "Examples/resources/Free_Fonts.h"
#include <SPI.h>
#include <TFT_eSPI.h>

// Local modules
#include "ui.h"
#include "time_sync.h"

// ====== WiFi/NTP (state machine lives in time_sync.cpp) ======

// ================= Globals =================
TFT_eSPI tft = TFT_eSPI();
uRTCLib rtc(0x68);

// UI helpers are now in ui.cpp

// Wi‑Fi/NTP helpers and RTC sync were extracted to time_sync.cpp

// State machine extracted to time_sync.cpp

void setup() {
  Serial.begin(115200);
  Serial.println("[boot] Pico W RTC/NTP state machine");

  // I2C on Wire1 (RTC)
  Wire1.setSDA(14);
  Wire1.setSCL(15);
  Wire1.begin();

  // TFT init
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  // UI labels and fonts
  uiSetup(tft);

  // Time sync state machine setup and immediate start
  timeSyncSetup(rtc);
  timeSyncStart();
}

void loop() {
  // ---- State machine tick ----
  timeSyncTick();

  // ---- UI tick ----
  const uint32_t now = millis();
  const uint32_t sinceSync = now - timeSyncLastSyncMillis();
  uiTick(tft, rtc, sinceSync, timeSyncUpdates());

  delay(20); // keep lwIP timers happy without freezing UI
}
