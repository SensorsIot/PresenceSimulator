
#define MAGICBYTE 85


typedef struct {
  byte markerFlag;
  int bootTimes;
  int runSpaces;
  long lastSubscribers;
} 
rtcMemDef __attribute__((aligned(4)));

rtcMemDef rtcMem = {
  MAGICBYTE,
  9999,
  9999,
  0
};

/*
 void handleTelnet() {
  if (TelnetServer.hasClient()) {
    // client is connected
    if (!Telnet || !Telnet.connected()) {
      if (Telnet) Telnet.stop();         // client disconnected
      Telnet = TelnetServer.available(); // ready for new client
    } else {
      TelnetServer.available().stop();  // have client, block new conections
    }
  }
}
*/

void printRTCmem() {
  Serial.println("");
  Serial.println("rtcMem ");
  Serial.print("markerFlag ");
  Serial.println(rtcMem.markerFlag);
  Serial.print("runSpaces ");
  Serial.println(rtcMem.runSpaces);
  Serial.print("bootTimes ");
  Serial.println(rtcMem.bootTimes);
  Serial.print("lastSubscribers ");
  Serial.println(rtcMem.lastSubscribers);
}


bool readRTCmem() {
  bool ret = true;
  system_rtc_mem_read(RTCMEMBEGIN, &rtcMem, sizeof(rtcMem));
  if (rtcMem.markerFlag != MAGICBYTE || rtcMem.lastSubscribers < 0 ) {
    rtcMem.markerFlag = MAGICBYTE;
    rtcMem.lastSubscribers = 0;
    rtcMem.bootTimes = 0;
    rtcMem.runSpaces = 0;
    system_rtc_mem_write(RTCMEMBEGIN, &rtcMem, sizeof(rtcMem));
    ret = false;
  }
  return ret;
}

void writeRTCmem() {
  rtcMem.markerFlag = MAGICBYTE;
  system_rtc_mem_write(RTCMEMBEGIN, &rtcMem, sizeof(rtcMem));
}

String getMACaddress() {
  uint8_t mac[6];
  char macStr[18] = {0};
  WiFi.macAddress(mac);
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],  mac[1], mac[2], mac[3], mac[4], mac[5]);
  return  String(macStr);
}


bool iotUpdater(String server, String url, String firmware, bool immediately, bool debugWiFi) {
  bool retValue = true;

  if (debugWiFi) {
    getMACaddress();
    Serial.print("IP = ");
    Serial.println(WiFi.localIP());
    Serial.print("Update_server ");
    Serial.println(server);
    Serial.print("UPDATE_URL ");
    Serial.println(url);
    Serial.print("FIRMWARE_VERSION ");
    Serial.println(firmware);
    Serial.println("Updating...");
  }
  t_httpUpdate_return ret = ESPhttpUpdate.update(server, 80, url, firmware);
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      retValue = false;
      if (debugWiFi) Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      Serial.println();
      break;

    case HTTP_UPDATE_NO_UPDATES:
      if (debugWiFi) Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      if (debugWiFi) Serial.println("HTTP_UPDATE_OK");
      break;
  }
  return retValue;
}


