#ifdef ESP32
#include <WiFi.h>
#include <ESPmDNS.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <SoftwareSerial.h>
#endif
#include <ArduinoHA.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "wifi.h"
// Create file with the following
// *************************************************************************
// #define SECRET_SSID "";  /* Replace with your SSID */
// #define SECRET_PSK "";   /* Replace with your WPA2 passphrase */
// *************************************************************************

const char ssid[] = SECRET_SSID;         /* Replace with your SSID */
const char passphrase[] = SECRET_PSK;   /* Replace with your WPA2 passphrase */

#define BROKER_ADDR IPAddress(192,168,178,42)

byte mac[] = {0x00, 0x10, 0xFA, 0x6E, 0x38, 0x4A};
// WiFi.macAddress();
double lastValue = 0;

WiFiClient clients[2];
#ifdef ESP32
WiFiClient tub = clients[1];
#else
SoftwareSerial tub(D6, D7); // RX, TX
#endif
HADevice device(mac, sizeof(mac));
HAMqtt mqtt(clients[0], device);
HASensor temp("temp"); // "temp" is unique ID of the sensor. You should define your own ID.
HASensor rawData("raw");
HASensor rawData2("raw2");
HASensor rawData3("raw3");
HASensor rawData4("raw4");
HASensor rawData5("raw5");
HASensor rawData6("raw6");
HASensor rawData7("raw7");
HABinarySensor pump1("pump1", "moving", false);
HABinarySensor pump2("pump2", "moving", false);
HABinarySensor heater("heater", "heat", false);
HABinarySensor light("light", "light", false);

#define MAX_SRV_CLIENTS 2
WiFiServer server(23);
WiFiClient serverClients[MAX_SRV_CLIENTS];

boolean pump1State = false;
boolean pump2State = false;
boolean heaterState = false;
boolean lightState = false;

void onBeforeSwitchStateChanged(bool state, HASwitch* s)
{
  // this callback will be called before publishing new state to HA
  // in some cases there may be delay before onStateChanged is called due to network latency
}

boolean isConnected = false;
void setup() {
  Serial.begin(115200);
  delay(10);

  // Make sure you're in station mode
  WiFi.mode(WIFI_STA);

  Serial.println("");
  Serial.print(F("Connecting to "));
  Serial.print(ssid);

  if (passphrase != NULL)
    WiFi.begin(ssid, passphrase);
  else
    WiFi.begin(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print(F("Connected with IP: "));
  Serial.println(WiFi.localIP());

#ifndef ESP32
  tub.begin(115200);
#endif

  ArduinoOTA.setHostname("hottub-sensor");

#ifdef ESP32
  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\nEnd");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
#else
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
#endif
  ArduinoOTA.begin();


  device.setName("Hottub");
  device.setSoftwareVersion("0.0.4");
  device.setManufacturer("Balboa");
  device.setModel("GL2000");

  // configure sensor (optional)
  temp.setUnitOfMeasurement("°C");
  temp.setDeviceClass("temperature");
//  temp.setIcon("mdi:home");
  temp.setName("Tub temperature");

  pump1.setName("Pump1");
  pump2.setName("Pump2");
  heater.setName("Heater");
  light.setName("Light");

  rawData.setName("Raw data");
  rawData2.setName("FB: ");
  rawData3.setName("D00: ");
  rawData4.setName("D01: ");
  rawData5.setName("D02: ");
  rawData6.setName("D03: ");
  rawData7.setName("CA82: ");
  
  mqtt.begin(BROKER_ADDR);

  //start server
  server.begin();
  server.setNoDelay(true);


}

String result = "";
String lastRaw = "";
String lastRaw2 = "";
String lastRaw3 = "";
String lastRaw4 = "";
String lastRaw5 = "";
String lastRaw6 = "";
String lastRaw7 = "";
double tubTemp;
boolean newData = false;
void loop() {
  mqtt.loop();
  ArduinoOTA.handle();

#ifdef ESP32
  if (!isConnected) {
    Serial.print("Try to connect to hottub ... ");
    if (!tub.connect("hottub", 7777)) {
      Serial.println("Connection failed.");
      Serial.println("Waiting 5 seconds before retrying...");
      delay(5000);
      return;
    }
    else {
      Serial.println("Connected");
      isConnected = true;
    }
  }
  if (!isConnected) {
    return;
  }
#endif

  if (tub.available() > 0) {
    size_t len = tub.available();
    //    Serial.printf("bytes avail = %u\n", len);
    uint8_t buf[len];
    tub.read(buf, len);
    handleBytes(buf, len);
  }

  telnetLoop();

  if (newData) {
    newData = false;
    Serial.printf("Send temp data %f\n", tubTemp);
      temp.setValue(tubTemp);

    pump1.setState(pump1State);
    pump2.setState(pump2State);
    heater.setState(heaterState);
    light.setState(lightState);
   
    rawData.setValue(lastRaw.c_str());
    rawData2.setValue(lastRaw2.c_str());
    rawData3.setValue(lastRaw3.c_str());
    rawData4.setValue(lastRaw4.c_str());
    rawData5.setValue(lastRaw5.c_str());
    rawData6.setValue(lastRaw6.c_str());
    rawData7.setValue(lastRaw7.c_str());

  }

}

void handleBytes(uint8_t buf[], size_t len) {
  for (int i = 0; i < len; i++) {
    if (String(buf[i], HEX) == "fa" || String(buf[i], HEX) == "ae" || String(buf[i], HEX) == "fb" || String(buf[i], HEX) == "ca") {
      
      // next byte is start of new message, so process what we have in result buffer
      
      Serial.print("message = ");
      Serial.println(result);

      telnetSend(result);

      // fa1433343043 = header + 340C = 34.0C
      if (result.substring(0, 4) == "fa14" && result.substring(11, 13) == "30") { // Change to 46 if fub in F not C
        
        tubTemp = (HexString2ASCIIString(result.substring(4, 10)).toDouble() / 10);
        Serial.printf("temp = %f\n", tubTemp);

        String pump = result.substring(13, 14);
        if(pump == "0") {
          pump1State = false;
          pump2State = false;
        }
        else if(pump == "2") {
          pump1State = true;
          pump2State = false;
        }
        else if(pump == "a") {
          pump1State = true;
          pump2State = true;
        }
        else if(pump == "8") {
          pump1State = false;
          pump2State = true;
        }

        String heater = result.substring(14, 15);
        if(heater == "0") {
          heaterState = false;
        }
        else if(heater == "1" || heater == "2") {
          heaterState = true;
        }

        String light = result.substring(15, 16);
        if(light == "0") {
          lightState = false;
        }
        else if(light == "3") {
          lightState = true;
        }

        String newRaw = result.substring(4, 17) + " pump=" + pump + " heater=" + heater  + " light=" + light;
        if(lastRaw != newRaw) {
          newData = true;
          lastRaw = newRaw;
        }

      }
      else if (result.substring(0, 2) == "fb") {
        if(lastRaw2 != result) {
          newData = true;
          lastRaw2 = result;
        }
      }
      else if (result.substring(0, 6) == "ae0d00") {
        if(lastRaw3 != result) {
          newData = true;
          lastRaw3 = result;
        }
      }
      else if (result.substring(0, 6) == "ae0d01") {
        if(lastRaw4 != result) {
          newData = true;
          lastRaw4 = result;
        }
      }
      else if (result.substring(0, 6) == "ae0d02") {
        if(lastRaw5 != result) {
          newData = true;
          lastRaw5 = result;
        }
      }
      else if (result.substring(0, 6) == "ae0d03") {
        if(lastRaw6 != result) {
          newData = true;
          lastRaw6 = result;
        }
      }
      else if (result.substring(0, 4) == "ca82") {
        if(lastRaw7 != result) {
          newData = true;
          lastRaw7 = result;
        }
      }

      
      result = ""; // clear buffer

    }
    if (buf[i] < 0x10) {
      result += '0';
    }
    result += String(buf[i], HEX);
  }
}

String HexString2ASCIIString(String hexstring) {
  String temp = "", sub = "", result;
  char buf[3];
  for (int i = 0; i < hexstring.length(); i += 2) {
    sub = hexstring.substring(i, i + 2);
    sub.toCharArray(buf, 3);
    char b = (char)strtol(buf, 0, 16);
    if (b == '\0')
      break;
    temp += b;
  }
  return temp;
}
