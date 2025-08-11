#include "Arduino.h"
#include "Wire.h"
#include "uRTCLib.h"

#include "Examples/resources/Free_Fonts.h"
#include <SPI.h>
#include <TFT_eSPI.h>

// ---- WiFi/NTP ----
#include <WiFi.h>
#include <WiFiUdp.h>
#include <time.h>

// ---------- Your WiFi ----------
const char* WIFI_SSID = "YOUR_WIFI";
const char* WIFI_PASS = "YOUR_PASSWORD";

// ---------- NTP config ----------
const char* NTP_SERVER   = "pool.ntp.org";
const uint16_t NTP_PORT  = 123;
const uint32_t NTP_TIMEOUT_MS = 1500;

// Bogotá is UTC-5, no DST.
const int32_t TZ_OFFSET_SECONDS = -5 * 3600;

// Resync cadence: 6 hours
const uint32_t SYNC_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;

WiFiUDP ntpUDP;
uint32_t lastSyncMillis = 0;

// Create tft object
TFT_eSPI tft = TFT_eSPI();

// uRTCLib rtc;
uRTCLib rtc(0x68);

uint8_t last_day = 99, last_month = 0, last_year = 0;
uint8_t last_hour = 99, last_minute = 99, last_second = 99;
int last_temp = -100;

const int DATE_Y = 45;
const int TIME_Y = 108;
const int TEMP_Y = 180;

// ------------------- UI helpers (unchanged) -------------------
void clearAndDrawText(String text, int x, int y, int w, int h,
                      uint16_t textColor, uint16_t bgColor) {
  tft.fillRect(x, y, w, h, bgColor);
  tft.setTextColor(textColor, bgColor);
  tft.drawString(text, x, y);
}

void dateUpdate() {
  uint8_t day = rtc.day();
  uint8_t month = rtc.month();
  uint8_t year = rtc.year();

  if (day != last_day || month != last_month || year != last_year) {
    char dateBuffer[12];
    sprintf(dateBuffer, "%02d/%02d/%d", day, month, year);

    tft.setFreeFont(FMB12);
    clearAndDrawText(dateBuffer, tft.textWidth("Date:  "), DATE_Y, 112, 16,
                     TFT_YELLOW, TFT_BLACK);

    last_day = day;
    last_month = month;
    last_year = year;
  }
}

void timeUpdate() {
  int xpos = 0;
  uint8_t hour = rtc.hour();
  uint8_t minute = rtc.minute();
  uint8_t second = rtc.second();

  tft.setFreeFont(FMB18);
  char hourBuffer[3];
  sprintf(hourBuffer, "%02d", hour);
  if (hour != last_hour) {
    clearAndDrawText(hourBuffer, xpos, TIME_Y, 44, 22, TFT_GREEN, TFT_BLACK);
    last_hour = hour;
  }
  xpos += tft.textWidth(hourBuffer);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  xpos += tft.drawChar(':', xpos, TIME_Y + 18);

  char minuteBuffer[3];
  sprintf(minuteBuffer, "%02d", minute);
  if (minute != last_minute) {
    clearAndDrawText(minuteBuffer, xpos, TIME_Y, 44, 22, TFT_SKYBLUE, TFT_BLACK);
    last_minute = minute;
  }

  xpos += tft.textWidth(minuteBuffer);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  xpos += tft.drawChar(':', xpos, TIME_Y + 18);
  tft.setFreeFont(FMB12);

  char secondBuffer[3];
  sprintf(secondBuffer, "%02d", second);
  if (second != last_second) {
    clearAndDrawText(secondBuffer, xpos, TIME_Y, 32, 16, TFT_MAGENTA, TFT_BLACK);
    last_second = second;
  }
}

void tempUpdate() {
  int temp = rtc.temp() / 100;
  if (temp != last_temp) {
    char tempBuffer[5];
    sprintf(tempBuffer, "%2d", temp);

    clearAndDrawText(tempBuffer, tft.textWidth("Temp:  "), TEMP_Y, 56, 14,
                     TFT_GREENYELLOW, TFT_BLACK);

    int16_t x = tft.textWidth("Temp:  ") + tft.textWidth(tempBuffer) + 4;
    int16_t y = TEMP_Y + 4;
    tft.fillCircle(x, y, 2, TFT_GREEN);
    tft.drawString("C", x + 4, TEMP_Y);
    last_temp = temp;
  }
}

// ------------------- WiFi/NTP helpers -------------------
static void wifiPowerOff() {
  ntpUDP.stop();                     // close any UDP sockets
  WiFi.disconnect(true);             // drop connection and clear config
  WiFi.end();                        // deinit driver (frees memory, turns radio off)
  #ifdef WIFI_OFF
  WiFi.mode(WIFI_OFF);               // if the core defines it, be explicit
  #endif
}

static bool wifiPowerOnAndConnect(uint32_t timeoutMs = 15000) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(200);
  }
  return WiFi.status() == WL_CONNECTED;
}

static bool getUnixTimeFromNTP(uint32_t& unixTimeOut) {
  uint8_t pkt[48] = {0};
  pkt[0] = 0x1B; // LI=0, VN=3, Mode=3 (client)

  if (!ntpUDP.begin(0)) return false;
  if (ntpUDP.beginPacket(NTP_SERVER, NTP_PORT) != 1) return false;
  ntpUDP.write(pkt, sizeof(pkt));
  if (ntpUDP.endPacket() != 1) return false;

  uint32_t start = millis();
  while (millis() - start < NTP_TIMEOUT_MS) {
    int size = ntpUDP.parsePacket();
    if (size >= 48) {
      ntpUDP.read(pkt, sizeof(pkt));
      ntpUDP.stop();

      uint32_t secsSince1900 =
        ((uint32_t)pkt[40] << 24) | ((uint32_t)pkt[41] << 16) |
        ((uint32_t)pkt[42] << 8)  | ((uint32_t)pkt[43]);

      const uint32_t NTP_UNIX_DELTA = 2208988800UL;
      unixTimeOut = secsSince1900 - NTP_UNIX_DELTA;
      return true;
    }
    delay(10);
  }
  ntpUDP.stop();
  return false;
}

static void setRTCFromUnix(uint32_t unixTimeUtc, int32_t tzOffsetSeconds) {
  time_t t = (time_t)((int64_t)unixTimeUtc + tzOffsetSeconds);
  struct tm* tm_p = gmtime(&t);
  if (!tm_p) return;
  uint8_t dow = (uint8_t)tm_p->tm_wday + 1; // DS3231 wants 1..7, Sun=1

  rtc.set(
    (uint8_t)tm_p->tm_sec,
    (uint8_t)tm_p->tm_min,
    (uint8_t)tm_p->tm_hour,
    dow,
    (uint8_t)tm_p->tm_mday,
    (uint8_t)tm_p->tm_mon + 1,
    (uint16_t)tm_p->tm_year + 1900
  );
}

// Convert a civil date-time (UTC) to Unix time (seconds since 1970-01-01).
// Works for years in a wide range; returns uint32_t (good through year 2106).
static uint32_t unixFromYMDHMS(int year, int mon, int day, int hour, int min, int sec) {
  // Howard Hinnant’s days-from-civil algorithm (condensed, no DST, no TZ).
  // Month 1..12, Day 1..31
  year -= mon <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = (unsigned)(year - era * 400);                 // [0, 399]
  const unsigned doy = (153u * (mon + (mon > 2 ? -3 : 9)) + 2) / 5 + day - 1; // [0, 365]
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;        // [0, 146096]
  const int days = era * 146097 + (int)doe - 719468;                 // days since 1970-01-01
  return (uint32_t)days * 86400u + (uint32_t)hour * 3600u + (uint32_t)min * 60u + (uint32_t)sec;
}

// One-shot: power on WiFi, sync RTC, power off WiFi
static bool doOneNtpSync() {

  Serial.println("\nStarting update of time!");
  bool ok = false;
  if (wifiPowerOnAndConnect(12000)) {
    uint32_t unixUtc;
    if (getUnixTimeFromNTP(unixUtc)) {
      // Only adjust if off by >= 1 s
      rtc.refresh();
      struct tm cur{};
      cur.tm_year = rtc.year() - 1900;
      cur.tm_mon  = rtc.month() - 1;
      cur.tm_mday = rtc.day();
      cur.tm_hour = rtc.hour();
      cur.tm_min  = rtc.minute();
      cur.tm_sec  = rtc.second();
      // Treat that local timestamp "as if" it were UTC, then shift by your TZ offset to get real UTC
uint32_t currentAsIfUtc = unixFromYMDHMS(cur.tm_year, cur.tm_mon, cur.tm_mday, cur.tm_hour,cur.tm_min, cur.tm_sec);
int64_t currentUtc = (int64_t)currentAsIfUtc - TZ_OFFSET_SECONDS; // TZ_OFFSET_SECONDS = -5*3600 for Bogotá
      int64_t delta = (int64_t)unixUtc - currentUtc;
      if (llabs(delta) >= 1) setRTCFromUnix(unixUtc, TZ_OFFSET_SECONDS);
      ok = true;
    }
  }
  if (ok){

    Serial.println("\nRTC time updated succesfully!");
  }else {
    Serial.println("\nRTC time update failed!");
  }
  wifiPowerOff();             // always turn radio off when done
  if (ok) lastSyncMillis = millis();
  return ok;
}

// ------------------- setup/loop -------------------
void setup() {
  Serial.begin(9600);
  Wire1.setSDA(14);
  Wire1.setSCL(15);
  Wire1.begin();

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setFreeFont(FMB12);
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.drawString("Date:  ", 0, DATE_Y);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString("Temp:  ", 0, TEMP_Y);

  // Optional: first sync at boot, then radio goes off
  doOneNtpSync();
}

void loop() {
  rtc.refresh();
  dateUpdate();
  timeUpdate();
  tempUpdate();

  // Periodic, radio-off cadence
  if (millis() - lastSyncMillis >= SYNC_INTERVAL_MS) {
    doOneNtpSync();     // best-effort; if it fails, RTC free-runs until next try
  }

  delay(1000);
}