#include "Arduino.h"

// RTC DS3231 libraries
#include "Wire.h"
#include "uRTCLib.h"

// TFT display IL9341 libraries
#include "Examples/resources/Free_Fonts.h"
#include <SPI.h>
#include <TFT_eSPI.h>

// ---- WiFi/NTP ----
#include <WiFi.h>
#include <WiFiUdp.h>
#include <time.h>

// ---------- Your WiFi ----------
const char *WIFI_SSID = "Perceptron";
const char *WIFI_PASS = "f1l0s0f14";

// ---------- NTP config ----------
const char *NTP_SERVER = "pool.ntp.org";
const uint16_t NTP_PORT = 123;
const uint32_t NTP_TIMEOUT_MS = 1500;

// Bogotá is UTC-5, no DST.
const int32_t TZ_OFFSET_SECONDS = -5 * 3600;

// Resync cadence: 6 hours
const uint32_t SYNC_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;

WiFiUDP ntpUDP;
uint32_t lastSyncMillis = 0;

// Create tft object
TFT_eSPI tft = TFT_eSPI();

// uRTCLib rtc; Address to read and write time
uRTCLib rtc(0x68);

// Previous date variables
uint8_t last_day = 99, last_month = 0, last_year = 0;
// Previous time variables
uint8_t last_hour = 99, last_minute = 99, last_second = 99;
// Previous temperature variable
float last_temp = -100.0;

// Y location of data (date, time and temperature) in display
const int DATE_Y = 45;
const int TIME_Y = 108;
const int TEMP_Y = 180;
const int UPDATE_Y=252;

// Number of updates
uint32_t updates = 0;
// ------------------- UI helpers -------------------

// Erases section with a black rectangle and writes new text
void clearAndDrawText(String text, int x, int y, int w, int h,
                      uint16_t textColor, uint16_t bgColor) {
  tft.fillRect(x, y, w, h, bgColor);
  tft.setTextColor(textColor, bgColor);
  tft.drawString(text, x, y);
}

// Updates date in display
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

// Updates time in display
void timeUpdate() {
  int xpos = 0;
  uint8_t hour = rtc.hour();
  uint8_t minute = rtc.minute();
  uint8_t second = rtc.second();

  tft.setFreeFont(FMB18);
  char hourBuffer[3];
  sprintf(hourBuffer, "%02d", hour);
  if (hour != last_hour) {
    clearAndDrawText(hourBuffer, xpos, TIME_Y, 44, 22, TFT_PINK, TFT_BLACK);
    last_hour = hour;
  }
  xpos += tft.textWidth(hourBuffer);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  xpos += tft.drawChar(':', xpos, TIME_Y + 18);

  char minuteBuffer[3];
  sprintf(minuteBuffer, "%02d", minute);
  if (minute != last_minute) {
    clearAndDrawText(minuteBuffer, xpos, TIME_Y, 44, 22, TFT_SKYBLUE,
                     TFT_BLACK);
    last_minute = minute;
  }

  xpos += tft.textWidth(minuteBuffer);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  xpos += tft.drawChar(':', xpos, TIME_Y + 18);
  tft.setFreeFont(FMB12);

  char secondBuffer[3];
  sprintf(secondBuffer, "%02d", second);
  if (second != last_second) {
    clearAndDrawText(secondBuffer, xpos, TIME_Y, 32, 16, TFT_MAGENTA,
                     TFT_BLACK);
    last_second = second;
  }
}

// Updates temperature in display
void tempUpdate() {
  float temp = rtc.temp() / 100.0;
  if (temp != last_temp) {
    char tempBuffer[5];
    sprintf(tempBuffer, "%.2f", temp);

    clearAndDrawText(tempBuffer, tft.textWidth("Temp:  "), TEMP_Y, 56, 14,
                     TFT_GREENYELLOW, TFT_BLACK);

    int16_t x = tft.textWidth("Temp:  ") + tft.textWidth(tempBuffer) + 4;
    int16_t y = TEMP_Y + 4;
    tft.fillCircle(x, y, 2, TFT_YELLOW);
    tft.drawString("C", x + 4, TEMP_Y);
    last_temp = temp;
  }
}

// Updates time since las update in display
void timeSinceUpdate(long time, uint32_t updates) {
  float minutes = (time/60000.0);
  uint8_t hours = minutes / 60;
  minutes = minutes - hours*60.0;

    char timeBuffer[8];
    sprintf(timeBuffer, "%02d:%.2f", hours, minutes );

  clearAndDrawText(timeBuffer, tft.textWidth("LastUpd: "), UPDATE_Y, 112, 14,
                     TFT_GREENYELLOW, TFT_BLACK);
    char updatesBuffer[3];
    sprintf(updatesBuffer, "%02d", updates);
  clearAndDrawText(updatesBuffer, tft.textWidth("Updates: "), UPDATE_Y+25, 42, 14,
                     TFT_GREENYELLOW, TFT_BLACK);
}
// ------------------- WiFi/NTP helpers -------------------
static void wifiPowerOff() {
  Serial.println("\nTurning off Wifi...");
  ntpUDP.stop();         // close any UDP sockets
  WiFi.disconnect(true); // drop connection and clear config
  WiFi.end();            // deinit driver (frees memory, turns radio off)
#ifdef WIFI_OFF
  WiFi.mode(WIFI_OFF); // if the core defines it, be explicit
#endif
  Serial.println("\nWifi is off...");
}

static bool wifiPowerOnAndConnect(uint32_t timeoutMs = 15000) {
  Serial.println("\nConnecting to Wifi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(200);
  }
  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("\nWifi connection succesful");
  } else {

    Serial.println("\nWifi connection failed");
  }
  return WiFi.status() == WL_CONNECTED;
}

static bool getUnixTimeFromNTP(uint32_t &unixTimeOut) {
  uint8_t pkt[48] = {0};
  pkt[0] = 0x1B; // LI=0, VN=3, Mode=3 (client)

  if (!ntpUDP.begin(0))
    return false;
  if (ntpUDP.beginPacket(NTP_SERVER, NTP_PORT) != 1)
    return false;
  ntpUDP.write(pkt, sizeof(pkt));
  if (ntpUDP.endPacket() != 1)
    return false;

  uint32_t start = millis();
  while (millis() - start < NTP_TIMEOUT_MS) {
    int size = ntpUDP.parsePacket();
    if (size >= 48) {
      ntpUDP.read(pkt, sizeof(pkt));
      ntpUDP.stop();

      uint32_t secsSince1900 = ((uint32_t)pkt[40] << 24) |
                               ((uint32_t)pkt[41] << 16) |
                               ((uint32_t)pkt[42] << 8) | ((uint32_t)pkt[43]);

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
  struct tm *tm_p = gmtime(&t);
  if (!tm_p)
    return;
  uint8_t dow = (uint8_t)tm_p->tm_wday + 1; // dS3231 wants 1..7, Sun=1
  uint8_t y2 = (tm_p->tm_year + 1900) % 100;

  rtc.set((uint8_t)tm_p->tm_sec, (uint8_t)tm_p->tm_min, (uint8_t)tm_p->tm_hour,
          dow, (uint8_t)tm_p->tm_mday, (uint8_t)tm_p->tm_mon + 1, y2);
}

// Convert a civil date-time (UTC) to Unix time (seconds since 1970-01-01).
// Works for years in a wide range; returns uint32_t (good through year 2106).
static uint32_t unixFromYMDHMS(int year, int mon, int day, int hour, int min,
                               int sec) {
  // Howard Hinnant’s days-from-civil algorithm (condensed, no DST, no TZ).
  // Month 1..12, Day 1..31
  year -= mon <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = (unsigned)(year - era * 400); // [0, 399]
  const unsigned doy =
      (153u * (mon + (mon > 2 ? -3 : 9)) + 2) / 5 + day - 1;  // [0, 365]
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy; // [0, 146096]
  const int days = era * 146097 + (int)doe - 719468; // days since 1970-01-01
  return (uint32_t)days * 86400u + (uint32_t)hour * 3600u +
         (uint32_t)min * 60u + (uint32_t)sec;
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
      struct tm cur {};
      cur.tm_year = rtc.year() - 1900;
      cur.tm_mon = rtc.month() - 1;
      cur.tm_mday = rtc.day();
      cur.tm_hour = rtc.hour();
      cur.tm_min = rtc.minute();
      cur.tm_sec = rtc.second();
      // Treat that local timestamp "as if" it were UTC, then shift by your TZ
      // offset to get real UTC
      uint32_t currentAsIfUtc =
          unixFromYMDHMS(cur.tm_year, cur.tm_mon, cur.tm_mday, cur.tm_hour,
                         cur.tm_min, cur.tm_sec);
      int64_t currentUtc =
          (int64_t)currentAsIfUtc -
          TZ_OFFSET_SECONDS; // TZ_OFFSET_SECONDS = -5*3600 for Bogotá
      int64_t delta = (int64_t)unixUtc - currentUtc;
      if (llabs(delta) >= 1)
        setRTCFromUnix(unixUtc, TZ_OFFSET_SECONDS);
        updates += 1;
      ok = true;
    }
  }
  if (ok) {

    Serial.println("\nRTC time updated succesfully!");
  } else {
    Serial.println("\nRTC time update failed!");
  }
  wifiPowerOff(); // always turn radio off when done
  if (ok)
    lastSyncMillis = millis();
  return ok;
}

// ------------------- setup/loop -------------------
void setup() {
  Serial.begin(115200);
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
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("Temp:  ", 0, TEMP_Y);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString("LastUpd:  ", 0, UPDATE_Y);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.drawString("Updates:  ", 0, UPDATE_Y+25);

  // Optional: first sync at boot, then radio goes off
  doOneNtpSync();
}

void loop() {
  rtc.refresh();
  unsigned long timeSinceLastUpdate =millis() - lastSyncMillis;
  // Periodic, radio-off cadence
  if ( timeSinceLastUpdate>= SYNC_INTERVAL_MS) {
    doOneNtpSync(); // best-effort; if it fails, RTC free-runs until next try
  }
  dateUpdate();
  timeUpdate();
  tempUpdate();
  timeSinceUpdate(timeSinceLastUpdate, updates);

  delay(1000);
}