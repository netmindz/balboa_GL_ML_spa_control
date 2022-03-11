#ifdef ESP32
#include <WiFi.h>
#include <ESPmDNS.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <SoftwareSerial.h>

SoftwareSerial tub(D6, D7); // RX, TX
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
unsigned long lastSentAt = millis();
double lastValue = 0;

WiFiClient clients[2];
WiFiClient client = clients[1];
HADevice device(mac, sizeof(mac));
HAMqtt mqtt(clients[0], device);
HASensor temp("temp"); // "temp" is unique ID of the sensor. You should define your own ID.

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


  // set device's details (optional)
  device.setName("Hottub");
  device.setSoftwareVersion("0.0.1");

  // configure sensor (optional)
  temp.setUnitOfMeasurement("°C");
  temp.setDeviceClass("temperature");
  temp.setIcon("mdi:home");
  temp.setName("Tub temperature");

  mqtt.begin(BROKER_ADDR);

}

String result = "";
double tubTemp;
void loop() {
  mqtt.loop();
  ArduinoOTA.handle();

#ifdef ESP32
  if (!isConnected) {
    Serial.print("Try to connect to hottub ... ");
    if (!client.connect("hottub", 7777)) {
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
  if (isConnected && client.available() > 0) {
    size_t len = client.available();
    //        Serial.printf("bytes avail = %u\n", len);
    uint8_t buf[len];
    client.read(buf, len);
    handleBytes(buf, len);
  }
#else
  if (tub.available() > 0) {
    size_t len = tub.available();
//    Serial.printf("bytes avail = %u\n", len);
    uint8_t buf[len];
    tub.read(buf, len);
    handleBytes(buf, len);
  }
#endif

  if ((millis() - lastSentAt) >= 10000) {
    lastSentAt = millis();
//    if(tubTemp > 0) {
      Serial.printf("Send temp data %f\n", tubTemp);
      lastSentAt = millis();
      temp.setValue(tubTemp);
//    }
//    else {
//      Serial.printf("No temp data yet\n");
//    }

    // Supported data types:
    // uint32_t (uint16_t, uint8_t)
    // int32_t (int16_t, int8_t)
    // double
    // float
    // const char*
  }
}

void handleBytes(uint8_t buf[], size_t len) {
  for (int i = 0; i < len; i++) {
    if (String(buf[i], HEX) == "fa") {
      //        Serial.printf("HexString2ASCIIString message = %s\n", HexString2ASCIIString(result));
      Serial.print("message = ");
      Serial.println(result);

      if (result.substring(0, 4) == "fa14") {
        double tempValue;
        tempValue = (HexString2ASCIIString(result.substring(4, 10)).toDouble() / 10);
        Serial.printf("temp = %f\n", tempValue);
        if(tempValue > 10) { // hack just to handle temp showing as 3.8 not 38
          tubTemp = tempValue;
        }
      }

      result = ""; // clear buffer
      //        Serial.println("\n");
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
