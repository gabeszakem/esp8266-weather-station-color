/**The MIT License (MIT)
Copyright (c) 2015 by Daniel Eichhorn
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
See more at http://blog.squix.ch

Adapted by Bodmer to use the faster TFT_ILI9341_ESP library:
https://github.com/Bodmer/TFT_ILI9341_ESP

*/
#include <simpleDSTadjust.h>

// Setup
const int UPDATE_INTERVAL_SECS = 10 * 60; // Update every 10 minutes

// HOSTNAME for OTA update
#define HOSTNAME "ESP8266-OTA-WEATHER-COLOR-ETKEZO"

// Pins for the TFT interface are defined in the User_Config.h file inside the TFT_ILI9341_ESP library
// These are the ones I used on a NodeMCU:
// #define TFT_DC D3
// #define TFT_CS D8
///#define TFT_RST -1 // Minus one means no pin allocated, connect to NodeMCU RST pin

// NTP settings Maximum of 3 servers
#define NTP_SERVERS "0.pool.ntp.org", "1.pool.ntp.org", "1.pool.ntp.org"

//DST rules for Central European Time Zone
#define UTC_OFFSET +1
struct dstRule StartRule = {"CEST", Last, Sun, Mar, 2, 3600}; // Central European Summer Time = UTC/GMT +2 hours
struct dstRule EndRule = {"CET", Last, Sun, Oct, 2, 0};       // Central European Time = UTC/GMT +1 hour

// Uncomment for 24 Hour style clock
#define STYLE_24HR




// Wunderground Settings, EDIT TO SUIT YOUR LOCATION
const boolean IS_METRIC = true; // Temperature only? Wind speed units appear to stay in mph. To do: investigate <<<<<<<<<<<<<<<<<<<<<<<<<
const String WUNDERGRROUND_API_KEY = "0fcd53ef95539db4";
const String WUNDERGRROUND_LANGUAGE = "HU";
const String WUNDERGROUND_COUNTRY = "HU";
const String WUNDERGROUND_CITY = "Dunaujvaros"; 

// Windspeed conversion, use 1 pair of #defines. To do: investigate a more convenient method <<<<<<<<<<<<<<<<<<<<<
//#define WIND_SPEED_SCALING 1.0      // mph
//#define WIND_SPEED_UNITS " mph"

//#define WIND_SPEED_SCALING 0.868976 // mph to knots
//#define WIND_SPEED_UNITS " kn"

#define WIND_SPEED_SCALING 1.60934  // mph to kph
//#define WIND_SPEED_UNITS " kph"
#define WIND_SPEED_UNITS " km/h"


//receive temperature signal from other device over udp connection USE UDP OR THINGSPEAK
const boolean USE_UDPSERVER=true;
const unsigned int UDPPort = 2807;
const int packetSize = 1024;

const unsigned int UDPPortNappali = 2807;
const IPAddress UDPNappaliIP =IPAddress(192,168,1,31);

//receive temperature signal rom other device over thingspeak connection USE UDP OR THINGSPEAK
/*Thingspeak communication to slow, not working time well. */
const boolean USE_THINGSPEAK=false;
//Thingspeak Settings - not used, no need to populate this at the moment
const unsigned long THINGSPEAK_CHANNEL_ID = 206323;
const char * THINGSPEAK_API_READ_KEY = "OJZPFFWD11FS3Z1M";

const int UPDATETHINGSPEAK = 60;

//Update frame time in second
const int UPDATEFRAMETIME = 10;

// List, so that the downloader knows what to fetch
String wundergroundIcons [] = {"chanceflurries","chancerain","chancesleet","chancesnow","clear","cloudy","flurries","fog","hazy","mostlycloudy","mostlysunny","partlycloudy","partlysunny","rain","sleet","snow","sunny","tstorms","unknown"};



/*const unsigned int UDPPort = 2807;
const IPAddress UDPNappaliIP =IPAddress(192,168,1,33);
/***************************
 * End Settings
 **************************/
