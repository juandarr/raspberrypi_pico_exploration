// UI module implementation
#include <Arduino.h>
#include "Examples/resources/Free_Fonts.h"
#include <TFT_eSPI.h>
#include <uRTCLib.h>

// Local UI state 
namespace {
  // Layout Y positions 
  constexpr int DATE_Y = 45;
  constexpr int TIME_Y = 108;
  constexpr int TEMP_Y = 180;
  constexpr int UPDATE_Y = 252;

  // Previous date/time/temp
  uint8_t last_day = 99, last_month = 0, last_year = 0;
  uint8_t last_hour = 99, last_minute = 99, last_second = 99;
  float last_temp = -100.0f;
  uint8_t last_updated_hour = 99;
  uint8_t last_updated_minute = 99;
  uint8_t last_updated_second = 99;
  uint32_t last_updates = UINT32_MAX;

  static void clearAndDrawText(TFT_eSPI &tft, const String &text, int x, int y, int w, int h,
                               uint16_t textColor, uint16_t bgColor) {
    tft.fillRect(x, y, w, h, bgColor);
    tft.setTextColor(textColor, bgColor);
    tft.drawString(text, x, y);
  }

  static void dateUpdate(TFT_eSPI &tft, uRTCLib &rtc) {
    uint8_t day = rtc.day();
    uint8_t month = rtc.month();
    uint8_t year = rtc.year();
    if (day != last_day || month != last_month || year != last_year) {
      char dateBuffer[12];
      sprintf(dateBuffer, "%02d/%02d/%d", day, month, year);
      tft.setFreeFont(FMB12);
      clearAndDrawText(tft, dateBuffer, tft.textWidth("Date:  "), DATE_Y, 112, 16,
                       TFT_YELLOW, TFT_BLACK);
      last_day = day;
      last_month = month;
      last_year = year;
    }
  }

  static void timeUpdate(TFT_eSPI &tft, uRTCLib &rtc) {
    int xpos = 0;
    uint8_t hour = rtc.hour();
    uint8_t minute = rtc.minute();
    uint8_t second = rtc.second();
    tft.setFreeFont(FMB18);
    char hourBuffer[3];
    sprintf(hourBuffer, "%02d", hour);
    if (hour != last_hour) {
      clearAndDrawText(tft, hourBuffer, xpos, TIME_Y, 44, 22, TFT_PINK, TFT_BLACK);
      last_hour = hour;
    }
    xpos += tft.textWidth(hourBuffer);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    xpos += tft.drawChar(':', xpos, TIME_Y + 18);
    char minuteBuffer[3];
    sprintf(minuteBuffer, "%02d", minute);
    if (minute != last_minute) {
      clearAndDrawText(tft, minuteBuffer, xpos, TIME_Y, 44, 22, TFT_SKYBLUE, TFT_BLACK);
      last_minute = minute;
    }
    xpos += tft.textWidth(minuteBuffer);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    xpos += tft.drawChar(':', xpos, TIME_Y + 18);
    tft.setFreeFont(FMB12);
    char secondBuffer[3];
    sprintf(secondBuffer, "%02d", second);
    if (second != last_second) {
      clearAndDrawText(tft, secondBuffer, xpos, TIME_Y, 32, 16, TFT_MAGENTA, TFT_BLACK);
      last_second = second;
    }
  }

  static void tempUpdate(TFT_eSPI &tft, uRTCLib &rtc) {
    float temp = rtc.temp() / 100.0f;
    if (temp != last_temp) {
      char tempBuffer[16];
      dtostrf(temp, 0, 2, tempBuffer);
      clearAndDrawText(tft, tempBuffer, tft.textWidth("Temp:  "), TEMP_Y, 56, 14,
                       TFT_GREENYELLOW, TFT_BLACK);
      int16_t x = tft.textWidth("Temp:  ") + tft.textWidth(tempBuffer) + 4;
      int16_t y = TEMP_Y + 4;
      tft.fillCircle(x, y, 2, TFT_YELLOW);
      tft.drawString("C", x + 4, TEMP_Y);
      last_temp = temp;
    }
  }

  static void timeSinceUpdate(TFT_eSPI &tft, long elapsedMs, uint32_t upds) {
    float minutesF = (elapsedMs / 60000.0f);
    uint8_t hours = minutesF / 60;
    uint8_t minutes = int(minutesF - hours * 60);
    uint8_t seconds = int((minutesF - hours * 60 - minutes) * 60);
    tft.setFreeFont(FMB9);
    if (last_updated_hour != hours || last_updated_minute != minutes ||
        last_updated_second != seconds) {
      char timeBuffer[12];
      sprintf(timeBuffer, "%02d:%02d:%02d", hours, minutes, seconds);
      clearAndDrawText(tft, timeBuffer, tft.textWidth("SinceUpd: "), UPDATE_Y, 126, 14,
                       TFT_GREENYELLOW, TFT_BLACK);
      last_updated_hour = hours;
      last_updated_minute = minutes;
      last_updated_second = seconds;
    }

    if (last_updates != upds) {
      char updatesBuffer[12];
      sprintf(updatesBuffer, "%lu", (unsigned long)upds);
      clearAndDrawText(tft, updatesBuffer, tft.textWidth("Update #: "), UPDATE_Y + 35,
                       42, 14, TFT_GREENYELLOW, TFT_BLACK);
      last_updates = upds;
    }
  }
} // namespace

void uiSetup(TFT_eSPI &tft) {
  // Initial labels (identical to previous code)
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setFreeFont(FMB12);

  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.drawString("Date:  ", 0, DATE_Y);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("Temp:  ", 0, TEMP_Y);

  tft.setFreeFont(FMB9);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString("SinceUpd:  ", 0, UPDATE_Y);

  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.drawString("Update #:  ", 0, UPDATE_Y + 35);
}

void uiTick(TFT_eSPI &tft, uRTCLib &rtc, uint32_t sinceSyncMs, uint32_t updates) {
  rtc.refresh();
  dateUpdate(tft, rtc);
  timeUpdate(tft, rtc);
  tempUpdate(tft, rtc);
  timeSinceUpdate(tft, sinceSyncMs, updates);
}

