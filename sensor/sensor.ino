// If connect to serial port over TCP, define the following
// #define SERIAL_OVER_IP_ADDR "192.168.178.131"


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
#include <WebSocketsServer.h>
#include <WebServer.h>
#include <WiFiAP.h>

#include "wifi.h"
// Create file with the following
// *************************************************************************
// #define SECRET_SSID "";  /* Replace with your SSID */
// #define SECRET_PSK "";   /* Replace with your WPA2 passphrase */
// *************************************************************************

const char ssid[] = SECRET_SSID;
const char passphrase[] = SECRET_PSK;

#define BROKER_ADDR IPAddress(192,168,178,42)

byte mac[] = {0x00, 0x10, 0xFA, 0x6E, 0x38, 0x4A};
// WiFi.macAddress();


WiFiClient clients[2];

#ifdef SERIAL_OVER_IP_ADDR
WiFiClient tub = clients[1];
#else
#ifdef ESP32
#define tub Serial2
#define RX_PIN 19
#define TX_PIN 23
#else
SoftwareSerial tub(D6, D7); // RX, TX
#endif
#endif

HADevice device(mac, sizeof(mac));
HAMqtt mqtt(clients[0], device);
HASensor temp("temp");
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

WebSocketsServer webSocket = WebSocketsServer(81);
WebServer webserver(80);

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
  delay(1000);

  // Make sure you're in station mode
  WiFi.mode(WIFI_STA);

  Serial.println("");
  Serial.print(F("Connecting to "));
  Serial.print(ssid);

  if (passphrase != NULL)
    WiFi.begin(ssid, passphrase);
  else
    WiFi.begin(ssid);

  int sanity = 0;
  while (sanity < 200) {
    sanity++;
    if(WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.print(F("Connected with IP: "));
      Serial.println(WiFi.localIP());
      break;
    }
    else {
      delay(500);
     Serial.print(".");
    }
  }
  
  if(WiFi.status() != WL_CONNECTED) {
    Serial.print("\n\nWifi failed, flip to fallback AP\n");
    WiFi.softAP("hottub", "Balboa");
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);    
  }
  


#ifndef SERIAL_OVER_IP_ADDR
#ifdef ESP32
  Serial.printf("Setting serial port as pins %u, %u\n", RX_PIN, TX_PIN); 
  tub.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  while (tub.available() > 0)  { // workarond for bug with hanging during Serial2.begin
    Serial.read();
  }
  Serial.printf("Set serial port as pins %u, %u\n", RX_PIN, TX_PIN); // added here to see if line about was where the hang was
  tub.updateBaudRate(115200);
#else
  tub.begin(115200);
#endif
#endif

  ArduinoOTA.setHostname("hottub-sensor");

  ArduinoOTA.begin();

  // start telnet server
  server.begin();
  server.setNoDelay(true);

  // start web
  webserver.on("/", handleRoot);
  webserver.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Home Assistant
  device.setName("Hottub");
  device.setSoftwareVersion("0.0.7");
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
  rawData2.setName("FA: ");
  rawData3.setName("D00: ");
  rawData4.setName("D01: ");
  rawData5.setName("D02: ");
  rawData6.setName("D03: ");
  rawData7.setName("FA Tail: ");

  mqtt.begin(BROKER_ADDR);


}

String result = "";
String lastRaw = "";
String lastRaw2 = "";
String lastRaw3 = "";
String lastRaw4 = "";
String lastRaw5 = "";
String lastRaw6 = "";
String lastRaw7 = "";
double tubTemp = -1;
boolean newData = false;
String lastJSON = "";
void loop() {
  mqtt.loop();
  ArduinoOTA.handle();

#ifdef SERIAL_OVER_IP_ADDR
  if (!isConnected) {
    Serial.print("Try to connect to hottub ... ");
    if (!tub.connect(SERIAL_OVER_IP_ADDR, 7777)) {
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
  webserver.handleClient();
  webSocket.loop();

  if (newData) {
    newData = false;
    
    rawData2.setValue(lastRaw2.c_str());

    rawData7.setValue(lastRaw7.c_str());

    String json = getStatusJSON();
    if(json != lastJSON) {
      webSocket.broadcastTXT(json);
      lastJSON = json;
    }
  }

}

void handleBytes(uint8_t buf[], size_t len) {
  for (int i = 0; i < len; i++) {
    if (String(buf[i], HEX) == "fa" || String(buf[i], HEX) == "ae" || String(buf[i], HEX) == "ca") { // || String(buf[i], HEX) == "fb"

      // next byte is start of new message, so process what we have in result buffer

      Serial.print("message = ");
      Serial.println(result);


      if (result.length() == 64 && result.substring(0, 4) == "fa14") {

        Serial.println("FA 14 Long");
        telnetSend(result);

      // fa1433343043 = header + 340C = 34.0C
        if (result.substring(10, 12) == "43") {
          double tmp = (HexString2ASCIIString(result.substring(4, 10)).toDouble() / 10);
          if(tubTemp != tmp) {
            newData = true;
            tubTemp = tmp;
            temp.setValue(tubTemp);
            Serial.printf("Sent temp data %f\n", tubTemp);
          }
//          Serial.printf("temp = %f\n", tubTemp);
        }
        else if (result.substring(10, 12) == "2d") {
//          Serial.println("temp = unknown");
//          telnetSend("temp = unknown");
        }
        else {
          Serial.println("non-temp " + result);
          telnetSend("non-temp " + result);
        }

        // If messages is temp or ---- for temp, it is status message
        if(result.substring(10, 12) == "43" || result.substring(10, 12) == "2d") {
          
          String pump = result.substring(13, 14);
          if (pump == "0") {
            pump1State = false;
            pump2State = false;
          }
          else if (pump == "2") {
            pump1State = true;
            pump2State = false;
          }
          else if (pump == "a") {
            pump1State = true;
            pump2State = true;
          }
          else if (pump == "8") {
            pump1State = false;
            pump2State = true;
          }
  
          String heater = result.substring(14, 15);
          if (heater == "0") {
            heaterState = false;
          }
          else if (heater == "1") {
            heaterState = true;
          }
          else if (heater == "2") {
            heaterState = false; // heater off, verifying temp change
          }
  
          String light = result.substring(15, 16);
          if (light == "0") {
            lightState = false;
          }
          else if (light == "3") {
            lightState = true;
          }
  
          String newRaw = result.substring(17, 46) + " pump=" + pump + " light=" + light;
          if (lastRaw != newRaw) {
            newData = true;
            lastRaw = newRaw;
            rawData.setValue(lastRaw.c_str());
          }
        }
        else {
          // FA but not temp data
          newData = true;
          lastRaw2 = result.substring(4, 28); 
        }

        lastRaw7 = result.substring(46, 64);

        pump1.setState(pump1State);
        pump2.setState(pump2State);
        heater.setState(heaterState);
        light.setState(lightState);


        // end of FA14        
      }
      else if (result.length() == 50 && result.substring(0, 4) == "ae0d") {

        Serial.println("AE 0D Long");
        telnetSend(result);

        String message = result.substring(0,32);  // ignore any FB ending
        
        if (result.substring(0, 6) == "ae0d00") {
          if (!lastRaw3.equals(message)) {
            lastRaw3 = message;
            rawData3.setValue(lastRaw3.c_str());
          }
        }
        else if (result.substring(0, 6) == "ae0d01") {
          if (!lastRaw4.equals(message)) {
            lastRaw4 = message;
            rawData4.setValue(lastRaw4.c_str());
          }
        }
        else if (result.substring(0, 6) == "ae0d02") {
          if (!lastRaw5.equals(message)) {
            lastRaw5 = message;
            rawData5.setValue(lastRaw5.c_str());
          }
        }
        else if (result.substring(0, 6) == "ae0d03") {
          if (!lastRaw6.equals(message)) {
            lastRaw6 = message;
            rawData6.setValue(lastRaw6.c_str());
          }
        }
        // end of AE 0D
      }
      else if (result.length() == 46 || result.length() == 32) {
        // ignore the short-form messages
        Serial.print(result.substring(0, 6) + " Short");
      }
      else {
        Serial.print("Unknown message : ");
        Serial.println(result);
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
