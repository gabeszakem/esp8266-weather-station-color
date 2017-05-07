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
  AUTHORS OR COPYBR_DATUM HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
  See more at http://blog.squix.ch

  Adapted by Bodmer to use the faster TFT_ILI9341_ESP library:
  https://github.com/Bodmer/TFT_ILI9341_ESP

  Plus:
  Minor changes to text placement and auto-blanking out old text with background colour padding
  Moon phase text added
  Forecast text lines are automatically split onto two lines at a central space (some are long!)
  Time is printed with colons aligned to tidy display
  Min and max forecast temperatures spaced out
  The ` character has been changed to a degree symbol in the 36 point font
  New smart WU splash startup screen and updated progress messages
  Display does not need to be blanked between updates
  Icons nudged about slightly to add wind direction

  Adapted by Gabeszakem basic of my progrem from Bodmer's version. :
  https://github.com/Bodmer/esp8266-weather-station-color
  Changed time display, show time every secund without blinking. 
  Changed time to dst. Adapted from https://github.com/neptune2/esp8266-weather-station-oled-DST project.
  Made a headline . Adapted wifi quality signal from https://github.com/neptune2/esp8266-weather-station-oled-DST project.
  Made possibility to change language (Hungarian or English) depend of using WUNDERGRROUND_LANGUAGE setup. 
  Created new screens. Next 3 day show weather text information.
  New Page show information from other temerature sensor. Wait data over UDP or Thingspeak.
  Thingspeak communicaciton slow so program prefer UDP communication.

*/

#include <Arduino.h>

#include <SPI.h>
#include <TFT_ILI9341_ESP.h> // Hardware-specific library

// Additional UI functions
#include "GfxUi.h"

// Fonts created by http://oleddisplay.squix.ch/
#include "ArialRoundedMTBold_14.h"
#include "ArialRoundedMTBold_36.h"
#include "DSEG7ClassicBold_44.h"

// Download helper
#include "WebResource.h"

#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

// Helps with connecting to internet
#include <WiFiManager.h>

// check settings.h for adapting to your needs
#include "settings.h"
#include <JsonListener.h>
#include <WundergroundClient.h>

  #include "ThingSpeak.h"


#include <simpleDSTadjust.h>
#include <time.h>

#include <WiFiUdp.h>
#include <ArduinoJson.h>



/*****************************
   Important: see settings.h to configure your settings!!!
 * ***************************/

TFT_ILI9341_ESP tft = TFT_ILI9341_ESP();       // Invoke custom library

boolean booted = true;
boolean updated = false;

GfxUi ui = GfxUi(&tft);

WebResource webResource;
// Setup simpleDSTadjust Library rules
simpleDSTadjust dstAdjusted(StartRule, EndRule);

// Set to false, if you prefere imperial/inches, Fahrenheit
WundergroundClient wunderground(IS_METRIC);
//ThingSpeak


//declaring prototypes
void configModeCallback (WiFiManager *myWiFiManager);
void downloadCallback(String filename, int16_t bytesDownloaded, int16_t bytesTotal);
ProgressCallback _downloadCallback = downloadCallback;
void downloadResources();
void updateData();
void drawProgress(uint8_t percentage, String text);
void drawTime();
void drawCurrentWeather();
void drawForecast();
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex);
String getMeteoconIcon(String iconText);
void drawAstronomy();
void drawSeparator(uint16_t y);
String replace(String text);

long lastDownloadUpdate = millis();
long lastFrameUpdate = millis();
long lastThingSpeakUpdate =-60000;

int numberOfFrames = 5;
int actualFrame = 0;
String temperatureValues[4];
String temperatureNames[4];
String humidityValues[4];



String lastDate = "1";

  WiFiClient  client;

  WiFiUDP UDPServer;
  
  byte packetBuffer[packetSize];

void setup() {
  Serial.begin(250000);
  tft.begin();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(2);
  tft.setFreeFont(&ArialRoundedMTBold_14);
  tft.setTextDatum(BC_DATUM);


  SPIFFS.begin();
  //listFiles();
  //Uncomment if you want to erase SPIFFS and update all internet resources, this takes some time!
  //tft.drawString("Formatting SPIFFS, so wait!", 120, 200); SPIFFS.format();

  if (SPIFFS.exists("/WU.jpg") == true) ui.drawJpeg("/WU.jpg", 0, 10);
  if (SPIFFS.exists("/Earth.jpg") == true) ui.drawJpeg("/Earth.jpg", 0, 320 - 56); // Image is 56 pixels high

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("Original by: blog.squix.org", 120, 220);
  tft.drawString("Adapted by: Bodmer", 120, 240);
  tft.drawString("Adapted from: neptune2", 120, 260);
  tft.drawString("Modified by: Gabeszakem", 120, 280);
  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  delay(1000);
  tft.drawString("Connecting to WiFi", 120, 200);
  tft.setTextPadding(240); // Pad next drawString() text to full width to over-write old text

  //screenshotToConsole();

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  // Uncomment for testing wifi manager
  //wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);

  //or use this for auto generated name ESP + ChipID
  wifiManager.autoConnect();

  //Manual Wifi
  //WiFi.begin(WIFI_SSID, WIFI_PWD);

  // OTA Setup
  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.begin();

  // download images from the net. If images already exist don't download
  tft.drawString("Downloading to SPIFFS...", 120, 200);
  tft.drawString(" ", 120, 240);  // Clear line
  tft.drawString(" ", 120, 260);  // Clear line
  downloadResources();
  //listFiles();
  tft.drawString(" ", 120, 200);  // Clear line above using set padding width
  tft.drawString("Fetching weather data...", 120, 220);
  //delay(500);
  if( USE_THINGSPEAK ) ThingSpeak.begin(client);


  if( USE_UDPSERVER ) UDPServer.begin(UDPPort);
  // load the weather information
  updateData();

}

long lastDrew = 0;
void loop() {
  // Handle OTA update requests
  ArduinoOTA.handle();

  // Check if we should update the clock
  //if (millis() - lastDrew > 30000 && wunderground.getSeconds() == "00") {
  if (millis() - lastDrew > 999) {
    drawTime();
    lastDrew = millis();
    updated = false;
  }

  // Check if we should update weather information
  if (millis() - lastDownloadUpdate > 1000 * UPDATE_INTERVAL_SECS) {
    updateData();
    lastDownloadUpdate = millis();

  }

  if (millis() - lastFrameUpdate > UPDATEFRAMETIME * 1000) {
    drawWeatherText();
    lastFrameUpdate = millis();
  }

  if(USE_UDPSERVER)  handleUDPServer();
}

// Called if WiFi has not been configured yet
void configModeCallback (WiFiManager *myWiFiManager) {
  tft.setTextDatum(BC_DATUM);
  tft.setFreeFont(&ArialRoundedMTBold_14);
  tft.setTextColor(TFT_GREENYELLOW);
  tft.drawString("Wifi Manager", 120, 28);
  tft.drawString("Please connect to AP", 120, 42);
  tft.setTextColor(TFT_WHITE);
  tft.drawString(myWiFiManager->getConfigPortalSSID(), 120, 56);
  tft.setTextColor(TFT_GREENYELLOW);
  tft.drawString("To setup Wifi Configuration", 120, 70);
}

// callback called during download of files. Updates progress bar
void downloadCallback(String filename, int16_t bytesDownloaded, int16_t bytesTotal) {
  Serial.println(String(bytesDownloaded) + " / " + String(bytesTotal));

  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  tft.setTextPadding(240);

  int percentage = 100 * bytesDownloaded / bytesTotal;
  if (percentage == 0) {
    tft.drawString(filename, 120, 220);
  }
  if (percentage % 5 == 0) {
    tft.setTextDatum(TC_DATUM);
    tft.setTextPadding(tft.textWidth(" 888% "));
    tft.drawString(String(percentage) + "%", 120, 245);
    ui.drawProgressBar(10, 225, 240 - 20, 15, percentage, TFT_WHITE, TFT_BLUE);
  }

}

// Download the bitmaps
void downloadResources() {
  // tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(&ArialRoundedMTBold_14);
  char id[5];

  // Download WU graphic jpeg first and display it, then the Earth view
  webResource.downloadFile((String)"http://i.imgur.com/njl1pMj.jpg", (String)"/WU.jpg", _downloadCallback);
  if (SPIFFS.exists("/WU.jpg") == true) ui.drawJpeg("/WU.jpg", 0, 10);

  webResource.downloadFile((String)"http://i.imgur.com/v4eTLCC.jpg", (String)"/Earth.jpg", _downloadCallback);
  if (SPIFFS.exists("/Earth.jpg") == true) ui.drawJpeg("/Earth.jpg", 0, 320 - 56);

  //webResource.downloadFile((String)"http://i.imgur.com/IY57GSv.jpg", (String)"/Horizon.jpg", _downloadCallback);
  //if (SPIFFS.exists("/Horizon.jpg") == true) ui.drawJpeg("/Horizon.jpg", 0, 320-160);

  //webResource.downloadFile((String)"http://i.imgur.com/jZptbtY.jpg", (String)"/Rainbow.jpg", _downloadCallback);
  //if (SPIFFS.exists("/Rainbow.jpg") == true) ui.drawJpeg("/Rainbow.jpg", 0, 0);

  for (int i = 0; i < 19; i++) {
    sprintf(id, "%02d", i);
    webResource.downloadFile("http://www.squix.org/blog/wunderground/" + wundergroundIcons[i] + ".bmp", wundergroundIcons[i] + ".bmp", _downloadCallback);
  }
  for (int i = 0; i < 19; i++) {
    sprintf(id, "%02d", i);
    webResource.downloadFile("http://www.squix.org/blog/wunderground/mini/" + wundergroundIcons[i] + ".bmp", "/mini/" + wundergroundIcons[i] + ".bmp", _downloadCallback);
  }
  for (int i = 0; i < 24; i++) {
    webResource.downloadFile("http://www.squix.org/blog/moonphase_L" + String(i) + ".bmp", "/moon" + String(i) + ".bmp", _downloadCallback);
  }
}

// Update the internet based information and update screen
void updateData() {
  // booted = true;  // Test only
  // booted = false; // Test only

  if (booted) ui.drawJpeg("/WU.jpg", 0, 10); // May have already drawn this but it does not take long
  else tft.drawCircle(200, 6, 6, TFT_CYAN); // Outer ring - optional

  if (booted) drawProgress(20, "Updating time...");
  else fillSegment(200, 6, 0, (int) (20 * 3.6), 6, TFT_CYAN);

  configTime(UTC_OFFSET * 3600, 0, NTP_SERVERS);

  if (booted) drawProgress(50, "Updating conditions...");
  else fillSegment(200, 6, 0, (int) (50 * 3.6), 6, TFT_CYAN);

  wunderground.updateConditions(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);

  if (booted) drawProgress(70, "Updating forecasts...");
  else fillSegment(200, 6, 0, (int) (60 * 3.6), 6, TFT_CYAN);

  wunderground.updateForecast(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);

  if (booted) {
    if(USE_THINGSPEAK){
      drawProgress(80, "Updating thingspeak...");
      freshThingspeakValues();
    }

  }else {
    fillSegment(200, 6, 0, (int) (70 * 3.6), 6, TFT_CYAN);
  }
  
    if (booted) {
     drawProgress(90, "Updating astronomy...");
   }else {
    fillSegment(200, 6, 0, (int) (70 * 3.6), 6, TFT_CYAN);
  }

  wunderground.updateAstronomy(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
  // lastUpdate = timeClient.getFormattedTime();
  // readyForWeatherUpdate = false;
  if (booted) drawProgress(100, "Done...");
  else fillSegment(200, 6, 0, 360, 6, TFT_CYAN);
  
  

  if (booted) delay(2000);

  if (booted) tft.fillScreen(TFT_BLACK);
  else   fillSegment(200, 6, 0, 360, 8, TFT_BLACK);

  if (booted) lastFrameUpdate = millis();

  //tft.fillScreen(TFT_CYAN); // For text padding and update graphics over-write checking only
  drawTime();
  drawCurrentWeather();
  if (booted) drawWeatherText();
  //drawForecast();
  //drawAstronomy();

  //if (booted) screenshotToConsole(); // Documentation support only!
  booted = false;
  updated = true;
}

// Progress bar helper
void drawProgress(uint8_t percentage, String text) {
  tft.setFreeFont(&ArialRoundedMTBold_14);

  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  tft.setTextPadding(240);
  tft.drawString(text, 120, 220);

  ui.drawProgressBar(10, 225, 240 - 20, 15, percentage, TFT_WHITE, TFT_BLUE);

  tft.setTextPadding(0);
}

// draws the clock
void drawTime() {

  char *dstAbbrev;
  time_t now = dstAdjusted.time(&dstAbbrev);

  struct tm * timeinfo = localtime (&now);
  String date = ctime(&now);
  date = date.substring(0, 11) + String(1900 + timeinfo->tm_year);
  char time[11];
  char sec[4];

#ifdef STYLE_24HR
  int hour = timeinfo->tm_hour;
  //sprintf(time, "%02d:%02d", hour, timeinfo->tm_min);
  sprintf(time, "%02d:%02d:%02d", hour, timeinfo->tm_min, timeinfo->tm_sec);
  sprintf(sec, "%02d", timeinfo->tm_sec);
#else
  int hour = (timeinfo->tm_hour + 11) % 12 + 1; // take care of noon and midnight
  //sprintf(time, "%d:%02d", hour, timeinfo->tm_min);
  sprintf(time, "%02d:%02d:%02d", hour, timeinfo->tm_min, timeinfo->tm_sec);
  sprintf(sec, "%02d", timeinfo->tm_sec);

#endif


  tft.setFreeFont(&ArialRoundedMTBold_14);
  if (!date.equals(lastDate) || updated) {
    tft.setTextDatum(BL_DATUM);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextPadding(tft.textWidth(" Ddd, 44 Mmm 4444 "));  // String width + margin
    //tft.drawString(date, 120, 14);
    tft.drawString(date, 5, 14);

  }
  lastDate = date;

  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" IIII "));  // String width + margin

  int8_t quality = getWifiQuality();
  for (int8_t i = 0; i < 5; i++) {
    for (int8_t j = 0; j < 2 * (i + 1); j++) {
      if (quality > i * 20 || j == 0) {
        tft.drawPixel (220 + 4 * i, 10 - j, TFT_CYAN );
        tft.drawPixel (221 + 4 * i, 10 - j, TFT_CYAN );
      }
    }
  }
  drawSeparator(14);
  drawSeparator(15);

  tft.setFreeFont(&ArialRoundedMTBold_36);
  //tft.setFreeFont(&DSEG7ClassicBold_44);
  tft.setTextDatum(L_BASELINE);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  if (strcmp(sec, "00") == 0 || updated) {
    tft.setTextPadding(tft.textWidth("  44:44:44  "));  // String width + margin
    //tft.drawString(time, 120, 50);
    tft.drawString(time, 40, 56);
  } else {
    tft.setTextPadding(tft.textWidth("44"));  // String width + margin
    tft.drawString(sec, 154, 56);
  }

  //drawSeparator(56);

  tft.setTextPadding(0);
}

// draws current weather information
void drawCurrentWeather() {
  // Weather Icon
  String weatherIcon = getMeteoconIcon(wunderground.getTodayIcon());
  //uint32_t dt = millis();
  ui.drawBmp(weatherIcon + ".bmp", 10, 59);//ui.drawBmp(weatherIcon + ".bmp", 0, 59)
  //Serial.print("Icon draw time = "); Serial.println(millis()-dt);

  String weatherText = wunderground.getWeatherText();
  Serial.println("weathertext :: " + weatherText);
  weatherText = replace(weatherText);
  //weatherText = "Heavy Thunderstorms with Small Hail"; // Test line splitting with longest(?) string

  tft.setFreeFont(&ArialRoundedMTBold_14);

  tft.setTextDatum(BL_DATUM);
  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);

  int splitPoint = 0;
  int xpos = 120;//int xpos = 230;
  splitPoint =  splitIndex(weatherText);
  if (splitPoint > 16) xpos = 120;//if (splitPoint > 16) xpos = 235;

  tft.setTextPadding(tft.textWidth(" Heavy Thunderstorms"));  // Max anticipated string width + margin
  if (splitPoint) {
	  tft.drawString(weatherText.substring(0, splitPoint), xpos, 80);
  }else{
	  tft.drawString(" ", xpos, 80);
  }
  //tft.setTextPadding(tft.textWidth(weatherText.substring(splitPoint)+"  "));  // Max anticipated string width + margin
  tft.setTextPadding(tft.textWidth(" Heavy Thunderstorms"));
  tft.drawString(weatherText.substring(splitPoint), xpos, 95);

  tft.setFreeFont(&ArialRoundedMTBold_36);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("882333C"));

  // Font ASCII code 96 (0x60) modified to make "`" a degree symbol
  weatherText = wunderground.getCurrentTemp();
  //if (weatherText.indexOf(".")) weatherText = weatherText.substring(0, weatherText.indexOf(".")); // Make it integer temperature
  if (weatherText == "") weatherText = "?";  // Handle null return
  String metric = "";
  if (IS_METRIC) metric = "C " ;
  else metric = "F ";

  tft.drawString(weatherText + "`" + metric, 120, 110);//tft.drawString(weatherText + "`", 221, 100);

  tft.setFreeFont(&ArialRoundedMTBold_14);
  tft.setTextPadding(0); // Reset padding width to none
  drawSeparator(149);
}

// draws the three forecast columns
void drawForecast() {
  drawForecastDetail(10, 171, 0);
  drawForecastDetail(95, 171, 2);
  drawForecastDetail(180, 171, 4);
  //drawSeparator(171 + 69);
}

// helper for the forecast columns
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex) {
  tft.setFreeFont(&ArialRoundedMTBold_14);

  String day = wunderground.getForecastTitle(dayIndex).substring(0, 12);
  day = replace(day);
  //day.toUpperCase();

  tft.setTextDatum(BC_DATUM);

  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("Csutortok"));
  tft.drawString(day, x + 25, y);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("-88   -88"));
  tft.drawString(wunderground.getForecastLowTemp(dayIndex) + "|" + wunderground.getForecastHighTemp(dayIndex), x + 25, y + 14);

  String weatherIcon = getMeteoconIcon(wunderground.getForecastIcon(dayIndex));
  ui.drawBmp("/mini/" + weatherIcon + ".bmp", x, y + 15);

  tft.setTextPadding(0); // Reset padding width to none

}

void drawForecastText(int dayIndex) {

  String forecastText = replace(wunderground.getForecastText(dayIndex));
  String day = replace(wunderground.getForecastTitle(dayIndex));
  tft.setFreeFont(&ArialRoundedMTBold_36);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  tft.drawString(day, 120, 170);

  String weatherIcon = getMeteoconIcon(wunderground.getForecastIcon(dayIndex));
  ui.drawBmp("/mini/" + weatherIcon + ".bmp", 0, 210);


  tft.setFreeFont(&ArialRoundedMTBold_14);
  tft.setTextDatum(BL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  int pointerStart=0;
  int pointerStop=20;
  int length=20;
  Serial.println(forecastText);
  int y=230;
  int x;
  for ( int i = 0; i <= 6; i++) {
	if (i < 3) {
		length=22; 	
		x=60;
    } else {
		length=27;
		x=10;
	}
	pointerStop=pointerStart+length;
	if(pointerStop <= forecastText.length()){
		int pointer=forecastText.substring(pointerStart,pointerStop).lastIndexOf(" ");	
		pointerStop=pointerStart+pointer;
	}else{
		pointerStop=forecastText.length();
	}
	//Serial.println("forecastText.substring("+String(pointerStart)+","+String(pointerStop)+")");
	//Serial.println(forecastText.substring(pointerStart,pointerStop));
	tft.drawString(forecastText.substring(pointerStart,pointerStop),x ,y+ (i * 15));
	pointerStart=pointerStop+1;	
  }
  
}

void drawThingspeak(){

  tft.setTextDatum(BC_DATUM);
  tft.setFreeFont(&ArialRoundedMTBold_36);
  int16_t H=32;
  int16_t L=57;

  tft.drawEllipse(60, 200,L, H,TFT_GREENYELLOW); 
  tft.drawEllipse(180, 200, L,H, TFT_GREENYELLOW); 
  tft.drawEllipse(60, 280, L, H,TFT_GREENYELLOW); 
  tft.drawEllipse(180, 280,L,H, TFT_GREENYELLOW); 

  //Temperature from thinkspeak
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString(temperatureValues[0].substring(0,temperatureValues[0].lastIndexOf(".")+2), 60, 220);
  tft.drawString(temperatureValues[1].substring(0,temperatureValues[1].lastIndexOf(".")+2), 180, 220);
  tft.drawString(temperatureValues[2].substring(0,temperatureValues[2].lastIndexOf(".")+2), 60, 300);
  tft.drawString(temperatureValues[3].substring(0,temperatureValues[3].lastIndexOf(".")+2), 180, 300);

  //Name of additional temperature ( Value get from thinkspeak)
  tft.setFreeFont(&ArialRoundedMTBold_14);
  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  tft.drawString(temperatureNames[0],60,180);
  tft.drawString(temperatureNames[1],180,180);
  tft.drawString(temperatureNames[2],60,260);
  tft.drawString(temperatureNames[3],180,260);

  //Humidity from thingspeak
  tft.setFreeFont(&ArialRoundedMTBold_14);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(humidityValues[0]+"%",60,234);
  tft.drawString(humidityValues[1]+"%",180,234);
  tft.drawString(humidityValues[2]+"%",60,314);
  tft.drawString(humidityValues[3]+"%",180,314);

}

// draw moonphase and sunrise/set and moonrise/set
void drawAstronomy() {
  tft.setFreeFont(&ArialRoundedMTBold_14);
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" Waxing Crescent "));
  String moonPhase = replace(wunderground.getMoonPhase());
  //tft.drawString(wunderground.getMoonPhase(), 120, 260 - 2);
  tft.drawString(moonPhase, 120, 260 - 2);


  int moonAgeImage = 24 * wunderground.getMoonAge().toInt() / 30.0;
  ui.drawBmp("/moon" + String(moonAgeImage) + ".bmp", 120 - 30, 260);

  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  tft.setTextPadding(0); // Reset padding width to none
  (WUNDERGRROUND_LANGUAGE == "HU") ? tft.drawString("Nap", 40, 280) : tft.drawString("Sun", 40, 280);


  tft.setTextDatum(BR_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" 88:88 "));
  int dt = rightOffset(wunderground.getSunriseTime(), ":"); // Draw relative to colon to them aligned
  tft.drawString(wunderground.getSunriseTime(), 40 + dt, 300);

  dt = rightOffset(wunderground.getSunsetTime(), ":");
  tft.drawString(wunderground.getSunsetTime(), 40 + dt, 315);

  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  tft.setTextPadding(0); // Reset padding width to none
  (WUNDERGRROUND_LANGUAGE == "HU") ? tft.drawString("Hold", 200, 280):tft.drawString("Moon", 200, 280);

  tft.setTextDatum(BR_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" 88:88 "));
  dt = rightOffset(wunderground.getMoonriseTime(), ":"); // Draw relative to colon to them aligned
  tft.drawString(wunderground.getMoonriseTime(), 200 + dt, 300);

  dt = rightOffset(wunderground.getMoonsetTime(), ":");
  tft.drawString(wunderground.getMoonsetTime(), 200 + dt, 315);

  tft.setTextPadding(0); // Reset padding width to none
}

// Helper function, should be part of the weather station library and should disappear soon
String getMeteoconIcon(String iconText) {
  if (iconText == "F") return "chanceflurries";
  if (iconText == "Q") return "chancerain";
  if (iconText == "W") return "chancesleet";
  if (iconText == "V") return "chancesnow";
  if (iconText == "S") return "chancestorms";
  //if (iconText == "S") return "tstorms";
  if (iconText == "B") return "clear";
  if (iconText == "Y") return "cloudy";
  if (iconText == "F") return "flurries";
  if (iconText == "M") return "fog";
  if (iconText == "E") return "hazy";
  if (iconText == "Y") return "mostlycloudy";
  if (iconText == "H") return "mostlysunny";
  if (iconText == "H") return "partlycloudy";
  if (iconText == "J") return "partlysunny";
  if (iconText == "W") return "sleet";
  if (iconText == "R") return "rain";
  if (iconText == "W") return "snow";
  if (iconText == "B") return "sunny";
  if (iconText == "0") return "tstorms";


  return "unknown";
}

// if you want separators, uncomment the tft-line
void drawSeparator(uint16_t y) {
  // tft.drawFastHLine(10, y, 240 - 2 * 10, 0x4228);
  tft.drawFastHLine(0, y, 240 * 10, TFT_CYAN);

}

// determine the "space" split point in a long string
int splitIndex(String text)
{
  int index = 0;
  while ( (text.indexOf(' ', index) >= 0) && ( index <= text.length() / 2 ) ) {
    index = text.indexOf(' ', index) + 1;
  }
  if (index) index--;
  return index;
}

// Calculate coord delta from start of text String to start of sub String contained within that text
// Can be used to vertically right align text so for example a colon ":" in the time value is always
// plotted at same point on the screen irrespective of different proportional character widths,
// could also be used to align decimal points for neat formatting
int rightOffset(String text, String sub)
{
  int index = text.indexOf(sub);
  return tft.textWidth(text.substring(index));
}

// Calculate coord delta from start of text String to start of sub String contained within that text
// Can be used to vertically left align text so for example a colon ":" in the time value is always
// plotted at same point on the screen irrespective of different proportional character widths,
// could also be used to align decimal points for neat formatting
int leftOffset(String text, String sub)
{
  int index = text.indexOf(sub);
  return tft.textWidth(text.substring(0, index));
}

// Draw a segment of a circle, centred on x,y with defined start_angle and subtended sub_angle
// Angles are defined in a clockwise direction with 0 at top
// Segment has radius r and it is plotted in defined colour
// Can be used for pie charts etc, in this sketch it is used for wind direction
#define DEG2RAD 0.0174532925 // Degrees to Radians conversion factor
#define INC 2 // Minimum segment subtended angle and plotting angle increment (in degrees)
void fillSegment(int x, int y, int start_angle, int sub_angle, int r, unsigned int colour)
{
  // Calculate first pair of coordinates for segment start
  float sx = cos((start_angle - 90) * DEG2RAD);
  float sy = sin((start_angle - 90) * DEG2RAD);
  uint16_t x1 = sx * r + x;
  uint16_t y1 = sy * r + y;

  // Draw colour blocks every INC degrees
  for (int i = start_angle; i < start_angle + sub_angle; i += INC) {

    // Calculate pair of coordinates for segment end
    int x2 = cos((i + 1 - 90) * DEG2RAD) * r + x;
    int y2 = sin((i + 1 - 90) * DEG2RAD) * r + y;

    tft.fillTriangle(x1, y1, x2, y2, x, y, colour);

    // Copy segment end to sgement start for next segment
    x1 = x2;
    y1 = y2;
  }
}
//Change hungarian specific characters bicouse missing from fonts
String replace(String text) {
  text.replace("ő", "o");
  text.replace("Ő", "O");
  text.replace("&#337;", "o");
  text.replace("í", "i");
  text.replace("Í", "I");
  text.replace("É", "E");
  text.replace("é", "e");
  text.replace("Á", "A");
  text.replace("á", "a");
  text.replace("Ó", "O");
  text.replace("ó", "o");
  text.replace("ő", "o");
  text.replace("Ő", "O");
  text.replace("í", "i");
  text.replace("Í", "I");
  text.replace("É", "E");
  text.replace("Ü", "U");
  text.replace("ü", "u");
  text.replace("Ű", "U");
  text.replace("ű", "u");
  text.replace("Ú", "U");
  text.replace("ú", "u");
  text.replace("Ö", "O");
  text.replace("ö", "o");
  text.replace("°", "`");
  return text;
}

int8_t getWifiQuality() {
  int32_t dbm = WiFi.RSSI();
  if (dbm <= -100) {
    return 0;
  } else if (dbm >= -50) {
    return 100;
  } else {
    return 2 * (dbm + 100);
  }
}

void drawWeatherText() {
  if (actualFrame == numberOfFrames) {
    actualFrame = 1;
  } else {
    actualFrame++;
  }
  tft.fillRect(0, 150, 240, 220, TFT_BLACK);
  //actualFrame=5;


  switch (actualFrame) {

    case 1:
      drawForecast();
      drawAstronomy();
      break;

    case 2:
      drawForecastText(0);
      break;
    case 3:
      drawForecastText(2);
      break;
    case 4:
      drawForecastText(4);
      break;
	case 5:
	//drawForecast();
	drawThingspeak();
    default:
      break;
  }
  if (USE_THINGSPEAK){
    freshThingspeakValues();
  }
  
}

void checkRefreshTime(){
  if (millis() - lastDownloadUpdate > 1000 * UPDATE_INTERVAL_SECS) {
    drawTime();
    lastDownloadUpdate = millis();
  }
}

void freshThingspeakValues(){
  if(millis()-lastThingSpeakUpdate >= UPDATETHINGSPEAK * 1000 ){
    if (!booted){
      tft.setFreeFont(&ArialRoundedMTBold_14);
      tft.setTextDatum(BC_DATUM);
      tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      tft.drawCircle(188, 6, 6, TFT_CYAN);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString("T", 188, 14);
    }
    Serial.println("thingspeak update");
    for(int i=0;i<8;i++){
      temperatureValues[i] = ThingSpeak.readStringField(THINGSPEAK_CHANNEL_ID,i+1,THINGSPEAK_API_READ_KEY);
      if (!booted)drawTime();
    }
    tft.drawCircle(188, 6, 6, TFT_BLACK);
      tft.setTextColor(TFT_BLACK, TFT_BLACK);
      tft.drawString("T", 188, 14);
    lastThingSpeakUpdate=millis();
  }
  
}

void handleUDPServer() {
  int cb = UDPServer.parsePacket();
  if (cb) {
    UDPServer.read(packetBuffer, packetSize);
    String myData = ""; 
    for(int i = 0; i < packetSize; i++) {
      myData += (char)packetBuffer[i];
    }
    Serial.println(myData);
    parseText(myData);
  }
}
//https://techtutorialsx.com/2016/07/30/esp8266-parsing-json/
//EXAMPLE:  { "ChannelNumber":1,"SensorType": Nappali, "Temperature": 10, "Humidity":50 }
void parseText(String myData){
  //char data[] = myData.substring(0, myData.indexOf('\0'));
  char data[100];
  myData.toCharArray(data,100);

  StaticJsonBuffer<packetSize> JSONBuffer;   //Memory pool
  JsonObject& parsed = JSONBuffer.parseObject(data); //Parse message
 
  if (!parsed.success()) {   //Check for errors in parsing
 
    Serial.println("Parsing failed");
  }else{

    String channelNumber = parsed["ChannelNumber"];
    Serial.println("ChannelNumber:"+channelNumber);
    String sensorType = parsed["SensorType"];
    String temp = parsed["Temperature"];
    String humidity = parsed["Humidity"];
    Serial.println("SensorType:"+sensorType);
    Serial.println("Temperature:"+temp);
    Serial.println("Humidity:"+humidity);

    if (channelNumber.equals("0")){
      
      temperatureNames[0] = sensorType;
      temperatureValues[0] = temp;
      humidityValues[0] = humidity;
    }
    if (channelNumber.equals("1")){

      temperatureNames[1] = sensorType;
      temperatureValues[1] = temp;
      humidityValues[1] = humidity;
    }
    if (channelNumber.equals("2")){

      temperatureNames[2] = sensorType;
      temperatureValues[2] = temp;
      humidityValues[2] = humidity;
    }
    if (channelNumber.equals("3")){

      temperatureNames[3] = sensorType;
      temperatureValues[3] = temp;
      humidityValues[3] = humidity;
    }
  }
}
