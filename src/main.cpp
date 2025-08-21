#include "Arduino.h"

// ====== Single-core, event-driven state machine (Pico W) ======
// UI + I2C run every loop; Wi‑Fi/NTP is non‑blocking via states.
// Adds robust DNS resolution, multiple NTP fallbacks, and HTTP fallback
// (for networks that block UDP/123). Verbose serial logs at each step.

// ====== RTC DS3231 ======
#define URTCLIB_WIRE Wire1  // bind uRTCLib to Wire1
#include <Wire.h>
#include <uRTCLib.h>

// ====== TFT ILI9341 ======
#include "Examples/resources/Free_Fonts.h"
#include <SPI.h>
#include <TFT_eSPI.h>

// ====== WiFi/NTP (Pico W) ======
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <time.h>
#include "secrets.h"

// ---------- NTP config ----------
static const uint16_t NTP_PORT = 123;
static const uint32_t NTP_TIMEOUT_MS = 1500;     // total wait after send
static const uint16_t NTP_LOCAL_PORT = 2390;     // local UDP port
static const char *NTP_HOSTS[] = {
  "pool.ntp.org",
  "time.google.com",
  "time.cloudflare.com",
  "time.nist.gov"
};
static const size_t NTP_HOST_COUNT = sizeof(NTP_HOSTS)/sizeof(NTP_HOSTS[0]);

// HTTP fallback (for UDP/123-blocked networks)
static const char *HTTP_TIME_HOST = "worldtimeapi.org"; // supports plain HTTP
static const char *HTTP_TIME_PATH = "/api/timezone/America/Bogota.txt"; // returns lines with "unixtime: <n>"
static const uint16_t HTTP_PORT = 80;
static const uint32_t HTTP_TIMEOUT_MS = 2500;

// Bogotá is UTC-5, no DST.
static const int32_t TZ_OFFSET_SECONDS = -5 * 3600;

// Resync cadence and warmup
static const uint32_t SYNC_INTERVAL_MS = 2UL * 60UL * 1000UL;
#ifndef SYNC_WARMUP_MS
#define SYNC_WARMUP_MS 10000UL  // 10s warm-up is usually enough
#endif

// Wi‑Fi connection timeout
#define WIFI_CONNECT_TIMEOUT_MS 12000UL

// ================= Globals =================
WiFiUDP ntpUDP;
TFT_eSPI tft = TFT_eSPI();
uRTCLib rtc(0x68);

uint32_t lastSyncMillis = 0;   // when RTC was last synced
uint32_t nextSyncAtMillis = 0; // when the next sync should trigger

// Previous date/time/temp for dirty updates
uint8_t last_day = 99, last_month = 0, last_year = 0;
uint8_t last_hour = 99, last_minute = 99, last_second = 99;
float last_temp = -100.0;
uint8_t last_updated_hour = 99;
uint8_t last_updated_minute = 99;
uint8_t last_updated_second = 99;
uint8_t updates = 0;
uint8_t last_updates = 100;
// UI layout constants
const int DATE_Y = 45; const int TIME_Y = 108; const int TEMP_Y = 180; const int UPDATE_Y = 252;

// ================= UI helpers =================
static void clearAndDrawText(String text, int x, int y, int w, int h,
                             uint16_t textColor, uint16_t bgColor) {
  tft.fillRect(x, y, w, h, bgColor);
  tft.setTextColor(textColor, bgColor);
  tft.drawString(text, x, y);
}

static void dateUpdate() {
  uint8_t day = rtc.day();
  uint8_t month = rtc.month();
  uint8_t year = rtc.year();
  if (day != last_day || month != last_month || year != last_year) {
    char dateBuffer[12];
    sprintf(dateBuffer, "%02d/%02d/%d", day, month, year);
    tft.setFreeFont(FMB12);
    clearAndDrawText(dateBuffer, tft.textWidth("Date:  "), DATE_Y, 112, 16,
                     TFT_YELLOW, TFT_BLACK);
    last_day = day; last_month = month; last_year = year;
  }
}

static void timeUpdate() {
  int xpos = 0;
  uint8_t hour = rtc.hour();
  uint8_t minute = rtc.minute();
  uint8_t second = rtc.second();
  tft.setFreeFont(FMB18);
  char hourBuffer[3]; sprintf(hourBuffer, "%02d", hour);
  if (hour != last_hour) { clearAndDrawText(hourBuffer, xpos, TIME_Y, 44, 22, TFT_PINK, TFT_BLACK); last_hour = hour; }
  xpos += tft.textWidth(hourBuffer);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); xpos += tft.drawChar(':', xpos, TIME_Y + 18);
  char minuteBuffer[3]; sprintf(minuteBuffer, "%02d", minute);
  if (minute != last_minute) { clearAndDrawText(minuteBuffer, xpos, TIME_Y, 44, 22, TFT_SKYBLUE, TFT_BLACK); last_minute = minute; }
  xpos += tft.textWidth(minuteBuffer);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); xpos += tft.drawChar(':', xpos, TIME_Y + 18);
  tft.setFreeFont(FMB12);
  char secondBuffer[3]; sprintf(secondBuffer, "%02d", second);
  if (second != last_second) { clearAndDrawText(secondBuffer, xpos, TIME_Y, 32, 16, TFT_MAGENTA, TFT_BLACK); last_second = second; }
}

static void tempUpdate() {
  float temp = rtc.temp() / 100.0;
  if (temp != last_temp) {
    char tempBuffer[16]; dtostrf(temp, 0, 2, tempBuffer);
    clearAndDrawText(tempBuffer, tft.textWidth("Temp:  "), TEMP_Y, 56, 14,
                     TFT_GREENYELLOW, TFT_BLACK);
    int16_t x = tft.textWidth("Temp:  ") + tft.textWidth(tempBuffer) + 4;
    int16_t y = TEMP_Y + 4; tft.fillCircle(x, y, 2, TFT_YELLOW); tft.drawString("C", x + 4, TEMP_Y);
    last_temp = temp;
  }
}

static void timeSinceUpdate(long elapsedMs, uint32_t upds) {
  float time = (elapsedMs / 60000.0f);
  uint8_t hours = time / 60; 
  uint8_t minutes = int(time - hours* 60);
  uint8_t seconds = int((time- hours*60 - minutes)*60);
  if (last_updated_hour != hours || last_updated_minute != minutes || last_updated_second != seconds){
  char timeBuffer[12]; 
  sprintf(timeBuffer, "%02d:%02d:%02d", hours, minutes,seconds);
  clearAndDrawText(timeBuffer, tft.textWidth("SinceUpd: "), UPDATE_Y, 126, 14, TFT_GREENYELLOW, TFT_BLACK);
    last_updated_hour = hours;
    last_updated_minute = minutes;
    last_updated_second = seconds;
  }
 
  if (last_updates != updates){
  char updatesBuffer[8]; 
  sprintf(updatesBuffer, "%lu", (unsigned long)upds);
  clearAndDrawText(updatesBuffer, tft.textWidth("Update #: "), UPDATE_Y + 35, 42, 14, TFT_GREENYELLOW, TFT_BLACK);
    last_updates = updates;
  }
}

// ================= Wi‑Fi/NTP helpers =================
static void wifiPowerOff() {
  Serial.println("[wifi] Turning off...");
  ntpUDP.stop();
  WiFi.disconnect(true);
  WiFi.end();
#ifdef WIFI_OFF
  WiFi.mode(WIFI_OFF);
#endif
  Serial.println("[wifi] Off");
}

static void wifiPowerOn() {
  Serial.println("[wifi] Powering on + STA mode");
  WiFi.persistent(false);  // don't write FLASH
  WiFi.mode(WIFI_STA);
}

static void wifiBegin() {
  Serial.print("[wifi] begin() SSID="); 
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

static bool resolveNtp(IPAddress &outIp, size_t hostIndex) {
  const char *host = NTP_HOSTS[hostIndex % NTP_HOST_COUNT];
  Serial.print("[dns] Resolving "); Serial.println(host);
  if (WiFi.hostByName(host, outIp) == 1) {
    Serial.print("[dns] "); Serial.print(host); Serial.print(" -> "); Serial.println(outIp);
    return true;
  }
  Serial.println("[dns] DNS failed");
  return false;
}

static bool ntpSend(IPAddress serverIp, uint32_t &sendStampMs) {
  uint8_t pkt[48] = {0}; pkt[0] = 0x1B; // LI=0, VN=3, Mode=3 (client)
  if (!ntpUDP.begin(NTP_LOCAL_PORT)) { Serial.println("[ntp] UDP begin failed"); return false; }
  if (ntpUDP.beginPacket(serverIp, NTP_PORT) != 1) { Serial.println("[ntp] beginPacket failed"); return false; }
  ntpUDP.write(pkt, sizeof(pkt));
  if (ntpUDP.endPacket() != 1) { Serial.println("[ntp] endPacket failed"); return false; }
  sendStampMs = millis(); Serial.print("[ntp] request sent to "); Serial.println(serverIp);
  return true;
}

static bool ntpTryReceive(uint32_t &unixOut) {
  uint8_t pkt[48]; int sz = ntpUDP.parsePacket();
  if (sz >= 48) {
    ntpUDP.read(pkt, sizeof(pkt)); ntpUDP.stop();
    uint32_t secsSince1900 = ((uint32_t)pkt[40] << 24) | ((uint32_t)pkt[41] << 16) | ((uint32_t)pkt[42] << 8) | (uint32_t)pkt[43];
    const uint32_t NTP_UNIX_DELTA = 2208988800UL; unixOut = secsSince1900 - NTP_UNIX_DELTA;
    Serial.print("[ntp] rx OK unix="); Serial.println(unixOut);
    return true;
  }
  return false;
}

static bool httpGetUnixTime(uint32_t &unixOut) {
  WiFiClient client; client.setTimeout(HTTP_TIMEOUT_MS);
  Serial.print("[http] GET http://"); Serial.print(HTTP_TIME_HOST); Serial.println(HTTP_TIME_PATH);
  if (!client.connect(HTTP_TIME_HOST, HTTP_PORT)) { Serial.println("[http] connect failed"); return false; }
  client.print(String("GET ") + HTTP_TIME_PATH + " HTTP/1.1 Host: " + HTTP_TIME_HOST + "Connection: close");

  // Skip headers
  if (!client.find("")) { Serial.println("[http] no header end"); 
  return false; }
  // Read body and scan for: "unixtime: <number>"
  while (client.connected() || client.available()) {
    String line = client.readStringUntil(' ');
    int idx = line.indexOf("unixtime:");
    if (idx >= 0) {
      idx += 9; // past label
      while (idx < (int)line.length() && (line[idx] == ' ' || line[idx] == '	')) idx++;
      String num = "";
      while (idx < (int)line.length() && isDigit(line[idx])) { num += line[idx++]; }
      if (num.length() > 0) {
        unixOut = (uint32_t)strtoul(num.c_str(), nullptr, 10);
        Serial.print("[http] unixtime="); Serial.println(unixOut);
        return true;
      }
    }
  }
  Serial.println("[http] unixtime not found");
  return false;
}

static void setRTCFromUnix(uint32_t unixTimeUtc, int32_t tzOffsetSeconds) {
  time_t t = (time_t)((int64_t)unixTimeUtc + tzOffsetSeconds);
  struct tm *tm_p = gmtime(&t); if (!tm_p) return;
  uint8_t dow = (uint8_t)tm_p->tm_wday + 1;  // DS3231 1..7, Sun=1
  uint8_t y2 = (tm_p->tm_year + 1900) % 100;
  rtc.set((uint8_t)tm_p->tm_sec, (uint8_t)tm_p->tm_min, (uint8_t)tm_p->tm_hour,
          dow, (uint8_t)tm_p->tm_mday, (uint8_t)tm_p->tm_mon + 1, y2);
}


// ================= State machine =================
enum SyncState : uint8_t {
  ST_IDLE = 0,           // UI only
  ST_WIFI_WARMUP,        // set STA mode, start connect
  ST_WIFI_WAIT_CONN,     // poll WiFi.status until connected or timeout
  ST_NTP_RESOLVE,        // DNS resolve host -> IPv4
  ST_NTP_SEND,           // open UDP + send request
  ST_NTP_WAIT,           // wait for reply or timeout
  ST_HTTP_FALLBACK,      // try HTTP time API (TCP/80) as last resort
  ST_APPLY_TIME,         // write DS3231
  ST_WIFI_SHUTDOWN       // turn radio off
};

static SyncState st = ST_IDLE;
static uint32_t stSince = 0;           // when current state started
static uint32_t ntpSendAt = 0;         // when NTP packet was sent
static size_t ntpHostIndex = 0;         // which NTP host we're trying
static IPAddress ntpServerIP;

static void smEnter(SyncState s) { st = s; stSince = millis(); }

static void scheduleNextSyncFromNow() {
  lastSyncMillis = millis();
  nextSyncAtMillis = lastSyncMillis + SYNC_INTERVAL_MS;
}

static bool syncWindowOpen() {
  uint32_t now = millis();
  if (now >= nextSyncAtMillis) return true; // overdue
  uint32_t remaining = nextSyncAtMillis - now;
  return remaining <= SYNC_WARMUP_MS;
}

void setup() {
  Serial.begin(115200);
  Serial.println("[boot] Pico W RTC/NTP state machine");

  // I2C on Wire1 (RTC)
  Wire1.setSDA(14); Wire1.setSCL(15); Wire1.begin();

  // TFT init
  tft.init(); tft.setRotation(0); tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1); tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setFreeFont(FMB12);
  tft.setTextColor(TFT_BLUE, TFT_BLACK); tft.drawString("Date:  ", 0, DATE_Y);
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.drawString("Temp:  ", 0, TEMP_Y);
  tft.setTextColor(TFT_RED, TFT_BLACK); tft.drawString("SinceUpd:  ", 0, UPDATE_Y);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK); tft.drawString("Update #:  ", 0, UPDATE_Y + 35);

  // First sync immediately at boot (no warm-up delay)
  nextSyncAtMillis = millis();
  smEnter(ST_WIFI_WARMUP);
}

void loop() {
  // Always keep UI responsive
  rtc.refresh();
  dateUpdate(); timeUpdate(); tempUpdate();
  timeSinceUpdate(millis() - lastSyncMillis, updates);

  // ---- State machine tick ----
  const uint32_t now = millis();
  switch (st) {
    case ST_IDLE: {
      if (syncWindowOpen()) smEnter(ST_WIFI_WARMUP);
      break;
    }
    case ST_WIFI_WARMUP: {
      ntpHostIndex = 0;
      wifiPowerOn();
      wifiBegin();
      smEnter(ST_WIFI_WAIT_CONN);
      break;
    }
    case ST_WIFI_WAIT_CONN: {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("[wifi] connected IP="); 
        Serial.println(WiFi.localIP());
        Serial.print("[wifi] DNS="); 
        Serial.println(WiFi.dnsIP());
        smEnter(ST_NTP_RESOLVE);
      } else if (now - stSince > WIFI_CONNECT_TIMEOUT_MS) {
        Serial.println("[wifi] connect timeout");
        smEnter(ST_WIFI_SHUTDOWN);
      }
      break;
    }
    case ST_NTP_RESOLVE: {
      if (resolveNtp(ntpServerIP, ntpHostIndex)) {
        smEnter(ST_NTP_SEND);
      } else {
        // try next host or bail to HTTP fallback
        ntpHostIndex = (ntpHostIndex + 1) % NTP_HOST_COUNT;
        if (ntpHostIndex == 0) smEnter(ST_HTTP_FALLBACK);
      }
      break;
    }
    case ST_NTP_SEND: {
      if (ntpSend(ntpServerIP, ntpSendAt)) {
        smEnter(ST_NTP_WAIT);
      } else {
        // UDP send failed; try next host or HTTP fallback
        ntpHostIndex = (ntpHostIndex + 1) % NTP_HOST_COUNT;
        smEnter((ntpHostIndex == 0) ? ST_HTTP_FALLBACK : ST_NTP_RESOLVE);
      }
      break;
    }
    case ST_NTP_WAIT: {
      uint32_t unixUtc;
      if (ntpTryReceive(unixUtc)) {
        setRTCFromUnix(unixUtc, TZ_OFFSET_SECONDS);
        updates++;
        scheduleNextSyncFromNow();
        Serial.println("[ntp] RTC updated");
        smEnter(ST_WIFI_SHUTDOWN);
      } else if (now - ntpSendAt > NTP_TIMEOUT_MS) {
        Serial.println("[ntp] timeout waiting reply -> HTTP fallback");
        smEnter(ST_HTTP_FALLBACK);
      }
      break;
    }
    case ST_HTTP_FALLBACK: {
      uint32_t unixUtc;
      if (httpGetUnixTime(unixUtc)) {
        setRTCFromUnix(unixUtc, TZ_OFFSET_SECONDS);
        updates++;
        scheduleNextSyncFromNow();
        Serial.println("[http] RTC updated");
      } else {
        Serial.println("[http] failed to get time");
        // failed this round; try again in a minute instead of 6 hours
        nextSyncAtMillis = millis() + 60000UL;
      }
      smEnter(ST_WIFI_SHUTDOWN);
      break;
    }
    case ST_APPLY_TIME: { // unused
      smEnter(ST_WIFI_SHUTDOWN);
      break;
    }
    case ST_WIFI_SHUTDOWN: {
      wifiPowerOff();
      smEnter(ST_IDLE);
      break;
    }
  }

  // Ensure Wi‑Fi stays off outside the warmup window
  if (st == ST_IDLE && !syncWindowOpen()) {
    if (WiFi.status() == WL_CONNECTED) wifiPowerOff();
  }

  delay(20); // keep lwIP timers happy without freezing UI
}