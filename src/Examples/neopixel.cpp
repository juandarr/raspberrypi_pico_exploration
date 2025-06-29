#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define PIN 15  // Data pin connected to the NeoPixel ring
#define NUMPIXELS 8 // Number of LEDs in the ring

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {

  pixels.begin(); // Initialize NeoPixel library.
  pixels.clear(); // Set all pixels off upon startup.
  pixels.show(); // Send the updated data to the ring.
  pixels.setBrightness(20);
}
void loop() {
  // Example: Cycle through colors
  for(int i=0; i<NUMPIXELS; i++){
    pixels.setPixelColor(i, pixels.Color(0, 150, 0)); // Green
    pixels.show();
    delay(250);
    pixels.setPixelColor(i, pixels.Color(0, 0, 150)); // Blue
    pixels.show();
    delay(250);
    pixels.setPixelColor(i, pixels.Color(150, 0, 0)); // Red
    pixels.show();
    delay(250);
  }

  pixels.clear(); // Turn off all LEDs after the cycle
  pixels.show();
  delay(1000); // Wait for a second
}