#include "Arduino.h"
#include "uRTCLib.h"
#include "Wire.h"

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
  rtc.set(15, 36, 18, 4, 23, 7, 25);
  // rtc.set(second, minute, hour, dayOfWeek, dayOfMonth, month, year)
  // set day of week (1=Sunday, 7=Saturday)
}

void loop() {
  rtc.refresh();

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