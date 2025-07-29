#include "Arduino.h"
#include "uRTCLib.h"
#include "Wire.h"

#include "Examples/resources/Free_Fonts.h" // Include the header file attached to this sketch
#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>

#define TFT_GREY 0x5AEB

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

// uRTCLib rtc;
uRTCLib rtc(0x68);

char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

void setup() {
  Serial.begin(9600);
  Wire1.setSDA(14);
  Wire1.setSCL(15);
  Wire1.begin();
  //URTCLIB_WIRE.begin();

  // Comment out below line once you set the date & time.
  // Following line sets the RTC with an explicit date & time
  // for example to set April 14 2025 at 12:56 you would call:
  //rtc.set(15, 36, 18, 4, 23, 7, 25);
  // rtc.set(second, minute, hour, dayOfWeek, dayOfMonth, month, year)
  // set day of week (1=Sunday, 7=Saturday)
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
}

int charSize = 6;
int secSize = 4;

byte xcolon = 0, xsecs = 0;

void loop() {
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setFreeFont(FMB12);                 // Select the font
  rtc.refresh();

    // Update digital time
    int xpos = 0;
    int ypos = 45; // Top left corner ot clock text, about half way down

      xpos += tft.drawString("Date: ", xpos, ypos);
      // Draw day, month and year 
      // Create a buffer to hold the formatted date string "DD/MM/YYYY" + null terminator
  char dateBuffer[12];

  // Get time from RTC (replace with your rtc.day(), etc.)
  uint8_t day = rtc.day();   // Example: rtc.day();
  uint8_t month = rtc.month();  // Example: rtc.month();
  uint8_t year = rtc.year();    // Example: rtc.year();

  // Format the numbers into the buffer.
  // %02d means an integer, padded with a leading 0 if it's less than 2 digits.
  sprintf(dateBuffer, "%02d/%02d/%d", day, month, year);
    tft.fillRect(xpos, ypos, 40, 22, TFT_BLACK); // Erase old second
      tft.drawString(dateBuffer, xpos, ypos);

      xpos = 0;
      ypos = 108; // Top left corner ot clock text, about half way down
    //int ysecs = ypos + 16;
  tft.setFreeFont(FMB18);                 // Select the font
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
      // Draw hours and minutes
      uint8_t hh = rtc.hour();
      char hour[3];
    sprintf(hour, "%02d", hh);
    tft.fillRect(xpos, ypos, 40, 22, TFT_BLACK); // Erase old second
      xpos += tft.drawString(hour, xpos, ypos); // Add hours leading zero for 24 hr clock

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      xpos += tft.drawChar(':', xpos, ypos+18 );
    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
      uint8_t mm = rtc.minute();
      char minute[3];
    sprintf(minute, "%02d", mm);
    tft.fillRect(xpos, ypos, 40, 22, TFT_BLACK); // Erase old second
      xpos += tft.drawString(minute, xpos, ypos); // Add hours leading zero for 24 hr clock
    

  tft.setFreeFont(FMB12);                 // Select the font
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        xpos += tft.drawChar(':', xpos, ypos+18); // Seconds colon
      
    tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
      //Draw seconds
      uint8_t ss = rtc.second();
      char seconds[3];
    sprintf(seconds, "%02d", ss);
    tft.fillRect(xpos, ypos, 40, 22, TFT_BLACK); // Erase old second
      xpos += tft.drawString(seconds, xpos, ypos); // Add hours leading zero for 24 hr clock
    
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      xpos = 0;
      ypos = 180;
      xpos += tft.drawString("Temp: ", xpos, ypos);
      char temp[4];
      sprintf(temp, "%02dC",rtc.temp()/100);
    tft.fillRect(xpos, ypos, 40, 22, TFT_BLACK); // Erase old second
      xpos += tft.drawString(temp, xpos, ypos);

  delay(1000);
}