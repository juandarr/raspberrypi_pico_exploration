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
  rtc.refresh();

    // Update digital time
    int xpos = 0;
    int ypos = 45; // Top left corner ot clock text, about half way down

      xpos += tft.drawString("Date: ", xpos, ypos, secSize);
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
      tft.drawString(dateBuffer, xpos, ypos, secSize);

      xpos = 0;
      ypos = 108; // Top left corner ot clock text, about half way down
    int ysecs = ypos + 16;
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
      // Draw hours and minutes
      uint8_t hh = rtc.hour();
      if (hh < 10) xpos += tft.drawChar('0', xpos, ypos, charSize); // Add hours leading zero for 24 hr clock
      xpos += tft.drawNumber(hh, xpos, ypos, charSize);             // Draw hours
      xcolon = xpos; // Save colon coord for later to flash on/off later

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      xpos += tft.drawChar(':', xpos, ypos - charSize, charSize);
    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
      uint8_t  mm = rtc.minute();
      if (mm < 10) xpos += tft.drawChar('0', xpos, ypos, charSize); // Add minutes leading zero
      xpos += tft.drawNumber(mm, xpos, ypos, charSize);             // Draw minutes
      xsecs = xpos; // Sae seconds 'x' position for later display updates
    

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        xpos += tft.drawChar(':', xsecs, ysecs, secSize); // Seconds colon
      
    tft.setTextColor(TFT_MAGENTA, TFT_BLACK);

      //Draw seconds
      uint8_t ss = rtc.second();
      if (ss < 10) xpos += tft.drawChar('0', xpos, ysecs, secSize); // add leading zero
      tft.drawNumber(ss, xpos, ysecs, secSize);                     // Draw seconds
    
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      xpos = 0;
      ypos = 180;
      xpos += tft.drawString("Temp: ", xpos, ypos, secSize);
      xpos += tft.drawNumber(rtc.temp()/100, xpos, ypos, secSize);
      xpos += tft.drawChar('°', xpos, ypos, secSize);
      tft.drawChar('C', xpos, ypos, secSize);




  Serial.print("Current Date & Time: ");
  Serial.print(rtc.year());
  Serial.print('/');
  Serial.print(rtc.month());
  Serial.print('/');
  Serial.print(rtc.day());

  Serial.print(" (");
  Serial.print(daysOfTheWeek[rtc.dayOfWeek() - 1]);
  Serial.print(") ");

  Serial.print(rtc.hour());
  Serial.print(':');
  Serial.print(rtc.minute());
  Serial.print(':');
  Serial.println(rtc.second());

  Serial.print("Temperature: ");
  Serial.print(rtc.temp() / 100);
  Serial.println("°C");

  Serial.println();
  delay(1000);
}