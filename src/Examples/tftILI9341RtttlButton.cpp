#include <Arduino.h>

// Include the PNG decoder library
#include <PNGdec.h>
#include "Examples/resources/reader.h" // Image is stored here in an 8-bit array
#include "Examples/resources/astro.h"
#include "Examples/resources/bike.h"
#include "Examples/resources/super.h"
#include "Examples/resources/camera.h"
// Include the TFT library https://github.com/Bodmer/TFT_eSPI
#include "SPI.h"
#include <TFT_eSPI.h>              // Hardware-specific library

// Stock font and GFXFF reference handle
#define GFXFF 1
#define FF18 &FreeSans12pt7b

#define OCTAVE_OFFSET 0
#define DEBUG false

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

TFT_eSPI tft = TFT_eSPI();         // Invoke custom library

// set pin numbers
const int buttonPrev = 17;     // the number of the pushbutton pin
const int buttonNext = 16;     // the number of the pushbutton pin
const int ledPin =  LED_BUILTIN;       // the number of the LED pin
const int speaker = 15;
int soundOn =  false;

// Exact equal-tempered frequencies C3..B8 (Hz) -----------------
const uint16_t noteFreq[] = {
  16,17,18,19,21,22,23,25,26,28,29,31, // C0..B0
  33,35,37,39,41,44,46,49,52,55,58,62, // C1..B1
  65,70,73,78,82,87,93,98,104,110,116,123, // C2..B2
  131,139,147,156,165,175,185,196,208,220,233,247,  // C3..B3
  262,277,294,311,330,349,370,392,415,440,466,494,  // C4..B4
  523,554,587,622,659,698,740,784,831,880,932,988,  // C5..B5
  1047,1109,1175,1245,1319,1397,1480,1568,1661,1760,1865,1976,  // C6..B6
  2093, 2217, 2349, 2489, 2637, 2793, 2959, 3135, 3322, 3520, 3729, 3951, // C7..B7
  4186, 4434, 4698, 4978, 5274, 5587, 5919, 6271, 6645, 7040, 7458, 7902, // C8..B8
}; 

char * songs[] ={
(char *)"Star Wars:d=4,o=5,b=45:32p,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#.6,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#6",
(char *)"ZeldaDungeon:d=4,o=5,b=80:16g,16a#,16d6,16d#6,16g,16a#,16d6,16d#6,16g,16a#,16d6,16d#6,16g,16a#,16d6,16d#6,16f#,16a,16d6,16d#6,16f#,16a,16d6,16d#6,16f#,16a,16d6,16d#6,16f#,16a,16d6,16d#6,16f,16g#,16d6,16d#6,16f,16g#,16d6,16d#6,16f,16g#,16d6,16d#6,16f,16g#,16d6,16d#6,16e,16g,16d6,16d#6,16e,16g,16d6,16d#6,16e,16g,16d6,16d#6,16e,16g,16d6,16d#6,16d#,16g,16c6,16d6,16d#,16g,16c6,16d6,16d#,16g,16c6,16d6,16d#,16g,16c6,16d6,16d,16g,16c6,16d6,16d,16g,16c6,16d6,16d,16g,16c6,16d6,16d,16g,16c6,16d6,16c,16f#,16a",
(char *)"ZeldaOvrGB:d=4,o=5,b=120:8c.,16g4,g.4,8c,16c,16d,16d#,16f,g.,16a,16a#,8a.,8g.,8f,8g.,16c,1c6,8p,8g,8d#6,8d6,8d#6,8f6,8g6,16c6,16g6,c.7,8g6,8f6,8d#6,8f6,16a#,16f6,a#.6,8f6,8d#6,8d6,8d#.6,16g,g.,16g,16f,8d#,8f,1g,8c.6,16g,g.,8c6,16c6,16d6,16d#6,16f6,g.6,16g#6,16a#6,8g#6,g6,8f6,8d#.6,16c6,g.6,8d#6,8c7,8g6,d#.7,8d7,8c7,8d7,8d#7,8f7,8g7,16f7,16g7,g#.7,a#7,8g#7,g7,8d7,8d#7,8f7,8d#7,8d7,2c.7",
(char *)"Mission Impossible:d=16,o=6,b=95:32d,32d#,32d,32d#,32d,32d#,32d,32d#,32d,32d,32d#,32e,32f,32f#,32g,g,8p,g,8p,a#,p,c7,p,g,8p,g,8p,f,p,f#,p,g,8p,g,8p,a#,p,c7,p,g,8p,g,8p,f,p,f#,p,a#,g,2d,32p,a#,g,2c#,32p,a#,g,2c,a#5,8c,2p,32p,a#5,g5,2f#,32p,a#5,g5,2f,32p,a#5,g5,2e,d#,8d",
(char *)"Zelda_gerudo:d=4,o=5,b=125:16c#,16f#,16g#,8a,16p,16c#,16f#,16g#,a,8p,16d,16f#,16g#,8a,16p,16d,16f#,16g#,a,8p,16b4,16e,16f#,8g#,16p,16b4,16e,16f#,g#,8p,16f#,16g#,16f#,2f,8p,16c#,16f#,16g#,8a,16p,16c#,16f#,16g#,a,8p,16d,16f#,16g#,8a,16p,16d,16f#,16g#,a,8p,16b4,16e,16f#,8g#,16p,16b4,16e,16f#,g#,8p,16a,16b,16a,2g#" ,
(char *)"ZeldaLegend:d=4,o=5,b=125:a#,f.,8a#,16a#,16c6,16d6,16d#6,2f6,8p,8f6,16f.6,16f#6,16g#.6,2a#.6,16a#.6,16g#6,16f#.6,8g#.6,16f#.6,2f6,f6,8d#6,16d#6,16f6,2f#6,8f6,8d#6,8c#6,16c#6,16d#6,2f6,8d#6,8c#6,8c6,16c6,16d6,2e6,g6,8f6,16f,16f,8f,16f,16f,8f,16f,16f,8f,8f,a#,f.,8a#,16a#,16c6,16d6,16d#6,2f6,8p,8f6,16f.6,16f#6,16g#.6,2a#.6,c#7,c7,2a6,f6,2f#.6,a#6,a6,2f6,f6,2f#.6,a#6,a6,2f6,d6,2d#.6,f#6,f6,2c#6,a#,c6,16d6,2e6,g6,8f6,16f,16f,8f,16f,16f,8f,16f,16f,8f,8f",
(char *)"victoryFFVIII:d=4,o=5,b=140:32d6,32p,32d6,32p,32d6,32p,d6,a#,c6,16d6,8p,16c6,2d6,a,g,a,16g,16p,c6,16c6,16p,b,16c6,16p,b,16b,16p,a,g,f#,16g,16p,1e,a,g,a,16g,16p,c6,16c6,16p,b,16c6,16p,b,16b,16p,a,g,a,16c6,16p,1d6",
(char *)"RickRoll:d=4,o=5,b=200:8g,8a,8c6,8a,e6,8p,e6,8p,d6.,p,8p,8g,8a,8c6,8a,d6,8p,d6,8p,c6,8b,a.,8g,8a,8c6,8a,2c6,d6,b,a,g.,8p,g,2d6,2c6.,p,8g,8a,8c6,8a,e6,8p,e6,8p,d6.,p,8p,8g,8a,8c6,8a,2g6,b,c6.,8b,a,8g,8a,8c6,8a,2c6,d6,b,a,g.,8p,g,2d6,2c6.",
(char *)"SuperMarioBross:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,16p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16c7,16p,16c7,16c7,p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16d#6,8p,16d6,8p,16c6",
(char *)"LeisureSuit:d=16,o=6,b=56:f.5,f#.5,g.5,g#5,32a#5,f5,g#.5,a#.5,32f5,g#5,32a#5,g#5,8c#.,a#5,32c#,a5,a#.5,c#.,32a5,a#5,32c#,d#,8e,c#.,f.,f.,f.,f.,f,32e,d#,8d,a#.5,e,32f,e,32f,c#,d#.,c#",
(char *)"SMBunderground:d=16,o=6,b=80:c,c5,a5,a,a#5,a#,2p,8p,c,c5,a5,a,a#5,a#,2p,8p,f5,f,d5,d,d#5,d#,2p,8p,f5,f,d5,d,d#5,d#,2p,32d#,d,32c#,c,p,d#,p,d,p,g#5,p,g5,p,c#,p,32c,f#,32f,32e,a#,32a,g#,32p,d#,b5,32p,a#5,32p,a5,g#5",
(char *)"SMBwater:d=8,o=6,b=225:4d5,4e5,4f#5,4g5,4a5,4a#5,b5,b5,b5,p,b5,p,2b5,p,g5,2e.,2d#.,2e.,p,g5,a5,b5,c,d,2e.,2d#,4f,2e.,2p,p,g5,2d.,2c#.,2d.,p,g5,a5,b5,c,c#,2d.,2g5,4f,2e.,2p,p,g5,2g.,2g.,2g.,4g,4a,p,g,2f.,2f.,2f.,4f,4g,p,f,2e.,4a5,4b5,4f,e,e,4e.,b5,2c.",
(char *)"The Simpsons:d=4,o=5,b=160:c.6,e6,f#6,8a6,g.6,e6,c6,8a,8f#,8f#,8f#,2g,8p,8p,8f#,8f#,8f#,8g,a#.,8c6,8c6,8c6,c6",
(char *)"Indiana:d=4,o=5,b=250:e,8p,8f,8g,8p,1c6,8p.,d,8p,8e,1f,p.,g,8p,8a,8b,8p,1f6,p,a,8p,8b,2c6,2d6,2e6,e,8p,8f,8g,8p,1c6,p,d6,8p,8e6,1f.6,g,8p,8g,e.6,8p,d6,8p,8g,e.6,8p,d6,8p,8g,f.6,8p,e6,8p,8d6,2c6",
(char *)"TakeOnMe:d=4,o=4,b=160:8f#5,8f#5,8f#5,8d5,8p,8b,8p,8e5,8p,8e5,8p,8e5,8g#5,8g#5,8a5,8b5,8a5,8a5,8a5,8e5,8p,8d5,8p,8f#5,8p,8f#5,8p,8f#5,8e5,8e5,8f#5,8e5,8f#5,8f#5,8f#5,8d5,8p,8b,8p,8e5,8p,8e5,8p,8e5,8g#5,8g#5,8a5,8b5,8a5,8a5,8a5,8e5,8p,8d5,8p,8f#5,8p,8f#5,8p,8f#5,8e5,8e5",
(char *)"Entertainer:d=4,o=5,b=140:8d,8d#,8e,c6,8e,c6,8e,2c.6,8c6,8d6,8d#6,8e6,8c6,8d6,e6,8b,d6,2c6,p,8d,8d#,8e,c6,8e,c6,8e,2c.6,8p,8a,8g,8f#,8a,8c6,e6,8d6,8c6,8a,2d6",
(char *)"Looney:d=4,o=5,b=140:32p,c6,8f6,8e6,8d6,8c6,a.,8c6,8f6,8e6,8d6,8d#6,e.6,8e6,8e6,8c6,8d6,8c6,8e6,8c6,8d6,8a,8c6,8g,8a#,8a,8f",
(char *)"GoodBad:d=4,o=5,b=56:32p,32a#,32d#6,32a#,32d#6,8a#.,16f#.,16g#.,d#,32a#,32d#6,32a#,32d#6,8a#.,16f#.,16g#.,c#6,32a#,32d#6,32a#,32d#6,8a#.,16f#.,32f.,32d#.,c#,32a#,32d#6,32a#,32d#6,8a#.,16g#.,d#",
(char *)"Flinstones:d=4,o=5,b=40:32p,16f6,16a#,16a#6,32g6,16f6,16a#.,16f6,32d#6,32d6,32d6,32d#6,32f6,16a#,16c6,d6,16f6,16a#.,16a#6,32g6,16f6,16a#.,32f6,32f6,32d#6,32d6,32d6,32d#6,32f6,16a#,16c6,a#,16a6,16d.6,16a#6,32a6,32a6,32g6,32f#6,32a6,8g6,16g6,16c.6,32a6,32a6,32g6,32g6,32f6,32e6,32g6,8f6,16f6,16a#.,16a#6,32g6,16f6,16a#.,16f6,32d#6,32d6,32d6,32d#6,32f6,16a#,16c.6,32d6,32d#6,32f6,16a#,16c.6,32d6,32d#6,32f6,16a#6,16c7,8a#.6",
(char *)"Gadget:d=16,o=5,b=50:32d#,32f,32f#,32g#,a#,f#,a,f,g#,f#,32d#,32f,32f#,32g#,a#,d#6,4d6,32d#,32f,32f#,32g#,a#,f#,a,f,g#,f#,8d#",
(char *)"MahnaMahna:d=16,o=6,b=125:c#,c.,b5,8a#.5,8f.,4g#,a#,g.,4d#,8p,c#,c.,b5,8a#.5,8f.,g#.,8a#.,4g,8p,c#,c.,b5,8a#.5,8f.,4g#,f,g.,8d#.,f,g.,8d#.,f,8g,8d#.,f,8g,d#,8c,a#5,8d#.,8d#.,4d#,8d#."
};

byte default_dur = 4;
byte default_oct = 6;
byte lowest_oct = 0;
int bpm = 63;
int num;
long wholenote;
long duration;
byte note;
byte scale;
bool songStarts = false;
char *songPtr;

void begin_rtttl(int idx,char *p)
{
  // Absolutely no error checking in here
  // format: d=N,o=N,b=NNN:
  // find the start (skip name, etc)
  const char* start = p;
  while(*p != ':') p++;    // ignore name
  
  int nameLength = p - start; // Calculate the length of the name

  // Create a character buffer for the name.
  // Make sure it's large enough for the longest possible name + 1 for the null terminator.
  char nameBuffer[20];

  // Check if a name was found and if it fits in our buffer
  if (nameLength > 0 && nameLength < sizeof(nameBuffer)) {
    // Copy the characters from the original string to our buffer
    char cstr[24]; // Ensure the array is large enough to hold the number and null terminator
    itoa(idx+1, cstr, 10); // 10 specifies base 10 (decimal)
    strncpy(nameBuffer, start, nameLength);
    char sep[3] = ". ";
    strcat(cstr,sep);
    
    // IMPORTANT: strncpy does not guarantee a null terminator. Add it manually.
    nameBuffer[nameLength] = '\0';
    strcat(cstr, nameBuffer);
    Serial.print("The extracted name is: ");
    Serial.println(cstr); // Prints "The extracted name is: PlayerName"
    //Serial.println(nameBuffer); // Prints "The extracted name is: PlayerName"

    tft.drawString(cstr, 20, 290, GFXFF);// Print the string name of the font
  }
  p++;                     // skip ':'

  // get default duration
  if(*p == 'd')
  {
    p++; p++;              // skip "d="
    num = 0;
    while(isdigit(*p))
    {
      num = (num * 10) + (*p++ - '0');
    }
    if(num > 0) default_dur = num;
    p++;                   // skip comma
  }

  if(DEBUG) { Serial.print("ddur: "); Serial.println(default_dur, 10); }

  // get default octave
  if(*p == 'o')
  {
    p++; p++;              // skip "o="
    num = *p++ - '0';
    if(num >= 3 && num <=7) default_oct = num;
    p++;                   // skip comma
  }

  if(DEBUG) { Serial.print("doct: "); Serial.println(default_oct, 10); }

  // get BPM
  if(*p == 'b')
  {
    p++; p++;              // skip "b="
    num = 0;
    while(isdigit(*p))
    {
      num = (num * 10) + (*p++ - '0');
    }
    bpm = num;
    p++;                   // skip colon
  }

  if(DEBUG) { Serial.print("bpm: "); Serial.println(bpm, 10); }

  // BPM usually expresses the number of quarter notes per minute
  wholenote = (60 * 1000L / bpm) * 2;  // this is the time for whole note (in milliseconds)

  if(DEBUG) { Serial.print("wn: "); Serial.println(wholenote, 10); }
  
  // Save current song pointer...
  songPtr = p;
}

bool next_rtttl() {

  char *p = songPtr;
  // if notes remain, play next note
  if(*p)
  {
    // first, get note duration, if available
    num = 0;
    while(isdigit(*p))
    {
      num = (num * 10) + (*p++ - '0');
    }
    
    if(num) duration = wholenote / num;
    else duration = wholenote / default_dur;  // we will need to check if we are a dotted note after

    // now get the note
    note = 0;

    switch(*p)
    {
      case 'c':
        note = 1;
        break;
      case 'd':
        note = 3;
        break;
      case 'e':
        note = 5;
        break;
      case 'f':
        note = 6;
        break;
      case 'g':
        note = 8;
        break;
      case 'a':
        note = 10;
        break;
      case 'b':
        note = 12;
        break;
      case 'p':
      default:
        note = 0;
    }
    p++;

    // now, get optional '#' sharp
    if(*p == '#')
    {
      note++;
      p++;
    }

    // now, get optional '.' dotted note
    if(*p == '.')
    {
      duration += duration/2;
      p++;
    }
  
    // now, get scale
    if(isdigit(*p))
    {
      scale = *p - '0';
      p++;
    }
    else
    {
      scale = default_oct;
    }

    scale += OCTAVE_OFFSET;

    if(*p == ',')
      p++;       // skip comma for next note (or we may be at the end)

    // Save current song pointer...
    songPtr = p;

    // now play the note
    if(note)
    {
      if(DEBUG) {
        Serial.print("Playing: ");
        Serial.print(scale, 10); Serial.print(' ');
        Serial.print(note, 10); Serial.print(" (");
        Serial.print(noteFreq[(scale - lowest_oct) * 12 + (note-1)], 10);
        Serial.print(") ");
        Serial.println(duration, 10);
      }
      uint16_t frequency = noteFreq[(scale - lowest_oct) * 12 + (note-1)];
      tone(speaker, frequency, duration);
      Serial.printf("Frequency: %f\n", frequency);

      // to distinguish the notes, set a minimum time between them.
      // the note's duration + 45% seems to work well:
      //1.45
      int pauseBetweenNotes = duration * 1.2;
      delay(pauseBetweenNotes);
      // stop the tone playing:
      noTone(speaker);
    }
    else
    {
      if(DEBUG) {
        Serial.print("Pausing: ");
        Serial.println(duration, 10);
      }
      delay(duration);
    }
    return 1; // note played successfully.
  }
  else {
    return 0; // all done
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

//-------------------
// MAIN PROGRAM
//-------------------

bool play = false;

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n Starting initialisation.");
  Serial.println("\n\n Using the PNGdec library");

  // Initialise the TFT
  tft.begin();
  tft.fillScreen(TFT_BLACK);

  pinMode(ledPin, OUTPUT);
  pinMode(buttonPrev, INPUT_PULLDOWN);
  pinMode(buttonNext, INPUT_PULLDOWN);
  pinMode(speaker, OUTPUT);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setFreeFont(FF18);                 // Select the font
  Serial.println("\r\nInitialisation done.");
}

int i = 0;
int tmp = -1;
int buttonPrevState = 0;
int buttonNextState = 0;

void loop(void)
{

  buttonPrevState = digitalRead(buttonPrev);
  buttonNextState = digitalRead(buttonNext);

  if (buttonPrevState == HIGH){
    i -= 1;
    delay(60);
  }else if (buttonNextState == HIGH){
    i += 1;
    delay(60);
  }
  if (tmp!=i) {

  tft.fillScreen(TFT_BLACK);
  int16_t rc = png.openFLASH((uint8_t *)image_data[i%(sizeof(image_data)/sizeof(char *))], image_sizes[i%(sizeof(image_data)/sizeof(char *))], pngDraw);
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

    if(!songStarts) 
    { // Start song
      digitalWrite(ledPin,HIGH); // Light the onboard LED while the song plays
      songStarts = true;
      begin_rtttl(i%(sizeof(songs)/sizeof(char *)),songs[i%(sizeof(songs)/sizeof(char *))]);
    }
    //Remove if you want to play ring tone while pressing push button
    while (next_rtttl()){
      buttonPrevState = digitalRead(buttonPrev);
      buttonNextState = digitalRead(buttonNext);
      if (buttonPrevState || buttonNextState){
        digitalWrite(ledPin,LOW); // Turn off the onboard LED.
        songStarts = false;
        play = false;
        if(DEBUG) Serial.println("Done!");
        delay(200);
        break;
      }
    }
    if(!next_rtttl()) 
    { // Play next note
      digitalWrite(ledPin,LOW); // Turn off the onboard LED.
      songStarts = false;
      play = false;
      if(DEBUG) Serial.println("Done!");
      delay(200);
    }
  tmp = i; 
  }
  
}