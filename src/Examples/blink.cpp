#include <Arduino.h>

const int ledPin = LED_BUILTIN;
const int d = 500;

void setup() {
  // ledPin as output
  pinMode(ledPin, OUTPUT);

  Serial.begin(115200);

  // Wait for delay d before proceeding
  delay(d);
  Serial.println("Hello Pico from PlatformIO!");
}

void loop() {
  // Turn on LED
  digitalWrite(ledPin, HIGH);
  Serial.println("LED ON");
  delay(d); 

  // Turn off LED
  digitalWrite(ledPin, LOW);
  Serial.println("LED OFF");
  delay(d); 
}