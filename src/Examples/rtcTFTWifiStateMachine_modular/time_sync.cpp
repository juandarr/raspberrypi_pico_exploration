// Wi‑Fi/NTP + HTTP fallback time synchronization state machine

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <time.h>

#include "secrets.h"
#include <uRTCLib.h>

// Internal configuration (copied from original main.cpp)
namespace cfg {
  constexpr uint16_t NTP_PORT = 123;
  constexpr uint32_t NTP_TIMEOUT_MS = 1500; // total wait after send
  constexpr uint16_t NTP_LOCAL_PORT = 2390; // local UDP port
  constexpr const char *NTP_HOSTS[] = {
      "pool.ntp.org", "time.google.com", "time.cloudflare.com", "time.nist.gov"};
  constexpr size_t NTP_HOST_COUNT = sizeof(NTP_HOSTS) / sizeof(NTP_HOSTS[0]);

  // HTTP fallback (for UDP/123-blocked networks)
  constexpr const char *HTTP_TIME_HOST = "worldtimeapi.org";
  constexpr const char *HTTP_TIME_PATH = "/api/timezone/America/Bogota.txt"; // returns lines with "unixtime: <n>"
  constexpr uint16_t HTTP_PORT = 80;
  constexpr uint32_t HTTP_TIMEOUT_MS = 2500;

  // Bogotá is UTC-5, no DST.
  constexpr int32_t TZ_OFFSET_SECONDS = -5 * 3600;

  // Resync cadence and warmup - Every 3 hours
  constexpr uint32_t SYNC_INTERVAL_MS = 3UL * 60UL * 60UL * 1000UL;
#ifndef SYNC_WARMUP_MS
#define SYNC_WARMUP_MS 10000UL // 10s warm-up is usually enough
#endif
  constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 12000UL;
}

namespace {
  // RTC binding
  uRTCLib *g_rtc = nullptr;

  // UDP socket
  WiFiUDP ntpUDP;

  // Scheduler
  uint32_t lastSyncMillis = 0;   // when RTC was last synced
  uint32_t nextSyncAtMillis = 0; // when the next sync should trigger

  // Counters
  uint32_t updates = 0;

  // State machine
  enum class SyncState : uint8_t {
    IDLE = 0,        // UI only
    WIFI_WARMUP,     // set STA mode, start connect
    WIFI_WAIT_CONN,  // poll WiFi.status until connected or timeout
    NTP_RESOLVE,     // DNS resolve host -> IPv4
    NTP_SEND,        // open UDP + send request
    NTP_WAIT,        // wait for reply or timeout
    HTTP_FALLBACK,   // try HTTP time API (TCP/80) as last resort
    WIFI_SHUTDOWN    // turn radio off
  };

  SyncState st = SyncState::IDLE;
  uint32_t stSince = 0;    // when current state started
  uint32_t ntpSendAt = 0;  // when NTP packet was sent
  size_t ntpHostIndex = 0; // which NTP host we're trying
  IPAddress ntpServerIP;

  void smEnter(SyncState s) {
    st = s;
    stSince = millis();
  }

  void scheduleNextSyncFromNow() {
    lastSyncMillis = millis();
    nextSyncAtMillis = lastSyncMillis + cfg::SYNC_INTERVAL_MS;
  }

  bool syncWindowOpen() {
    uint32_t now = millis();
    if ((int32_t)(now - nextSyncAtMillis) >= 0)
      return true; // overdue
    uint32_t remaining = nextSyncAtMillis - now;
    return remaining <= SYNC_WARMUP_MS;
  }

  void wifiPowerOff() {
    Serial.println("[wifi] Turning off...");
    ntpUDP.stop();
    WiFi.disconnect(true);
    WiFi.end();
#ifdef WIFI_OFF
    WiFi.mode(WIFI_OFF);
#endif
    Serial.println("[wifi] Off");
  }

  void wifiPowerOn() {
    Serial.println("[wifi] Powering on + STA mode");
    WiFi.persistent(false); // don't write FLASH
    WiFi.mode(WIFI_STA);
  }

  void wifiBegin() {
    Serial.print("[wifi] begin() SSID=");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }

  bool resolveNtp(IPAddress &outIp, size_t hostIndex) {
    const char *host = cfg::NTP_HOSTS[hostIndex % cfg::NTP_HOST_COUNT];
    Serial.print("[dns] Resolving ");
    Serial.println(host);
    if (WiFi.hostByName(host, outIp) == 1) {
      Serial.print("[dns] ");
      Serial.print(host);
      Serial.print(" -> ");
      Serial.println(outIp);
      return true;
    }
    Serial.println("[dns] DNS failed");
    return false;
  }

  bool ntpSend(IPAddress serverIp, uint32_t &sendStampMs) {
    uint8_t pkt[48] = {0};
    pkt[0] = 0x1B; // LI=0, VN=3, Mode=3 (client)
    if (!ntpUDP.begin(cfg::NTP_LOCAL_PORT)) {
      Serial.println("[ntp] UDP begin failed");
      return false;
    }
    if (ntpUDP.beginPacket(serverIp, cfg::NTP_PORT) != 1) {
      Serial.println("[ntp] beginPacket failed");
      return false;
    }
    ntpUDP.write(pkt, sizeof(pkt));
    if (ntpUDP.endPacket() != 1) {
      Serial.println("[ntp] endPacket failed");
      return false;
    }
    sendStampMs = millis();
    Serial.print("[ntp] request sent to ");
    Serial.println(serverIp);
    return true;
  }

  bool ntpTryReceive(uint32_t &unixOut, uint32_t sendStampMs) {
    uint8_t pkt[48];
    int sz = ntpUDP.parsePacket();
    if (sz >= 48) {
      uint32_t arrivalStampMs = millis();
      ntpUDP.read(pkt, sizeof(pkt));
      ntpUDP.stop();

      uint32_t secsSince1900 = ((uint32_t)pkt[40] << 24) | ((uint32_t)pkt[41] << 16) |
                                ((uint32_t)pkt[42] << 8) | (uint32_t)pkt[43];

      uint32_t roundTripMs = arrivalStampMs - sendStampMs;
      uint32_t correctionMs = roundTripMs / 2;

      const uint32_t NTP_UNIX_DELTA = 2208988800UL;
      uint32_t serverUnixTime = secsSince1900 - NTP_UNIX_DELTA;

      // Add the correction, rounding to the nearest second using integer math
      unixOut = serverUnixTime + (uint32_t)(correctionMs / 1000);
      if ((correctionMs % 1000) >= 500) {
        unixOut++;
      }

      Serial.print("[ntp] rx OK. round trip=");
      Serial.print(roundTripMs);
      Serial.println("ms");
      Serial.print("[ntp] server unix=");
      Serial.println(serverUnixTime);
      Serial.print("[ntp] correction=");
      Serial.print(correctionMs);
      Serial.println("ms");
      Serial.print("[ntp] corrected unix=");
      Serial.println(unixOut);

      return true;
    }
    return false;
  }

  bool httpGetUnixTime(uint32_t &unixOut) {
    WiFiClient client;
    client.setTimeout(cfg::HTTP_TIMEOUT_MS);
    Serial.print("[http] GET http://");
    Serial.print(cfg::HTTP_TIME_HOST);
    Serial.println(cfg::HTTP_TIME_PATH);
    if (!client.connect(cfg::HTTP_TIME_HOST, cfg::HTTP_PORT)) {
      Serial.println("[http] connect failed");
      return false;
    }

    // Send a standards-compliant HTTP GET request
    client.print(String("GET ") + cfg::HTTP_TIME_PATH + " HTTP/1.1\r\n" +
                 "Host: " + cfg::HTTP_TIME_HOST + "\r\n" +
                 "Connection: close\r\n\r\n");

    // Find the end of the headers
    if (!client.find("\r\n\r\n")) {
      Serial.println("[http] invalid response");
      client.stop();
      return false;
    }

    // Find the line containing "unixtime:" and parse the number
    if (client.find("unixtime: ")) {
      String numStr = client.readStringUntil('\n');
      unixOut = (uint32_t)strtoul(numStr.c_str(), nullptr, 10);
      Serial.print("[http] unixtime=");
      Serial.println(unixOut);
      client.stop();
      return true;
    }

    Serial.println("[http] unixtime not found");
    client.stop();
    return false;
  }

  void setRTCFromUnix(uint32_t unixTimeUtc, int32_t tzOffsetSeconds) {
    if (!g_rtc) return;
    time_t t = (time_t)((int64_t)unixTimeUtc + tzOffsetSeconds);
    struct tm *tm_p = gmtime(&t);
    if (!tm_p) return;
    uint8_t dow = (uint8_t)tm_p->tm_wday + 1; // DS3231 1..7, Sun=1
    uint8_t y2 = (tm_p->tm_year) % 100;
    g_rtc->set((uint8_t)tm_p->tm_sec, (uint8_t)tm_p->tm_min, (uint8_t)tm_p->tm_hour,
               dow, (uint8_t)tm_p->tm_mday, (uint8_t)tm_p->tm_mon + 1, y2);
  }
}

void timeSyncSetup(uRTCLib &rtc) {
  g_rtc = &rtc;
  // Initial schedule: next sync at some point; we'll kick immediately when asked
  nextSyncAtMillis = millis();
}

void timeSyncStart() {
  // First sync immediately at boot (no warm-up delay)
  nextSyncAtMillis = millis();
  // Enter warmup to start Wi‑Fi connection
  smEnter(SyncState::WIFI_WARMUP);
}

void timeSyncTick() {
  const uint32_t now = millis();
  switch (st) {
  case SyncState::IDLE: {
    if (syncWindowOpen()) {
      smEnter(SyncState::WIFI_WARMUP);
    }
    break;
  }
  case SyncState::WIFI_WARMUP: {
    ntpHostIndex = 0;
    wifiPowerOn();
    wifiBegin();
    smEnter(SyncState::WIFI_WAIT_CONN);
    break;
  }
  case SyncState::WIFI_WAIT_CONN: {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("[wifi] connected IP=");
      Serial.println(WiFi.localIP());
      Serial.print("[wifi] DNS=");
      Serial.println(WiFi.dnsIP());
      smEnter(SyncState::NTP_RESOLVE);
    } else if ((now - stSince) > cfg::WIFI_CONNECT_TIMEOUT_MS) {
      Serial.println("[wifi] connect timeout");
      smEnter(SyncState::WIFI_SHUTDOWN);
    }
    break;
  }
  case SyncState::NTP_RESOLVE: {
    if (resolveNtp(ntpServerIP, ntpHostIndex)) {
      smEnter(SyncState::NTP_SEND);
    } else {
      // try next host or bail to HTTP fallback
      ntpHostIndex = (ntpHostIndex + 1) % cfg::NTP_HOST_COUNT;
      if (ntpHostIndex == 0)
        smEnter(SyncState::HTTP_FALLBACK);
    }
    break;
  }
  case SyncState::NTP_SEND: {
    if (ntpSend(ntpServerIP, ntpSendAt)) {
      smEnter(SyncState::NTP_WAIT);
    } else {
      // UDP send failed; try next host or HTTP fallback
      ntpHostIndex = (ntpHostIndex + 1) % cfg::NTP_HOST_COUNT;
      smEnter((ntpHostIndex == 0) ? SyncState::HTTP_FALLBACK : SyncState::NTP_RESOLVE);
    }
    break;
  }
  case SyncState::NTP_WAIT: {
    uint32_t unixUtc;
    if (ntpTryReceive(unixUtc, ntpSendAt)) {
      setRTCFromUnix(unixUtc, cfg::TZ_OFFSET_SECONDS);
      updates++;
      scheduleNextSyncFromNow();
      Serial.println("[ntp] RTC updated");
      smEnter(SyncState::WIFI_SHUTDOWN);
    } else if ((now - ntpSendAt) > cfg::NTP_TIMEOUT_MS) {
      Serial.println("[ntp] timeout, trying next server...");
      ntpUDP.stop();
      ntpHostIndex++;
      if (ntpHostIndex >= cfg::NTP_HOST_COUNT) {
        Serial.println("[ntp] All NTP hosts timed out -> HTTP fallback");
        smEnter(SyncState::HTTP_FALLBACK);
      } else {
        smEnter(SyncState::NTP_RESOLVE);
      }
    }
    break;
  }
  case SyncState::HTTP_FALLBACK: {
    uint32_t unixUtc;
    if (httpGetUnixTime(unixUtc)) {
      setRTCFromUnix(unixUtc, cfg::TZ_OFFSET_SECONDS);
      updates++;
      scheduleNextSyncFromNow();
      Serial.println("[http] RTC updated");
    } else {
      Serial.println("[http] failed to get time");
      // failed this round; try again in a minute instead of 3 hours
      nextSyncAtMillis = millis() + 60000UL;
    }
    smEnter(SyncState::WIFI_SHUTDOWN);
    break;
  }
  case SyncState::WIFI_SHUTDOWN: {
    wifiPowerOff();
    smEnter(SyncState::IDLE);
    break;
  }
  }

  // Ensure Wi‑Fi stays off outside the warmup window
  if (st == SyncState::IDLE && !syncWindowOpen()) {
    if (WiFi.status() == WL_CONNECTED)
      wifiPowerOff();
  }
}

uint32_t timeSyncLastSyncMillis() { return lastSyncMillis; }
uint32_t timeSyncUpdates() { return updates; }

