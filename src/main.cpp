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

void clearAndDrawText(String text, int x, int y, int w, int h,
                      uint16_t textColor, uint16_t bgColor) {
  tft.fillRect(x, y, w, h, bgColor); // Clear the background
  tft.setTextColor(textColor, bgColor);
  tft.drawString(text, x, y);
}

void dateUpdate() {
  // --- Date Update ---
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

  // --- Time Update ---
  uint8_t hour = rtc.hour();
  uint8_t minute = rtc.minute();
  uint8_t second = rtc.second();

  tft.setFreeFont(FMB18); // Select the font
  // Draw hours
  char hourBuffer[3];
  sprintf(hourBuffer, "%02d", hour);
  if (hour != last_hour) {

    clearAndDrawText(hourBuffer, xpos, TIME_Y, 44, 22, TFT_GREEN, TFT_BLACK);

    last_hour = hour;
  }
  xpos += tft.textWidth(hourBuffer);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  xpos += tft.drawChar(':', xpos, TIME_Y + 18); // Minutes colon

  // Draw minutes
  char minuteBuffer[3];
  sprintf(minuteBuffer, "%02d", minute);
  if (minute != last_minute) {

    clearAndDrawText(minuteBuffer, xpos, TIME_Y, 44, 22, TFT_SKYBLUE,
                     TFT_BLACK);

    last_minute = minute;
  }

  xpos += tft.textWidth(minuteBuffer);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  xpos += tft.drawChar(':', xpos, TIME_Y + 18); // Seconds colon
  tft.setFreeFont(FMB12);                       // Select the font

  // Draw seconds
  char secondBuffer[3];
  sprintf(secondBuffer, "%02d", second);
  if (second != last_second) {

    clearAndDrawText(secondBuffer, xpos, TIME_Y, 32, 16, TFT_MAGENTA,
                     TFT_BLACK);
    last_second = second;
  }
}

void tempUpdate() {
  // --- Temperature Update ---
  int temp = rtc.temp() / 100;
  if (temp != last_temp) {
    char tempBuffer[5];
    sprintf(tempBuffer, "%2d", temp);

    clearAndDrawText(tempBuffer, tft.textWidth("Temp:  "), TEMP_Y, 28, 14,
                     TFT_GREENYELLOW, TFT_BLACK);

    // Draw the degree symbol as a circle
    int16_t x = tft.textWidth("Temp:  ") + tft.textWidth(tempBuffer) + 4;
    int16_t y = TEMP_Y + 4; // Adjust offset for vertical position
    tft.fillCircle(x, y, 2, TFT_GREEN);

    // Now draw the C
    tft.drawString("C", x + 4, TEMP_Y);
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