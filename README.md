This repo compiles explorations with the Raspberry Pi Pico family of microncontrollers. My initial focus is the Pico W, but I will try more devices and peripherals in the future.

## Examples

- Blink: blinking led with a delay
- Button, led, buzzer: a button used to activate a buzzer and led (ON when pushed, OFF otherwise)
- Neopixel: controlling neopixel arrays with the Neopixel adafruit library
- Neopixel, button animations: enable disable different neopixel animations with the button
- TFT LCD display: showing a png image using the eSPI library
- TFT LCD display, buttons: move through a gallery of png images

## Libraries

Libraries can be added in the `platform.io` file at the root folder, section `lib_deps`
