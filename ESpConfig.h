
#define MAGICBYTES "CFG"

extern "C" {
#include "user_interface.h" // this is for the RTC memory read/write functions
}

#define MAXRETRIES 50

unsigned long entry;


void writeConfig() {
  Serial.println("Writing Config");
  EEPROM.begin(EEPROM_SIZE);
  ssid.toCharArray(config.ssid, 20);
  password.toCharArray(config.password, 20);
  config.magicBytes[0] = MAGICBYTES[0];
  config.magicBytes[1] = MAGICBYTES[1];
  config.magicBytes[2] = MAGICBYTES[2];

  for (unsigned int t = 0; t < sizeof(config); t++) EEPROM.write(t, *((char*)&config + t));
  EEPROM.end();
}

boolean readConfig() {
  Serial.println("Reading Config");
  boolean ret = false;
  EEPROM.begin(EEPROM_SIZE);
  long magicBytesBegin = sizeof(config) - 4;

  if (EEPROM.read(magicBytesBegin) == MAGICBYTES[0] && EEPROM.read(magicBytesBegin + 1) == MAGICBYTES[1] && EEPROM.read(magicBytesBegin + 2) == MAGICBYTES[2]) {
    Serial.println(" Configuration found");
    for (unsigned int t = 0; t < sizeof(config); t++) *((char*)&config + t) = EEPROM.read(t);
    EEPROM.end();
    ssid = String(config.ssid);
    password = String(config.password);
    ret = true;

  } else {
    Serial.println("Configurarion NOT FOUND!!!!");
    writeConfig();
  }
  /*
    Serial.println(config.ssid);
    Serial.println(config.password);
    Serial.println(config.IP[0]);
    Serial.println(config.Netmask[0]);
    Serial.println(config.Gateway[0]);
    Serial.println(config.dhcp);
    Serial.println(config.constant1);
    Serial.println(config.constant2);
    Serial.println(config.constant3);
    Serial.println(config.constant4);
    Serial.println(config.constant5);
    Serial.println(config.constant6);
    Serial.println(config.IOTappStore1);
    Serial.println(config.IOTappStorePHP1);
    Serial.println(config.IOTappStore2);
    Serial.println(config.IOTappStorePHP2);
    Serial.println(config.magicBytes);
  */
}

// convert a single hex digit character to its integer value (from https://code.google.com/p/avr-netino/)
unsigned char h2int(char c)
{
  if (c >= '0' && c <= '9') {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}

String urldecode(String input) // (based on https://code.google.com/p/avr-netino/)
{
  char c;
  String ret = "";

  for (byte t = 0; t < input.length(); t++)
  {
    c = input[t];
    if (c == '+') c = ' ';
    if (c == '%') {


      t++;
      c = input[t];
      t++;
      c = (h2int(c) << 4) | h2int(input[t]);
    }

    ret.concat(c);
  }
  return ret;
}

//
// Check the Values is between 0-255
//
boolean checkRange(String Value)
{
  if (Value.toInt() < 0 || Value.toInt() > 255)
  {
    return false;
  }
  else
  {
    return true;
  }
}

#include "ESP_Helpers.h"
#include "Page_Root.h"
#include "Page_Admin.h"
#include "Page_Script.js.h"
#include "Page_Style.css.h"
#include "Page_Information.h"
#include "Page_applSettings.h"
#include "PAGE_NetworkConfiguration.h"

void serverSetup() {
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  // WiFi.mode(WIFI_AP);
  WiFi.softAP( "SONOFF");


  // Admin page
  webServer.on ( "/", []() {
    Serial.println("admin.html");
    webServer.send ( 200, "text/html", PAGE_AdminMainPage );  // const char top of page
  }  );

  webServer.on ( "/favicon.ico",   []() {
    Serial.println("favicon.ico");
    webServer.send ( 200, "text/html", "" );
  }  );

  // Network config
  webServer.on ( "/config.html", send_network_configuration_html );
  // Info Page
  webServer.on ( "/info.html", []() {
    Serial.println("info.html");
    webServer.send ( 200, "text/html", PAGE_Information );
  }  );

  webServer.on ( "/exit.html", send_exit_html);

  webServer.on ( "/appl.html", send_application_configuration_html  );
  webServer.on ( "/style.css", []() {
    Serial.println("style.css");
    webServer.send ( 200, "text/plain", PAGE_Style_css );
  } );
  webServer.on ( "/microajax.js", []() {
    Serial.println("microajax.js");
    webServer.send ( 200, "text/plain", PAGE_microajax_js );
  } );
  webServer.on ( "/admin/values", send_network_configuration_values_html );
  webServer.on ( "/admin/connectionstate", send_connection_state_values_html );
  webServer.on ( "/admin/infovalues", send_information_values_html );
  webServer.on ( "/admin/applvalues", send_application_configuration_values_html );


  webServer.onNotFound ( []() {
    Serial.println("Page Not Found");
    webServer.send ( 400, "text/html", "Page not Found" );
  }  );
  webServer.begin();
  Serial.println( "HTTP server started" );
}

void configESP() {
  int retries = 0;
  for (int i = 0; i < 5; i++) Serial.println("");
  Serial.println("Start Configuration");
  Serial.println("");
  Serial.println("");
  readConfig();
  entry = millis();

  while (digitalRead(GPIO0) == OFF) {
    delay(100);
    yield();    // wait till GPIOo released
  }
  serverSetup();

  while (millis() - entry < 70000) {
    webServer.handleClient();
    yield(); // For ESP8266 to not dump
    if (digitalRead(GPIO0) == OFF) {
      espRestart('H');
    }
    ESP.wdtFeed();
  }
  espRestart('H');
}
