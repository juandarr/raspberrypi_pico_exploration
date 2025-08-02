#include "Arduino.h"
#include "Wire.h"
#include "uRTCLib.h"

#include "Examples/resources/Free_Fonts.h" // Include the header file attached to this sketch
#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library

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

void dateUpdate() {
  // --- Date Update ---
  uint8_t day = rtc.day();
  uint8_t month = rtc.month();
  uint8_t year = rtc.year();

  if (day != last_day || month != last_month || year != last_year) {
    char dateBuffer[12];
    sprintf(dateBuffer, "%02d/%02d/%d", day, month, year);

    tft.setFreeFont(FMB12);
    // Erase old date by drawing over it in black
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    char oldDateBuffer[12];
    sprintf(oldDateBuffer, "%02d/%02d/%d", last_day, last_month, last_year);
    tft.drawString(oldDateBuffer, tft.textWidth("Date:  "), DATE_Y);

    // Draw new date
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(dateBuffer, tft.textWidth("Date:  "), DATE_Y);

    last_day = day;
    last_month = month;
    last_year = year;
  }
}

void timeUpdate() {

  int xpos = 0;

  // --- Time Update ---
  uint8_t hour = rtc.hour();
  uint8_t minute = rtc.minute();
  uint8_t second = rtc.second();

  tft.setFreeFont(FMB18); // Select the font
  // Draw hours
  char hourBuffer[3];
  sprintf(hourBuffer, "%02d", hour);
  if (hour != last_hour) {

    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    char oldHourBuffer[3];
    sprintf(oldHourBuffer, "%02d", last_hour);
    tft.drawString(oldHourBuffer, xpos, TIME_Y);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    sprintf(hourBuffer, "%02d", hour);
    tft.drawString(hourBuffer, xpos, TIME_Y);
    last_hour = hour;
  }
  xpos += tft.textWidth(hourBuffer);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  xpos += tft.drawChar(':', xpos, TIME_Y + 18); // Minutes colon

  // Draw minutes
  char minuteBuffer[3];
  sprintf(minuteBuffer, "%02d", minute);
  if (minute != last_minute) {

    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    char oldMinuteBuffer[3];
    sprintf(oldMinuteBuffer, "%02d", last_minute);
    tft.drawString(oldMinuteBuffer, xpos, TIME_Y);

    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
    sprintf(minuteBuffer, "%02d", minute);
    tft.drawString(minuteBuffer, xpos, TIME_Y);
    last_minute = minute;
  }

  xpos += tft.textWidth(minuteBuffer);

  tft.setFreeFont(FMB12); // Select the font
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  xpos += tft.drawChar(':', xpos, TIME_Y + 18); // Seconds colon

  // Draw seconds
  char secondBuffer[3];
  sprintf(secondBuffer, "%02d", second);
  if (second != last_second) {

    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    char oldSecondBuffer[3];
    sprintf(oldSecondBuffer, "%02d", last_second);
    tft.drawString(oldSecondBuffer, xpos, TIME_Y);

    tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    sprintf(secondBuffer, "%02d", second);
    tft.drawString(secondBuffer, xpos, TIME_Y);
    last_second = second;
  }
}

void tempUpdate() {
  // --- Temperature Update ---
  int temp = rtc.temp() / 100;
  if (temp != last_temp) {
    char tempBuffer[5];
    sprintf(tempBuffer, "%2dC", temp);

    tft.setFreeFont(FMB12);
    // Erase old temp
    tft.setTextColor(TFT_BLACK);
    char oldTempBuffer[5];
    sprintf(oldTempBuffer, "%2dC", last_temp);
    tft.drawString(oldTempBuffer, tft.textWidth("Temp:  "), TEMP_Y);

    // Draw new temp
    tft.setTextColor(TFT_YELLOW);
    tft.drawString(tempBuffer, tft.textWidth("Temp:  "), TEMP_Y);

    last_temp = temp;
  }
}

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

  // Initial labels
  tft.setFreeFont(FMB12);
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.drawString("Date:  ", 0, DATE_Y);
  tft.setFreeFont(FMB12);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString("Temp:  ", 0, TEMP_Y);
}

void loop() {
  rtc.refresh();

  dateUpdate();
  timeUpdate();
  tempUpdate();

  delay(1000);
}