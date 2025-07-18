// This example renders a png file that is stored in a FLASH array
// using the PNGdec library (available via library manager).

// Note: The PNGDEC required lots of RAM to work (~40kbytes) so
// this sketch is will not run on smaller memory processors (e.g.
// ESP8266, STM32F103 etc.)

// Image files can be converted to arrays using the tool here:
// https://notisrac.github.io/FileToCArray/
// To use this tool:
//   1. Drag and drop file on "Browse..." button
//   2. Tick box "Treat as binary"
//   3. Click "Convert"
//   4. Click "Save as file" and move the header file to sketch folder
//   5. Open the sketch in IDE
//   6. Include the header file containing the array (panda.h in this example)

// Arduino library
#include "Arduino.h"
// Include the PNG decoder library
#include <PNGdec.h>
#include "Examples/resources/reader.h" // Image is stored here in an 8-bit array
#include "Examples/resources/astro.h"
#include "Examples/resources/bike.h"
#include "Examples/resources/super.h"
#include "Examples/resources/camera.h"


// 1. Create an array of pointers to your image data arrays.
//    The type 'const uint8_t*' matches the 'const byte[]' from your image files.
const uint8_t* image_data[] = {reader, astro, bike, super, camera};

// 2. Create a parallel array to store the size of each image array.
//    We use sizeof() on the original arrays here.
const size_t image_sizes[] = {sizeof(reader), sizeof(astro), sizeof(bike)};

// 3. Calculate the number of images automatically. This is more robust than hard-coding '3'.
const int num_images = sizeof(image_data) / sizeof(image_data[0]);

//int[] arr[] = [reader, astro, bike];

PNG png; // PNG decoder instance

#define MAX_IMAGE_WIDTH 240 // Adjust for your images

int16_t xpos = 0;
int16_t ypos = 0;

// Include the TFT library https://github.com/Bodmer/TFT_eSPI
#include "SPI.h"
#include <TFT_eSPI.h>              // Hardware-specific library
TFT_eSPI tft = TFT_eSPI();         // Invoke custom library

// set button numbers
const int buttonUp = 17;     // the number of the pushbutton up
const int buttonDown = 16;

//====================================================================================
//                                    Setup
//====================================================================================
void setup()
{
  Serial.begin(115200);
  Serial.println("\n\n Using the PNGdec library");

  // Initialise the TFT
  tft.begin();
  tft.fillScreen(TFT_BLACK);

  pinMode(buttonUp, INPUT_PULLDOWN);
  pinMode(buttonDown, INPUT_PULLDOWN);
  Serial.println("\r\nInitialisation done.");
}

//====================================================================================
//                                    Loop
//====================================================================================
int i = 0;
int tmp = -1;
int buttonUpState = 0;
int buttonDownState = 0;
void loop()
{
  buttonUpState = digitalRead(buttonUp);
  buttonDownState = digitalRead(buttonDown);

  if (buttonUpState == HIGH){
    i += 1;
    if (i>=num_images){
      i = 0;
    }
    delay(60);
  }else if (buttonDownState == HIGH){
    i -= 1;
    if (i<0){
      i = num_images - 1;
    }
    delay(60);
  }
  if (tmp!=i) {
  int16_t rc = png.openFLASH((uint8_t *)image_data[i], image_sizes[i], pngDraw);
    if (rc == PNG_SUCCESS) {
      Serial.println("Successfully opened png file");
      Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
      tft.startWrite();
      uint32_t dt = millis();
      rc = png.decode(NULL, 0);
      Serial.print(millis() - dt); Serial.println("ms");
      tft.endWrite();
    // png.close(); // not needed for memory->memory decode

  }
  tmp = i; 
  }
}


//=========================================v==========================================
//                                      pngDraw
//====================================================================================
// This next function will be called during decoding of the png file to
// render each image line to the TFT.  If you use a different TFT library
// you will need to adapt this function to suit.
// Callback function to draw pixels to the display
void pngDraw(PNGDRAW *pDraw) {
  uint16_t lineBuffer[MAX_IMAGE_WIDTH];
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  tft.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}

