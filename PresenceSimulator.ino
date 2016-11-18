/* This sketch simulates presence by switching a fake TV or a lamp on and off. The simulator starts before sunset and ends at 10pm. 
   It reads sunset and actual time from the internet.
   If you pull GPIO0 to ground, it enters into setup mode wher you can enter credentials etc. Connect to the "SONOFF" WLAN and connect to 192.168.4.1
   
   This sketch also connects to the iopappstore and loads the assigned firmware down. The assignment is done on the server based on the MAC address of the board

    On the server, you need PHP script "iotappstoreV20.php" and the bin files are in the .\bin folder

    This work is based on the ESPhttpUpdate examples
    The setup routine is based on work done by John Lassen
    http://www.john-lassen.de/index.php/projects/esp-8266-arduino-ide-webconfig

  Copyright (c) [2016] [Andreas Spiess]

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

  Version 1.0

*/

#define VERSION "V1.0"
#define FIRMWARE "PresenceSimulator "VERSION
#include <credentials.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_NeoPixel.h>
#include "RemoteDebug.h"        //https://github.com/JoaoLopesF/RemoteDebug
#include <SNTPtime.h>
#include "data.h" // Output of Python script

#define EEPROM_SIZE 1024

#define SWITCH_PIN 12
#define PIXEL_PIN 14

#define NUM_LEDS 60

#define ON true
#define OFF false
#define GPIO0 0

#define VCC_ADJ 1.

#define MIN_CHANGE_MINUTES 7
#define MAX_CHANGE_MINUTES 15

#define RTCMEMBEGIN 68

extern "C" {
#include "user_interface.h" // this is for the RTC memory read/write functions
}

WiFiServer server(80);
ESP8266WebServer webServer(80);

strDateTime dateTime;
SNTPtime NTPch("ch.pool.ntp.org");

RemoteDebug Debug;

// remoteDebug
uint32_t mLastTime = 0;
uint32_t mTimeSeconds = 0;

enum statusFakeDef {
  TVonLoff,
  TVoffLon,
  TVonLon
} statusFake;

typedef struct {
  char ssid[20];
  char password[20];
  byte  IP[4];
  byte  Netmask[4];
  byte  Gateway[4];
  boolean dhcp;
  char constant1[50];
  char constant2[50];
  char constant3[50];
  char constant4[50];
  char constant5[50];
  char constant6[50];
  char IOTappStore1[40];
  char IOTappStorePHP1[40];
  char IOTappStore2[40];
  char IOTappStorePHP2[40];
  char magicBytes[4];
} strConfig;

strConfig config = {
  mySSID,
  myPASSWORD,
  0, 0, 0, 0,
  255, 255, 255, 0,
  192, 168, 0, 1,
  true,
  "constant1",
  "constant2",
  "constant3",
  "constant4",
  "constant5",
  "constant6",
  "192.168.0.200",
  "/iotappstore/iotappstorev20.php",
  "iotappstore.org",
  "/iotappstore/iotappstorev20.php",
  "CFG"
};
HTTPClient http;

#define  numPixels (sizeof(colors) / sizeof(colors[0]))
uint32_t pixelNum;
uint32_t totalTime, holdTime, fadeTime, startTime, elapsed;
uint16_t nr, ng, nb, r, g, b, i;
uint8_t  hi, lo, r8, g8, b8, frac;
uint16_t pr = 0, pg = 0, pb = 0; // Prev R, G, B

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIXEL_PIN, NEO_GRB);
#define  numPixels (sizeof(colors) / sizeof(colors[0]))

long delayCount = -1;
unsigned long entryTV = millis();
String ssid, password;
boolean fakeTV = OFF, scare = OFF, fakeLight = OFF, manual = OFF;
byte lastMinute,  actualMinute, nextChange;
String constant1, constant2, constant3, constant4, constant5, constant6, IOTappStore1, IOTappStorePHP1, IOTappStore2, IOTappStorePHP2;
long tts;

void espRestart(char mmode) {
  while (digitalRead(GPIO0) == OFF) yield();    // wait till GPIOo released
  delay(500);
  system_rtc_mem_write(RTCMEMBEGIN + 100, &mmode, 1);
  ESP.restart();
}

#include "ESPConfig.h"
#include "FakeTV.h"
#include "Sparkfun.h"

//-------------------------------------------------------------------

void setup() {
  char progMode;

  Serial.begin(115200);
  system_rtc_mem_read(RTCMEMBEGIN + 100, &progMode, 1);
  if (progMode == 'S') configESP();

  for (int i = 0; i < 5; i++) Serial.println("");
  Serial.println("Start "FIRMWARE);
  pinMode(GPIO0, INPUT_PULLUP);  // GPIO0 as input for Config mode selection

  pinMode(SWITCH_PIN, OUTPUT);
  // pinMode(3, OUTPUT);

  readRTCmem();
  rtcMem.bootTimes++;
  writeRTCmem();
  printRTCmem();

  readConfig();
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

  WiFi.begin(config.ssid, config.password);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (digitalRead(GPIO0) == OFF) espRestart('S');
    if (retries >= MAXRETRIES) espRestart('H');
    retries++;
  }
  Serial.println("");
  Serial.println("connected");
  if (iotUpdater(config.IOTappStore1, config.IOTappStorePHP1, FIRMWARE, false, true) == 0) {
    iotUpdater(config.IOTappStore2, config.IOTappStorePHP2, FIRMWARE, true, true);
  }
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");


  remoteDebugSetup();


  strip.begin();
  for (int i = 0; i < NUM_LEDS; i++) strip.setPixelColor(i, 0, 0, 0);
  strip.show();
  randomSeed(analogRead(A0));
  pixelNum = random(numPixels); // Begin at random point

  while (!NTPch.setSNTPtime()) Serial.print("."); // set internal clock
  Serial.println();
  dateTime = NTPch.getTime(1.0, 1); // get time from internal clock
  NTPch.printDateTime(dateTime);
  statusFake = TVonLon;
  scare = OFF;
  lastMinute = dateTime.minute;
  actualMinute = dateTime.minute;
  nextChange = addMinute(actualMinute, random(MIN_CHANGE_MINUTES, MAX_CHANGE_MINUTES));
  tts = timeToSunset(); // time to sunset

  Debug.println(config.constant3);
  sendSparkfun();

  ESP.wdtEnable(WDTO_8S);
}

//----------------------------------------------------------------------------

void loop() {
  ESP.wdtFeed();

  Debug.handle();

  dateTime = NTPch.getTime(1.0, 1); // get time from internal clock
  actualMinute = dateTime.minute;

  // adjust internal time and get differecne to sunset
  if ( abs(actualMinute - lastMinute) > 10) { // set exact time
    int retries = 0;
    while (!NTPch.setSNTPtime()) {
      retries++;
      if (retries > 20) ESP.restart();
      Serial.print("."); // set internal clock
    }
    Serial.println("");
    dateTime = NTPch.getTime(1.0, 1); // get time from internal clock
    lastMinute = dateTime.minute;
    actualMinute = dateTime.minute;

    if (Debug.ative(Debug.DEBUG)) {
      Debug.printf("Actual Time %u hour : %u minute\n", dateTime.hour, dateTime.minute);
    }
    tts = timeToSunset(); // time to sunset
    Serial.print("tts ");
    Serial.println(tts);
    NTPch.printDateTime(dateTime);
    Serial.println("Time adjusted");
  }
  if ((tts < 60 * 60 && dateTime.hour < 22) || manual) scare = ON; // switch the devices on or off
  else scare = OFF;

  if (digitalRead(GPIO0) == OFF) espRestart('S');

  if (WiFi.status() != WL_CONNECTED) espRestart('H');

  WiFiClient client = server.available();
  if (client) {
    Serial.println("new client");
    while (!client.available()) {
      delay(1);
    }

    // Read the first line of the request
    String request = client.readStringUntil('\r');
    Serial.print("Request ");
    Serial.println(request);
    client.flush();

    // Match the request

    if (request.indexOf("/SWITCH=ON") != -1) {
      delayCount = 0;    // last time an "ON" was received
      manual = ON;
      statusFake = nextStatus(statusFake); // set random status
      Serial.println("ON received ");
      //   digitalWrite(3,ON);
    }
    if (request.indexOf("/SWITCH=OFF") != -1) {
      delayCount = -1;
      digitalWrite(SWITCH_PIN, OFF);
      manual = OFF;
      Serial.println("OFF received ");
    }
    if (request.indexOf("/STATUS") != -1) {
      Serial.println("Status request ");
    }
    // Return the response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println(""); //  do not forget this one
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    //client.println("<br><br>");
    client.print("Manual: ");
    client.print(manual);
    client.print(" Scare: ");
    client.print(scare);
    client.print(" fakeTV: ");
    client.print(fakeTV);
    client.print(" fakeLight: ");
    client.print(fakeLight);
    client.print(" Status: ");
    client.print(statusFake);
    client.print(" Actual minute: ");
    client.print(actualMinute);
    client.print(" Next Change at: ");
    client.print(nextChange);
    client.print(" Next Change in: ");
    client.print(minuteDiff(actualMinute, nextChange));
    client.print(" minutes Diff ");
    client.print(tts);
    client.print(" minutes");
    client.println("<br><br>");
    client.println("Click <a href=\"/SWITCH=ON\">here</a> turn the SWITCH on pin 12 ON<br>");
    client.println("Click <a href=\"/SWITCH=OFF\">here</a> turn the SWITCH on pin 12 OFF<br>");
    client.println("Click <a href=\"/STATUS\">here</a> get status<br>");
    client.println("</html>");
    delay(1);
    Serial.println("Client disonnected");
    Serial.println("");
  }
  if (scare) {
    switch (statusFake) {
      case TVonLon:
        fakeTV = ON;
        fakeLight = ON;
        if (minuteDiff(actualMinute, nextChange) == 0) {
          nextChange = addMinute(actualMinute, random(MIN_CHANGE_MINUTES, MAX_CHANGE_MINUTES));
          statusFake = nextStatus(statusFake);
          printStatus();
        }
        break;

      case TVonLoff:
        fakeTV = ON;
        fakeLight = OFF;
        if (minuteDiff(actualMinute, nextChange) == 0) {
          nextChange = addMinute(actualMinute, random(MIN_CHANGE_MINUTES, MAX_CHANGE_MINUTES));
          statusFake = TVonLon;
          printStatus();
        }
        break;

      case TVoffLon:
        fakeTV = OFF;
        fakeLight = ON;
        if (minuteDiff(actualMinute, nextChange) == 0) {
          nextChange = addMinute(actualMinute, random(MIN_CHANGE_MINUTES, MAX_CHANGE_MINUTES));
          statusFake = TVonLon;
          printStatus();
        }
        break;

      default:
        break;
    }
  } else {
    fakeTV = OFF;
    fakeLight = OFF;
  }
  if (fakeTV) fakeTVsub();
  else {
    for (int i = 0; i < NUM_LEDS; i++) strip.setPixelColor(i, 0, 0, 0);
    strip.show();
  }
  if (fakeLight) digitalWrite(SWITCH_PIN, ON);
  else digitalWrite(SWITCH_PIN, OFF);

  // debugMessage();
}
// --------------------------- END LOOP --------------------------------

byte minuteDiff(byte actual, byte target) {
  int diff = target - actual;
  if (diff < 0) diff = diff + 60;
  return (diff);
}

int addMinute(byte actual, byte target) {
  int addi = target + actual;
  if (addi >= 60) return (addi - 60);
  else return (addi);
}

statusFakeDef nextStatus(statusFakeDef lastStatus) {
  statusFakeDef nextStatus;
  do {
    switch (random(0, 3)) {
      case 0: {
          nextStatus = TVoffLon;
          break;
        }
      case 1: {
          nextStatus = TVonLoff;
          break;
        }
      case 2: {
          nextStatus = TVonLon;
          break;
        }
    }
  } while (nextStatus == lastStatus);

  return nextStatus;
}

void printStatus() {
  NTPch.printDateTime(dateTime);
  Serial.print("Diff to Sunset ");
  Serial.print(tts);
  Serial.print(" Next Status ");
  Serial.print(statusFake);
  Serial.print(" at ");
  Serial.print(nextChange);
  Serial.print(" in  ");
  Serial.println(minuteDiff(actualMinute, nextChange));
}

long timeToSunset() {
  String _payload = "";
  long _diff, _sunset, _unixTime;
  String appID(config.constant1);
  String location(config.constant2);
  const String url = "http://api.openweathermap.org/data/2.5/weather?zip=" + location + "&APPID=" + appID;
  Serial.println(url);

  http.begin(url);

  int httpCode = http.GET();
  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      _payload = http.getString();
      //      Serial.println(_payload);
      int hi = _payload.indexOf("sunset");
      _payload = _payload.substring(hi + 8, hi + 18);
      _sunset = _payload.toInt();
      _unixTime = time(NULL);
      _diff = _sunset - _unixTime;
      //   Serial.println(_diff/3600);
      yield();
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    return -1;
  }
  http.end();
  yield();

  Serial.print("Sunset ");
  Serial.print(_sunset);
  Serial.print(" unixtime ");
  Serial.print(_unixTime);
  Serial.print(" _diff ");
  Serial.println(_diff / 3600.0);

  return _diff;
}

void debugMessage() {
  if ((millis() / 1000) % 5 == 0) { // Each 5 seconds

    // Debug levels

    if (Debug.ative(Debug.VERBOSE)) {
      Debug.println("* This is a message of debug level VERBOSE");
    }
    if (Debug.ative(Debug.DEBUG)) {
      Debug.println("* This is a message of debug level DEBUG");
    }
    if (Debug.ative(Debug.INFO)) {
      Debug.println("* This is a message of debug level INFO");
    }
    if (Debug.ative(Debug.WARNING)) {
      Debug.println("* This is a message of debug level WARNING");
    }
    if (Debug.ative(Debug.ERROR)) {
      Debug.println("* This is a message of debug level ERROR");
    }
  }
}

void remoteDebugSetup() {
  // remoteDebug
  // Register host name in WiFi and mDNS
  String hostNameWifi = config.constant3;
  hostNameWifi.concat(".local");
  WiFi.hostname(hostNameWifi);
  if (MDNS.begin(config.constant3)) {
    Serial.print("* MDNS responder started. http://");
    Serial.print(config.constant3);
    Serial.println(".local");
  }
  MDNS.addService("telnet", "tcp", 23);
  // Initialize the telnet server of RemoteDebug
  Debug.begin(config.constant3); // Initiaze the telnet server
  Debug.setResetCmdEnabled(true); // Enable the reset command
  // Debug.showProfiler(true); // To show profiler - time between messages of Debug
  // Good to "begin ...." and "end ...." messages
  // This sample (serial -> educattional use only, not need in production)

  // Debug.showTime(true); // To show time
}

