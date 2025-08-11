// Raspberry Pi Pico W + Arduino-Pico core
// Prints time from NTP to Serial Monitor

#include "Arduino.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <time.h>

const char* WIFI_SSID = "SSID";
const char* WIFI_PASS = "PASSWORD";

const char* NTP_SERVER = "pool.ntp.org";
const uint16_t NTP_PORT = 123;
const uint32_t NTP_TIMEOUT_MS = 1500;

// Bogotá (UTC-5), no DST. Change if you like living dangerously.
const int32_t TZ_OFFSET_SECONDS = -5 * 3600;

WiFiUDP udp;

// Simple Wi-Fi connect with timeout
bool wifiConnect(uint32_t overallTimeoutMs = 15000) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < overallTimeoutMs) {
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
}

// Query NTP and return Unix time (UTC seconds since 1970-01-01)
bool getUnixTimeFromNTP(uint32_t& unixTimeOut) {
  uint8_t pkt[48] = {0};
  pkt[0] = 0x1B; // LI=0, VN=3, Mode=3 (client)

  if (!udp.begin(0)) return false;

  if (udp.beginPacket(NTP_SERVER, NTP_PORT) != 1) return false;
  udp.write(pkt, sizeof(pkt));
  if (udp.endPacket() != 1) return false;

  uint32_t start = millis();
  while (millis() - start < NTP_TIMEOUT_MS) {
    int size = udp.parsePacket();
    if (size >= 48) {
      udp.read(pkt, sizeof(pkt));
      udp.stop();

      // Transmit Timestamp seconds at bytes 40..43
      uint32_t secsSince1900 =
          ((uint32_t)pkt[40] << 24) |
          ((uint32_t)pkt[41] << 16) |
          ((uint32_t)pkt[42] << 8)  |
          ((uint32_t)pkt[43]);

      const uint32_t NTP_UNIX_DELTA = 2208988800UL; // 1900->1970
      unixTimeOut = secsSince1900 - NTP_UNIX_DELTA;
      return true;
    }
    delay(10);
  }
  udp.stop();
  return false;
}

// Format helper: prints "YYYY-MM-DD HH:MM:SS"
void printFormatted(const char* label, uint32_t unixSeconds) {
  time_t t = (time_t)unixSeconds;
  struct tm* tm_p = gmtime(&t); // break into Y/M/D H:M:S
  if (!tm_p) return;

  char buf[32];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
           tm_p->tm_year + 1900, tm_p->tm_mon + 1, tm_p->tm_mday,
           tm_p->tm_hour, tm_p->tm_min, tm_p->tm_sec);
  Serial.print(label);
  Serial.println(buf);
}

void setup() {
  Serial.begin(115200);
  // Give USB serial a moment on some hosts
  uint32_t waitStart = millis();
  while (!Serial && millis() - waitStart < 2000) {}

  Serial.println("\nPico W NTP demo (UDP)");
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);

  if (!wifiConnect()) {
    Serial.println("Wi-Fi failed. Check credentials or router mood.");
    return;
  }
  Serial.print("Wi-Fi OK. IP: ");
  Serial.println(WiFi.localIP());

  uint32_t unixUtc;
  if (getUnixTimeFromNTP(unixUtc)) {
    printFormatted("UTC:   ", unixUtc);

    // Local time (UTC-5 for Bogotá)
    uint32_t local = unixUtc + TZ_OFFSET_SECONDS;
    printFormatted("Local: ", local);
  } else {
    Serial.println("NTP request timed out. Try again later.");
  }
}

void loop() {
  // Repeat every 10 seconds so you can watch it tick.
  static uint32_t last = 0;
  if (millis() - last >= 10000) {
    last = millis();
    uint32_t unixUtc;
    if (WiFi.status() == WL_CONNECTED && getUnixTimeFromNTP(unixUtc)) {
      printFormatted("UTC:   ", unixUtc);
      uint32_t local = unixUtc + TZ_OFFSET_SECONDS;
      printFormatted("Local: ", local);
    } else {
      Serial.println("No Wi-Fi or NTP timeout.");
    }
  }
}