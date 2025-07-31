#include "Arduino.h"
#include "Wire.h"
#include "uRTCLib.h"

#include "Examples/resources/Free_Fonts.h" // Include the header file attached to this sketch
#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library

TFT_eSPI tft = TFT_eSPI(); // Create tft object

// uRTCLib rtc;
uRTCLib rtc(0x68);

char daysOfTheWeek[7][12] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                             "Thursday", "Friday", "Saturday"};

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
}

String last_date_str = "";
String last_hour_str = "";
String last_min_str = "";
String last_sec_str = "";
String last_temp_str = "";

void loop() {
  tft.setFreeFont(FMB12); // Select the font
  rtc.refresh();

  // Update digital time
  int xpos = 0;
  int ypos = 45; // Top left corner ot clock text, about half way down

  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  xpos += tft.drawString("Date: ", xpos, ypos);
  // Draw day, month and year
  // Create a buffer to hold the formatted date string "DD/MM/YYYY" + null
  // terminator
  char dateBuffer[12];

  // Get time from RTC (replace with your rtc.day(), etc.)
  uint8_t day = rtc.day();
  uint8_t month = rtc.month();
  uint8_t year = rtc.year();

  // Format the numbers into the buffer.
  // %02d means an integer, padded with a leading 0 if it's less than 2 digits.
  sprintf(dateBuffer, "%02d/%02d/%d", day, month, year);
  String new_date_str = String(dateBuffer);
  if (last_date_str != new_date_str) {

    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.drawString(last_date_str, xpos, ypos);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(new_date_str, xpos, ypos);
    last_date_str = new_date_str;
  }

  xpos = 0;
  ypos = 108; // Top left corner ot clock text, about half way down

  tft.setFreeFont(FMB18); // Select the font
  // Draw hours
  uint8_t hh = rtc.hour();
  char hour[3];
  sprintf(hour, "%02d", hh);
  String new_hour_str = String(hour);

  if (last_hour_str != new_hour_str) {
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.drawString(last_hour_str, xpos, ypos);
    last_hour_str = new_hour_str;
  }
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  xpos += tft.drawString(new_hour_str, xpos,
                         ypos); // Add hours leading zero for 24 hr clock

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  xpos += tft.drawChar(':', xpos, ypos + 18); // Minutes colon
  // Draw minutes
  uint8_t mm = rtc.minute();
  char minute[3];
  sprintf(minute, "%02d", mm);
  String new_min_str = String(minute);
  if (last_min_str != new_min_str) {
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.drawString(last_min_str, xpos, ypos);
    last_min_str = new_min_str;
  }

  tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);

  xpos += tft.drawString(new_min_str, xpos, ypos);

  tft.setFreeFont(FMB12); // Select the font
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  xpos += tft.drawChar(':', xpos, ypos + 18); // Seconds colon

  // Draw seconds
  uint8_t ss = rtc.second();
  char seconds[3];
  sprintf(seconds, "%02d", ss);
  String new_sec_str = String(seconds);
  if (last_sec_str != new_sec_str) {
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.drawString(last_sec_str, xpos, ypos);
    tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    xpos += tft.drawString(new_sec_str, xpos, ypos);
    last_sec_str = new_sec_str;
  }

  xpos = 0;
  ypos = 180;
  // Draw temperature
  tft.setTextColor(TFT_RED, TFT_BLACK);
  xpos += tft.drawString("Temp: ", xpos, ypos);
  char temp[5];
  sprintf(temp, "%02dC", rtc.temp() / 100);
  String new_temp_str = String(temp);
  if (last_temp_str != new_temp_str) {
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.drawString(last_temp_str, xpos, ypos);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(new_temp_str, xpos, ypos);
    last_temp_str = new_temp_str;
  }

  delay(1000);
}