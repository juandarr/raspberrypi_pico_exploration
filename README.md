This project is a PlatformIO‐based playground for the Raspberry Pi Pico family. It combines small, self‑contained hardware demos. Initial focus is the Pico W, but will try more devices and peripherals in the future.

## Directory Structure

| Path             | Purpose / Contents                                                                                                                                                                                                                                                        |
| ---------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `platformio.ini` | PlatformIO configuration. Sets target board (`rpipicow`), Arduino core, monitor speed, and declares library dependencies such as **Adafruit NeoPixel**, **TFT_eSPI**, **PNGdec**, and **uRTCLib**. It also excludes the `src/Examples` folder from the default build.     |
| `include/`       | Placeholder for project headers (currently empty).                                                                                                                                                                                                                        |
| `lib/`           | Reserved for private libraries (empty).                                                                                                                                                                                                                                   |
| `src/`           | Application sources. Contains `main.cpp` and an `Examples/` folder.                                                                                                                                                                                                       |
| `src/Examples/`  | A collection of independent sketches that illustrate specific features (LED blinking, button–LED–buzzer interactions, NeoPixel animations, TFT graphics, RTTTL tunes, Wi‑Fi UDP, RTC setup, etc.). Also holds `resources/` with fonts (`Free_Fonts.h`) and sample images. |
| `test/`          | Placeholder for PlatformIO tests (none included yet).                                                                                                                                                                                                                     |
| `README.md`      | Brief introduction and list of available examples.                                                                                                                                                                                                                        |

## Examples

The `src/Examples` folder offers incremental demos that can be built individually by adjusting `build_src_filter` in `platformio.ini`. They cover:

- LED Blink: blinking led with a delay
- Button, led, buzzer: a button used to activate a buzzer and led (ON when pushed, OFF otherwise)
- Neopixel: controlling neopixel arrays with the Neopixel adafruit library
- Neopixel, button animations: enable disable different neopixel animations with the button
- Buzzer, button: RTTTL tones from different sources
- TFT LCD display: showing a png image using the eSPI library
- TFT LCD display, buttons: move through a gallery of png images
- TFT LCD display, buttons, rtttl tones: presentation of RTTTL titles, option to pick next tone and display of png image for each new tone

These sketches act as building blocks for the more integrated main program.

## Libraries

Libraries can be added in the `platform.io` file at the root folder, section `lib_deps`
