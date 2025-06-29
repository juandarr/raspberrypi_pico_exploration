#include <Arduino.h>

// set pin numbers
const int buttonPin = 10;     // the number of the pushbutton pin
const int ledPin =  LED_BUILTIN;       // the number of the LED pin
const int buzzer = 13;
int soundOn =  false;
// variable for storing the push button status 
int buttonState = 0;

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLDOWN);
  pinMode(buzzer, OUTPUT);

}

void loop(){
  buttonState = digitalRead(buttonPin);
  if (soundOn == true){

    digitalWrite(ledPin, HIGH);
    if (buttonState == HIGH) {
      soundOn = false;
    }
    digitalWrite(buzzer, HIGH);
    delay(100);
    digitalWrite(buzzer, LOW);
    delay(100);
  }else{

    digitalWrite(ledPin, LOW);
    if (buttonState == HIGH) {
      soundOn = true;
    }
  }
}