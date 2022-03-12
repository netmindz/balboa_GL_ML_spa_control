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
HASensor rawData("raw");
HABinarySensor pump1("pump1", "moving", false);
HABinarySensor pump2("pump2", "moving", false);
HABinarySensor heater("heater", "heat", false);
HABinarySensor light("light", "light", false);

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
  device.setSoftwareVersion("0.0.3");
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
  
  mqtt.begin(BROKER_ADDR);

}

String result = "";
String lastRaw = "";
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

    pump1.setState(pump1State);
    pump2.setState(pump2State);
    heater.setState(heaterState);
    light.setState(lightState);

   
    rawData.setValue(lastRaw.c_str());

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
    if (String(buf[i], HEX) == "fa" || String(buf[i], HEX) == "ae") {
      
      // next byte is start of new message, so process what we have in result buffer
      
      //        Serial.printf("HexString2ASCIIString message = %s\n", HexString2ASCIIString(result));
      Serial.print("message = ");
      Serial.println(result);

      if (result.substring(0, 4) == "fa14") {
        double tempValue;
        tempValue = (HexString2ASCIIString(result.substring(4, 10)).toDouble() / 10);
        Serial.printf("temp = %f\n", tempValue);
        if (tempValue > 10) { // hack just to handle temp showing as 3.8 not 38
          tubTemp = tempValue;
        }

        String pump = result.substring(10, 1);
        if(pump == "0") {
          pump1State = false;
          pump2State = false;
        }
        else if(pump == "2") {
          pump1State = true;
          pump2State = false;
        }
        else if(pump == "3") {
          pump1State = true;
          pump2State = true;
        }
        else if(pump == "8") {
          pump1State = false;
          pump2State = true;
        }

        String heater = result.substring(11, 1);
        if(heater == "0") {
          heaterState = false;
        }
        else if(heater == "1" && heater == "2") {
          heaterState = true;
        }

        String light = result.substring(12, 1);
        if(light == "0") {
          lightState = false;
        }
        else if(light == "1") {
          lightState = true;
        }

        lastRaw = result.substring(0, 13) + " pump=" + pump + " heater=" + heater  + "light=" + light;

      }
      else if (result.substring(0, 4) == "ae0d") {

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
