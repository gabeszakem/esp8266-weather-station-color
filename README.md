This forked copy has been adapted to be compatible with a NodeMCU 1.0 with different pin allocations for the TFT display interface. It uses a faster TFT_ILI9341_ESP library and includes Jpeg splash screen at startup.

<a href="http://imgur.com/vwvLz1T"><img src="http://i.imgur.com/vwvLz1T.png" title="source: imgur.com" /></a>

And tweaked weather screen:

<a href="http://imgur.com/xS7fWyw"><img src="http://i.imgur.com/xS7fWyw.png" title="source: imgur.com" /></a>




The original Squix repository ReadMe file follows, not that some of the text is no longer relevant...

# esp8266-weather-station-color

ESP8266 Weather Station in Color using ILI9341 TFT 240x320 display

## Hardware Requirements

This code is made for an 240x320 65K ILI9341 display with code running on an ESP8266.
You can buy such a display here: 

[http://www.banggood.com/2_2-Inch-Serial-TFT-SPI-Screen-p-912854.html](http://www.banggood.com/2_2-Inch-Serial-TFT-SPI-LCD-Screen-Module-HD-240-x-320-5110-Compatible-p-912854.html?p=6R31122484684201508S)

## Software Requirements/ Libraries

* Arduino IDE with ESP8266 platform installed
* [Weather Station Library](https://github.com/squix78/esp8266-weather-station) or through Library Manager
* [Adafruit ILI9341](https://github.com/adafruit/Adafruit_ILI9341) or through Library Manager
* [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) or through Library Manager
* [WifiManager](https://github.com/tzapu/WiFiManager)

You also need to get an API key for the Wunderground data: https://www.wunderground.com/

## Wiring

| ILI9341       | NodeMCU      |
| ------------- |:-------------:| 
| MISO          | -             | 
| LED           | 56 Ohms to 3V3 or 5V*           | 
| SCK           | D5            | 
| MOSI          | D7            |
| DC/RS         | D3            |
| RESET         | D4           |
| CS            | D8            |
| GND           | GND           |
| VCC           | 3V3           |

[*] Some 2.2" TFT displays require an LED current limit resistor of 56 Ohms when connected to 5V, others have a transistor switch so the LED can be PWM controlled by a digital pin to vary the backlight brightness.
