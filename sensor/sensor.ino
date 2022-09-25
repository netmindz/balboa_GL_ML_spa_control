// If connect to serial port over TCP, define the following
// #define SERIAL_OVER_IP_ADDR "192.168.178.131"

// If using MAX485 board which requires RTS_PIN for request-to-send, define the following:
// #define MAX485 TRUE;

#ifdef ESP32
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFiAP.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <SoftwareSerial.h> // - https://github.com/plerup/espsoftwareserial
#include <ESP8266WebServer.h>
#include <ESP8266WiFiAP.h>
#endif
#include <ArduinoHA.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WebSocketsServer.h>

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

// Perform measurements or read nameplate values on your tub to define the power [kW]
// for each device in order to calculate tub power usage
const float POWER_HEATER = 2.8;
const float POWER_PUMP_CIRCULATION = 0.3;
const float POWER_PUMP1_LOW = 0.31;
const float POWER_PUMP1_HIGH = 1.3;
const float POWER_PUMP2_LOW = 0.3;
const float POWER_PUMP2_HIGH = 0.6;

const int MINUTES_PER_DEGC = 45; // Tweak for your tub - would be nice to auto-learn in the future to allow for outside temp etc

const char* ZERO_SPEED = "off";
const char* LOW_SPEED = "low";
const char* HIGH_SPEED = "high";

WiFiClient clients[2];

#ifdef SERIAL_OVER_IP_ADDR
WiFiClient tub = clients[1];
#else
#ifdef ESP32
#define tub Serial2
#define RX_PIN 19
#define TX_PIN 23
#else
SoftwareSerial tub;
#define RX_PIN D6
#define TX_PIN D7
#endif
#ifdef MAX485
#define RTS_PIN D1 // RS485 direction control, RequestToSend TX or RX, required for MAX485 board.
#endif
#endif

HADevice device(mac, sizeof(mac));
HAMqtt mqtt(clients[0], device);
HASensor temp("temp");
HASensor targetTemp("targetTemp");
HASensor timeToTemp("timeToTemp");
HASensor currentState("status");
HASensor haTime("time");
HASensor rawData("raw");
HASensor rawData2("raw2");
HASensor rawData3("raw3");
HASensor rawData4("raw4");
HASensor rawData5("raw5");
HASensor rawData6("raw6");
HASensor rawData7("raw7");
HASensor currentMode("mode");
HASensor uptime("uptime");
HABinarySensor pump1("pump1", "moving", false);
HABinarySensor pump2("pump2", "moving", false);
HASensor pump1_state("pump1_state");
HASensor pump2_state("pump2_state");
HABinarySensor heater("heater", "heat", false);
HABinarySensor light("light", "light", false);
HASensor tubpower("tubpower");

#define MAX_SRV_CLIENTS 2
WiFiServer server(23);
WiFiClient serverClients[MAX_SRV_CLIENTS];

WebSocketsServer webSocket = WebSocketsServer(81);
#ifdef ESP32
WebServer webserver(80);
#else
ESP8266WebServer webserver(80);
#endif

boolean pump1State = false;
boolean pump2State = false;
String pump1Speed = ZERO_SPEED;
String pump2Speed = ZERO_SPEED;
boolean heaterState = false;
boolean lightState = false;
float tubpowerCalc = 0;

String sendBuffer;

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
    if (WiFi.status() == WL_CONNECTED) {
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

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("\n\nWifi failed, flip to fallback AP\n");
    WiFi.softAP("hottub", "Balboa");
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
  }



#ifndef SERIAL_OVER_IP_ADDR
#ifdef MAX485
  pinMode(RTS_PIN, OUTPUT);
  digitalWrite(RTS_PIN, LOW);
#endif
#ifdef ESP32
  Serial.printf("Setting serial port as pins %u, %u\n", RX_PIN, TX_PIN);
  tub.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  while (tub.available() > 0)  { // workarond for bug with hanging during Serial2.begin - https://github.com/espressif/arduino-esp32/issues/5443
    Serial.read();
  }
  Serial.printf("Set serial port as pins %u, %u\n", RX_PIN, TX_PIN); // added here to see if line about was where the hang was
  tub.updateBaudRate(115200);
#else
  tub.enableIntTx(false);
  tub.begin(115200, SWSERIAL_8N1, RX_PIN, TX_PIN, false); // RX, TX
//  tub.setRxBufferSize(1024);
#endif
#endif

  ArduinoOTA.setHostname("hottub-sensor");

  ArduinoOTA.begin();

  // start telnet server
  server.begin();
  server.setNoDelay(true);

  // start web
  webserver.on("/", handleRoot);
  webserver.on("/send", handleSend);
  webserver.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Home Assistant
  device.setName("Hottub");
  device.setSoftwareVersion("0.0.9");
  device.setManufacturer("Balboa");
  device.setModel("GL2000");

  // configure sensor (optional)
  temp.setUnitOfMeasurement("°C");
  temp.setDeviceClass("temperature");
  temp.setName("Tub temperature");

  targetTemp.setUnitOfMeasurement("°C");
  targetTemp.setDeviceClass("temperature");
  targetTemp.setName("Target Tub temp");


  timeToTemp.setName("Time to temp");

  currentState.setName("Status");

  pump1.setName("Pump1");
  pump1_state.setName("Pump1 State");
  //  pump1.setIcon("mdi:chart-bubble");
  pump2.setName("Pump2");
  pump2_state.setName("Pump2 State");
  //  pump2.setIcon("mdi:chart-bubble");
  heater.setName("Heater");
  //  heater.setIcon("mdi:radiator");
  light.setName("Light");
  haTime.setName("Time");

  rawData.setName("Raw data");
  rawData2.setName("FA: ");
  rawData3.setName("post temp: ");
  rawData4.setName("D01: ");
  rawData5.setName("D02: ");
  rawData6.setName("D03: ");
  rawData7.setName("FA Tail: ");

  currentMode.setName("Mode");
  uptime.setName("Uptime");

  tubpower.setName("Tub Power");
  tubpower.setUnitOfMeasurement("kW");
  tubpower.setDeviceClass("power");


#ifdef BROKER_USERNAME
  mqtt.begin(BROKER_ADDR, BROKER_USERNAME, BROKER_PASSWORD);
#else
  mqtt.begin(BROKER_ADDR);
#endif

}

String result = "";
String lastRaw = "";
String lastRaw2 = "";
String lastRaw3 = "";
String lastRaw4 = "";
String lastRaw5 = "";
String lastRaw6 = "";
String lastRaw7 = "";
String tubMode = "";
double tubTemp = -1;
double tubTargetTemp = -1;
String state = "unknown";
String lastJSON = "";
int lastUptime = 0;
String timeString = "";

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
  else {
    if(sendBuffer != "") {
      Serial.printf("Sending [%s]\n", sendBuffer);
      telnetSend("W: " + sendBuffer);
      tub.write(sendBuffer.c_str());
      sendBuffer = "";
    }
  }

  telnetLoop();
  webserver.handleClient();
  webSocket.loop();

  String json = getStatusJSON();
  if (json != lastJSON) {
    webSocket.broadcastTXT(json);
    lastJSON = json;
  }

  if (((millis() / 1000) - lastUptime) >= 30) {
    lastUptime = millis() / 1000;
    uptime.setValue(lastUptime);
  }

}

void handleBytes(uint8_t buf[], size_t len) {
  for (int i = 0; i < len; i++) {
    if (String(buf[i], HEX) == "fa" || String(buf[i], HEX) == "ae" || String(buf[i], HEX) == "ca") { // || String(buf[i], HEX) == "fb"

      // next byte is start of new message, so process what we have in result buffer

      //      Serial.print("message = ");
      //      Serial.println(result);


      if (result.length() == 64 && result.substring(0, 4) == "fa14") {

        Serial.println("FA 14 Long");
        telnetSend(result);

        // fa1433343043 = header + 340C = 34.0C

        // If messages is temp or ---- for temp, it is status message
        if (result.substring(10, 12) == "43" || result.substring(10, 12) == "2d") {

          tubpowerCalc = 0;
          String pump = result.substring(13, 14);

          if (pump == "0") {
            pump1State = false;
            pump2State = false;
            pump1Speed = ZERO_SPEED;
            pump2Speed = ZERO_SPEED;
          }
          else if (pump == "1"){
            pump1State = true;
            pump2State = false;
            pump1Speed = LOW_SPEED;
            pump2Speed = ZERO_SPEED;
            tubpowerCalc += POWER_PUMP1_LOW;
          }
          else if (pump == "2"){
            pump1State = true;
            pump2State = false;
            pump1Speed = HIGH_SPEED;
            pump2Speed = ZERO_SPEED;
            tubpowerCalc += POWER_PUMP1_HIGH;
          }
          else if (pump == "7") {
            pump1State = false;
            pump2State = true;
            pump1Speed = ZERO_SPEED;
            pump2Speed = LOW_SPEED;
            tubpowerCalc += POWER_PUMP2_LOW;
          }

          else if (pump == "8") {
            pump1State = false;
            pump2State = true;
            pump1Speed = ZERO_SPEED;
            pump2Speed = HIGH_SPEED;
            tubpowerCalc += POWER_PUMP2_HIGH;
          }

          else if (pump == "9") {
            pump1State = true;
            pump2State = true;
            pump1Speed = LOW_SPEED;
            pump2Speed = HIGH_SPEED;
            tubpowerCalc += POWER_PUMP1_LOW;
            tubpowerCalc += POWER_PUMP2_HIGH;
          }
          else if (pump == "a") {
            pump1State = true;
            pump2State = true;
            pump1Speed = HIGH_SPEED;
            pump2Speed = LOW_SPEED;
            tubpowerCalc += POWER_PUMP1_HIGH;
            tubpowerCalc += POWER_PUMP2_LOW;            
          }

          String heater = result.substring(14, 15);
          if (heater == "0") {
            heaterState = false;
          }
          else if (heater == "1") {
            heaterState = true;
            tubpowerCalc += POWER_HEATER;
          }
          else if (heater == "2") {
            heaterState = true; // heater off, verifying temp change, but creates noisy state if we return false
            tubpowerCalc += POWER_HEATER;
          }

          tubpower.setValue(tubpowerCalc);
          
          String light = result.substring(15, 16);
          if (light == "0") {
            lightState = false;
          }
          else if (light == "3") {
            lightState = true;
          }

          // Ignore last 2 bytes as possibly checksum, given we have temp earlier making look more complex than perhaps it is
          String newRaw = result.substring(17, 44);
          if (lastRaw != newRaw) {

            lastRaw = newRaw;
            rawData.setValue(lastRaw.c_str());

            String s = result.substring(17, 18);
            if (s == "4") {
              state = "Sleep";
              tubMode = "Sleep";
            }
            else if (s == "9") {
              state = "Circulation ?";
//              tubMode = "Standard";
            }
            else if (s == "1") {
              state = "Standard";
              tubMode = "Standard";
            }
            else if (s == "2") {
              state = "Economy";
              tubMode = "Economy";
            }
            else if (s == "a") {
              state = "Cleaning";
            }
            else if (s == "c") {
              state = "Circulation in sleep?";
            }
            else if (s == "b" || s == "3") {
              state = "Std in Eco"; // Was in eco, Swap to STD for 1 hour only
              tubMode = "Std in Eco";
            }
            else {
              state = "Unknown " + s;
            }
            currentMode.setValue(tubMode.c_str());

            String menu = result.substring(18, 20);
            if (menu == "00") {
              // idle
            }
            else if (menu == "4c") {
              state = "Set Mode";
            }
            else if (menu == "5a") {
              tubMode = "Standby?"; // WT: not tested to confirm if this is the act of setting Standby or just seen when in standby
            }
            else { // 46 for set temp, but also other things like filter time
              state = "menu " + menu;
            }

            // 94600008002ffffff0200000000f5


            if(result.substring(28, 32) != "ffff") {
              timeString = HexString2TimeString(result.substring(28, 32));              
            }
            else {
              timeString = "--:--";
            }
            haTime.setValue(timeString.c_str());
            
            // temp up - ff0100000000?? - end varies

            // temp down - ff0200000000?? - end varies

            String cmd = result.substring(34, 44);
            if (cmd == "0000000000")  {
              // none
            }
            else if (cmd.substring(0, 4) == "01") {
              state = "Temp Up";
            }
            else if (cmd.substring(0, 4) == "02") {
              state = "Temp Down";
            }
            else {
              telnetSend("CMD: " + cmd);
            }

            if (!lastRaw3.equals(cmd) && cmd != "0000000000") { // ignore idle command
              lastRaw3 = cmd;
              rawData3.setValue(lastRaw3.c_str());
            }

            if (result.substring(10, 12) == "43") { // "C"
              double tmp = (HexString2ASCIIString(result.substring(4, 10)).toDouble() / 10);
              if (menu == "46") {
                tubTargetTemp = tmp;
                targetTemp.setValue(tubTargetTemp);
                Serial.printf("Sent target temp data %f\n", tubTargetTemp);
              }
              else {
                if (tubTemp != tmp) {
                  tubTemp = tmp;
                  temp.setValue(tubTemp);
                  Serial.printf("Sent temp data %f\n", tubTemp);
                }
                if(heaterState && (tubTemp < tubTargetTemp)) {
                  double tempDiff = (tubTargetTemp - tubTemp);
                  String timeToTempMsg =  (String) (tempDiff * MINUTES_PER_DEGC);
                  timeToTempMsg += " mins"; 
                  timeToTemp.setValue(timeToTempMsg.c_str());  
                }
                else {
                  timeToTemp.setValue("");
                }
              }
            }
            else if (result.substring(10, 12) == "2d") { // "-"
              //          Serial.println("temp = unknown");
              //          telnetSend("temp = unknown");
            }
            else {
              Serial.println("non-temp " + result);
              telnetSend("non-temp " + result);
            }


            currentState.setValue(state.c_str());
          }
        }
        else {
          // FA but not temp data
          lastRaw2 = result.substring(4, 28);
          rawData2.setValue(lastRaw2.c_str());
        }

        String tail = result.substring(46, 64);
        if (tail != lastRaw7) {
          lastRaw7 = tail;
          rawData7.setValue(lastRaw7.c_str());
        }

        pump1.setState(pump1State);
        pump1_state.setValue(pump1Speed.c_str());
        pump2.setState(pump2State);
        pump2_state.setValue(pump2Speed.c_str());
        heater.setState(heaterState);
        light.setState(lightState);


        // end of FA14
      }
      else if (result.length() == 50 && result.substring(0, 4) == "ae0d") {

        Serial.println("AE 0D Long");
        telnetSend(result);

        String message = result.substring(0, 32); // ignore any FB ending

        //        if (result.substring(0, 6) == "ae0d00") {
        //          if (!lastRaw3.equals(message)) {
        //            lastRaw3 = message;
        //            rawData3.setValue(lastRaw3.c_str());
        //          }
        //        }
        //        else
        if (result.substring(0, 6) == "ae0d01" && message != "ae0d010000000000000000000000005a") {
          if (!lastRaw4.equals(message)) {
            lastRaw4 = message;
            rawData4.setValue(lastRaw4.c_str());
          }
        }
        else if (result.substring(0, 6) == "ae0d02" && message != "ae0d02000000000000000000000000c3") {
          if (!lastRaw5.equals(message)) {
            lastRaw5 = message;
            rawData5.setValue(lastRaw5.c_str());
          }
        }
        else if (result.substring(0, 6) == "ae0d03" && message != "ae0d03000000000000000000000000b4") {
          if (!lastRaw6.equals(message)) {
            lastRaw6 = message;
            rawData6.setValue(lastRaw6.c_str());
          }
        }
        // end of AE 0D
      }
      else if (result.length() == 46 || result.length() == 32) {
        // ignore the short-form messages
        //        Serial.println(result.substring(0, 6) + " Short");
        telnetSend(result);
      }
      else {
        Serial.printf("Unknown message (%u): ", result.length() );
        Serial.println(result);
        telnetSend("U: " + result);
      }

      result = ""; // clear buffer

    }
    if (buf[i] < 0x10) {
      result += '0';
    }
    result += String(buf[i], HEX);
  }
}

String HexString2TimeString(String hexstring){
  // Convert "HHMM" in HEX to "HH:MM" with decimal representation
  String time = "";
  int hour = strtol(hexstring.substring(0, 2).c_str(), NULL, 16);
  int minute  = strtol(hexstring.substring(2, 4).c_str(), NULL, 16);

  if(hour<10) time.concat("0");     // Add leading zero
  time.concat(hour);
  time.concat(":");
  if(minute<10) time.concat("0");   // Add leading zero
  time.concat(minute);
  
  return time;
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
