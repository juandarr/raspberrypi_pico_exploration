#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define PIN 15  // Data pin connected to the NeoPixel ring
#define NUMPIXELS 8 // Number of LEDs in the ring

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// set pin numbers
const int buttonPin = 10;     // the number of the pushbutton pin
const int ledPin =  LED_BUILTIN;       // the number of the LED pin

// variable for storing the push button status 
int buttonState = 0;
int isChase = false;

int delayval = 20;

void readButtonAndReact (){

        buttonState = digitalRead(buttonPin);
        Serial.println(buttonState);
        if (buttonState == HIGH){
          digitalWrite(ledPin, HIGH);
          if (not(isChase)){
            isChase = true;
          }
        }else {
          digitalWrite(ledPin, LOW);
          if (isChase){
            isChase = false;
          }
        }
}

uint32_t Wheel(byte WheelPos) {

  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + j) & 255));
      readButtonAndReact();
      if (isChase) break;
    }
    pixels.show();
    if (isChase) break;
    delay(wait);
  }

  if (isChase) return;
}
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  // do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, c);    //turn every third pixel on
        readButtonAndReact();
        if (not(isChase)) break;
      }
      pixels.show();
      if (not(isChase)) break;
      delay(wait);

      for (int i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, 0);        //turn every third pixel off
        if (not(isChase)) break;
      }
    }
    if (not(isChase)) break;
  }
  if (not(isChase)) return;
}

void setup() {

  pixels.begin(); // Initialize NeoPixel library.
  pixels.clear(); // Set all pixels off upon startup.
  pixels.show(); // Send the updated data to the ring.
  pixels.setBrightness(20);
}

void loop() {
  if (isChase){
    delayval= 30;
    theaterChase(pixels.Color(127, 127, 127), delayval); // White color
    theaterChase(pixels.Color(127,   0,   0), delayval); // Red color
    theaterChase(pixels.Color(  0,   0, 127), delayval); // Blue color
  }else {
    delayval = 10;
    rainbowCycle(delayval);
  }
}