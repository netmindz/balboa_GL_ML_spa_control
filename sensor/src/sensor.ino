// If connect to serial port over TCP, define the following
// #define SERIAL_OVER_IP_ADDR "192.168.178.131"

#define WDT_TIMEOUT 30

// #define AP_FALLBACK

#define WDT_TIMEOUT 30

#ifdef ESP32
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFiAP.h>
#include <esp_task_wdt.h>
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
#include <ArduinoQueue.h>

#include "constants.h"

#include "wifi_secrets.h"
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

int delayTime = 40;

WiFiClient clients[1];

#ifdef ESP32
#define tub Serial2
#define RX_PIN 19
#define TX_PIN 23
#define RTS_PIN 22 // RS485 direction control, RequestToSend TX or RX, required for MAX485 board.
#define PIN_5_PIN 18
#else
SoftwareSerial tub;
#define RX_PIN D6
#define TX_PIN D7
#define PIN_5_PIN D4
#define RTS_PIN D1 // RS485 direction control, RequestToSend TX or RX, required for MAX485 board.
#endif


HADevice device(mac, sizeof(mac));
HAMqtt mqtt(clients[0], device, 25);
HASwitch ecoMode("eco_only");
HASensorNumber temp("temp");
HASensorNumber targetTemp("targetTemp");
HASensorNumber timeToTemp("timeToTemp");
HASensor currentState("status");
HASensor haTime("time");
HASensor rawData("raw");
HASensor rawData2("raw2");
HASensor rawData3("raw3");
// HASensor rawData4("raw4");
// HASensor rawData5("raw5");
// HASensor rawData6("raw6");
HASensor rawData7("raw7");
HASelect tubMode("mode");
HASensorNumber uptime("uptime");
HASelect pump1("pump1");
HASelect pump2("pump2");
HABinarySensor heater("heater");
HASwitch light("light");
HASensorNumber tubpower("tubpower");

HAButton btnUp("up");
HAButton btnDown("down");
HAButton btnMode("mode");

#define MAX_SRV_CLIENTS 2
WiFiServer server(23);
WiFiClient serverClients[MAX_SRV_CLIENTS];

WebSocketsServer webSocket = WebSocketsServer(81);
#ifdef ESP32
WebServer webserver(80);
#else
ESP8266WebServer webserver(80);
#endif

int pump1State = 0;
int pump2State = 0;
boolean heaterState = false;
boolean lightState = false;
float tubpowerCalc = 0;
double tubTemp = -1;
double tubTargetTemp = -1;
String state = "unknown";


ArduinoQueue<String> sendBuffer(10);

void onBeforeSwitchStateChanged(bool state, HASwitch* s)
{
  // this callback will be called before publishing new state to HA
  // in some cases there may be delay before onStateChanged is called due to network latency
  // Serial.println("before Switch changed, clear command");
  // sendBuffer = "";
}

void onSwitchStateChanged(bool state, HASwitch* sender)
{
    Serial.print("Switch changed - ");
    if(state != lightState) {
      Serial.println("Toggle");
      sendBuffer.enqueue(COMMAND_LIGHT);
    }
    else {
      Serial.println("No change needed");
    }
}

void onPumpSwitchStateChanged(int8_t index, HASelect* sender)
{
  Serial.printf("onPumpSwitchStateChanged %s %u\n", sender->getName(), index);
  switch (index) {
    case 0:
        // Option "Off" was selected
        break;

    case 1:
        // Option "Low" was selected
        break;

    case 2:
        // Option "High" was selected
        break;

    default:
        // unknown option
        return;
    }

}
void onEcoSwitchStateChanged(bool state, HASwitch* s)
{
    Serial.print("Eco Switch changed - ");
    if(state == true) {
      if(tubMode.getCurrentState() == MODE_IDX_ECO) {
        Serial.println("No change needed");
      }
      else if(tubMode.getCurrentState() == MODE_IDX_STD) {
        Serial.println("Turn on Eco from STD");
        sendBuffer.enqueue(COMMAND_CHANGE_MODE);
        // sendBuffer.enqueue(COMMAND_EMPTY);
        sendBuffer.enqueue(COMMAND_DOWN);
        sendBuffer.enqueue(COMMAND_EMPTY);
        sendBuffer.enqueue(COMMAND_CHANGE_MODE);
      }
      else if(tubMode.getCurrentState() == MODE_IDX_SLP) {
        Serial.println("Turn on Eco from Sleep");
        sendBuffer.enqueue(COMMAND_CHANGE_MODE);
        // sendBuffer.enqueue(COMMAND_EMPTY);
        sendBuffer.enqueue(COMMAND_DOWN);
        // sendBuffer.enqueue(COMMAND_EMPTY);
        sendBuffer.enqueue(COMMAND_DOWN);
        sendBuffer.enqueue(COMMAND_CHANGE_MODE);
      }
    }
    else {
      if(tubMode.getCurrentState() == MODE_IDX_STD) {
        Serial.println("No change needed");
      }
      else if(tubMode.getCurrentState() == MODE_IDX_ECO) {
        Serial.println("Turn off eco from eco");
        sendBuffer.enqueue(COMMAND_CHANGE_MODE);
        sendBuffer.enqueue(COMMAND_EMPTY);
        sendBuffer.enqueue(COMMAND_DOWN);
        sendBuffer.enqueue(COMMAND_EMPTY);
        sendBuffer.enqueue(COMMAND_DOWN);
        sendBuffer.enqueue(COMMAND_CHANGE_MODE);
      }
      else if(tubMode.getCurrentState() == MODE_IDX_SLP) {
        Serial.println("Turn off eco from sleep");
        sendBuffer.enqueue(COMMAND_CHANGE_MODE);
        sendBuffer.enqueue(COMMAND_EMPTY);
        sendBuffer.enqueue(COMMAND_DOWN);
        sendBuffer.enqueue(COMMAND_EMPTY);
        sendBuffer.enqueue(COMMAND_CHANGE_MODE);
      }
    }
  }

void onModeSwitchStateChanged(int8_t index, HASelect* sender) {
    Serial.printf("Mode Switch changed - %u\n", index);
}

void onButtonPress(HAButton * sender) {
  String name = sender->getName();
  Serial.printf("Button press - %s\n", name);
  if(name == "Up") {
    sendBuffer.enqueue(COMMAND_UP);
  }
  else if(name == "Down") {
    sendBuffer.enqueue(COMMAND_DOWN);
  }
  else if(name == "Mode") {
    sendBuffer.enqueue(COMMAND_CHANGE_MODE);
  }
  else {
    Serial.printf("Unknown button %s\n", name);
  }

}

boolean isConnected = false;
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

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
  while (sanity < 20) {
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
    #ifdef AP_FALLBACK
      Serial.print("\n\nWifi failed, flip to fallback AP\n");
      WiFi.softAP("hottub", "Balboa");
      IPAddress myIP = WiFi.softAPIP();
      Serial.print("AP IP address: ");
      Serial.println(myIP);
    #else
      Serial.print("\n\nWifi failed, reboot\n");
      delay(1000);
      ESP.restart();
    #endif     
  }




  pinMode(RTS_PIN, OUTPUT);
  Serial.printf("Setting pin %u LOW\n", RTS_PIN);
  digitalWrite(RTS_PIN, LOW);
  pinMode(PIN_5_PIN, INPUT);  
#ifdef ESP32
  Serial.printf("Setting serial port as pins %u, %u\n", RX_PIN, TX_PIN);
  tub.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  while (tub.available() > 0)  { // workarond for bug with hanging during Serial2.begin - https://github.com/espressif/arduino-esp32/issues/5443
    Serial.read();
  }
  Serial.printf("Set serial port as pins %u, %u\n", RX_PIN, TX_PIN); // added here to see if line about was where the hang was
  tub.updateBaudRate(115200);
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
  device.setSoftwareVersion("0.2.0");
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
  timeToTemp.setUnitOfMeasurement("minutes");
  timeToTemp.setDeviceClass("duration");
  timeToTemp.setIcon("mdi:clock");

  currentState.setName("Status");

  pump1.setName("Pump1");
  pump1.setOptions("Off;Medium;High");
  pump1.setIcon("mdi:chart-bubble");
  pump1.onCommand(onPumpSwitchStateChanged);

  pump2.setName("Pump2");
  pump2.setOptions("Off;Medium;High");
  pump2.setIcon("mdi:chart-bubble");
  pump2.onCommand(onPumpSwitchStateChanged);

  heater.setName("Heater");
  heater.setIcon("mdi:radiator");
  light.setName("Light");
  light.onCommand(onSwitchStateChanged);

  ecoMode.setName("Eco Only");
  ecoMode.onCommand(onEcoSwitchStateChanged);


  haTime.setName("Time");

  rawData.setName("Raw data");
  rawData2.setName("CMD");
  rawData3.setName("post temp: ");
  // rawData4.setName("D01: ");
  // rawData5.setName("D02: ");
  // rawData6.setName("D03: ");
  rawData7.setName("FB");

  
  tubMode.setName("Mode");
  tubMode.setOptions("Standard;Economy;Sleep");
  tubMode.onCommand(onModeSwitchStateChanged);

  
  uptime.setName("Uptime");
  uptime.setUnitOfMeasurement("seconds");
  uptime.setDeviceClass("duration");

  tubpower.setName("Tub Power");
  tubpower.setUnitOfMeasurement("kW");
  tubpower.setDeviceClass("power");

  btnUp.setName("Up");
  btnDown.setName("Down");
  btnMode.setName("Mode");

  btnUp.onCommand(onButtonPress);
  btnDown.onCommand(onButtonPress);
  btnMode.onCommand(onButtonPress);



#ifdef BROKER_USERNAME
  mqtt.begin(BROKER_ADDR, BROKER_USERNAME, BROKER_PASSWORD);
#else
  mqtt.begin(BROKER_ADDR);
#endif

#ifdef ESP32
  Serial.println("Configuring WDT...");
  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch
#endif
  Serial.println("End of setup");
  digitalWrite(LED_BUILTIN, LOW);
}

String result = "";
String lastRaw = "";
String lastRaw2 = "";
String lastRaw3 = "";
String lastRaw4 = "";
String lastRaw5 = "";
String lastRaw6 = "";
String lastRaw7 = "";
String lastJSON = "";
int lastUptime = 0;
String timeString = "";
int msgLength = 0;

int i = 0;
void loop() {
  bool panelSelect = digitalRead(PIN_5_PIN); // LOW when we are meant to read data
  if (tub.available() > 0) {
    size_t len = tub.available();
    //    Serial.printf("bytes avail = %u\n", len);
    uint8_t buf[len];
    tub.read(buf, len);
    if (panelSelect == LOW) { // Only read data meant for us
      for (int i = 0; i < len; i++) {
        if (buf[i] < 0x10) {
          result += '0';
        }
        result += String(buf[i], HEX);
      }
      if(msgLength == 0 && result.length() == 2) {
        String messageType = result.substring(0, 2);
        if(messageType == "fa") {
          msgLength = 46;
        }
        else if(messageType == "ae") {
          msgLength = 32;
        }
        else {
          Serial.print("Unknown message length for ");
          Serial.println(messageType);
        }
      }          
      else if(result.length() == msgLength) {
         if(result.length() == 46 ) {
          // if(i %10 == 0)  
          sendCommand();
          i++;
        }
        handleMessage();
      }
    }
    else {
      // Serial.print("H");
      result = "";
      msgLength = 0;
    }
  }

  if(panelSelect == HIGH) {
    mqtt.loop();
    ArduinoOTA.handle();

    telnetLoop();

    // TODO: disabled while trying to elimate timing issues
    // webserver.handleClient();
    // webSocket.loop();

  // String json = getStatusJSON();
  // if (json != lastJSON) {
  //   webSocket.broadcastTXT(json);
  //   lastJSON = json;
  // }
  }

  if (((millis() / 1000) - lastUptime) >= 15) {
    lastUptime = millis() / 1000;
    uptime.setValue(lastUptime);
  }
  
#ifdef ESP32
  esp_task_wdt_reset();
#endif
}

void handleMessage() {
  
      //      Serial.print("message = ");
      //      Serial.println(result);


      if (result.substring(0, 4) == "fa14") {

        // Serial.println("FA 14");
        // telnetSend(result);

        // fa1433343043 = header + 340C = 34.0C

        // If messages is temp or ---- for temp, it is status message
        if (result.substring(10, 12) == "43" || result.substring(10, 12) == "2d") {

          tubpowerCalc = 0;
          String pump = result.substring(13, 14);

          if (pump == "0") {
            pump1State = 0;
            pump2State = 0;
          }
          else if (pump == "1"){
            pump1State = 1;
            pump2State = 0;
            tubpowerCalc += POWER_PUMP1_LOW;
          }
          else if (pump == "2"){
            pump1State = 2;
            pump2State = 0;
            tubpowerCalc += POWER_PUMP1_HIGH;
          }
          else if (pump == "7") {
            pump1State = 0;
            pump2State = 1;
            tubpowerCalc += POWER_PUMP2_LOW;
          }

          else if (pump == "8") {
            pump1State = 0;
            pump2State = 2;
            tubpowerCalc += POWER_PUMP2_HIGH;
          }

          else if (pump == "9") {
            pump1State = 1;
            pump2State = 2;
            tubpowerCalc += POWER_PUMP1_LOW;
            tubpowerCalc += POWER_PUMP2_HIGH;
          }
          else if (pump == "a") {
            pump1State = 2;
            pump2State = 1;
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
              tubMode.setState(MODE_IDX_SLP);
            }
            else if (s == "9") {
              state = "Circulation ?";
              tubMode.setState(MODE_IDX_STD); // TODO: confirm
            }
            else if (s == "1") {
              state = "Standard";
              tubMode.setState(MODE_IDX_STD);
            }
            else if (s == "2") {
              state = "Economy";
              tubMode.setState(MODE_IDX_ECO);
            }
            else if (s == "a") {
              state = "Cleaning"; // TODO: can't tell our actual mode here - could be any of the 3 I think
            }
            else if (s == "c") {
              state = "Circulation in sleep?";
              tubMode.setState(MODE_IDX_SLP);
            }
            else if (s == "b" || s == "3") {
              state = "Std in Eco"; // Was in eco, Swap to STD for 1 hour only
              tubMode.setState(MODE_IDX_STD);
            }
            else {
              state = "Unknown " + s;
            }
// TODO            currentMode.setValue(tubMode.getCurrentState c_str());

            String menu = result.substring(18, 20);
            if (menu == "00") {
              // idle
            }
            else if (menu == "4c") {
              state = "Set Mode";
            }
            else if (menu == "5a") {
              state = "Standby?"; // WT: not tested to confirm if this is the act of setting Standby or just seen when in standby
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
            if (!lastRaw3.equals(cmd)) {
              // Controller responded to command
              sendBuffer.dequeue();
              Serial.printf("YAY: command response : %u\n", delayTime);
            }

            if (!lastRaw3.equals(cmd) && cmd != "0000000000") { // ignore idle command
              lastRaw3 = cmd;
              rawData3.setValue(lastRaw3.c_str());
            }

            if (result.substring(10, 12) == "43") { // "C"
              double tmp = (HexString2ASCIIString(result.substring(4, 10)).toDouble() / 10);
              if (menu == "46") {
                tubTargetTemp = tmp;
                targetTemp.setValue((float) tubTargetTemp);
                Serial.printf("Sent target temp data %f\n", tubTargetTemp);
              }
              else {
                if (tubTemp != tmp) {
                  tubTemp = tmp;
                  temp.setValue((float) tubTemp);
                  Serial.printf("Sent temp data %f\n", tubTemp);
                }
                if(heaterState && (tubTemp < tubTargetTemp)) {
                  double tempDiff = (tubTargetTemp - tubTemp);
                  float timeToTempValue =   (tempDiff * MINUTES_PER_DEGC);
                  timeToTemp.setValue(timeToTempValue);
                }
                else {
                  timeToTemp.setValue(0);
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

        if(result.length() >= 64) { // "Long" messages only
          String tail = result.substring(46, 64);
          if (tail != lastRaw7) {
            lastRaw7 = tail;
            rawData7.setValue(lastRaw7.c_str());
          }
        }

        pump1.setState(pump1State);
        pump2.setState(pump2State);
        heater.setState(heaterState);
        light.setState(lightState);


        // end of FA14
      }
      else if (result.substring(0, 4) == "ae0d") {

        // Serial.println("AE 0D");
        // telnetSend(result);

        String message = result.substring(0, 32); // ignore any FB ending

        if (result.substring(0, 6) == "ae0d01" && message != "ae0d010000000000000000000000005a") {
          if (!lastRaw4.equals(message)) {
            lastRaw4 = message;
            // rawData4.setValue(lastRaw4.c_str());
          }
        }
        else if (result.substring(0, 6) == "ae0d02" && message != "ae0d02000000000000000000000000c3") {
          if (!lastRaw5.equals(message)) {
            lastRaw5 = message;
            // rawData5.setValue(lastRaw5.c_str());
          }
        }
        else if (result.substring(0, 6) == "ae0d03" && message != "ae0d03000000000000000000000000b4") {
          if (!lastRaw6.equals(message)) {
            lastRaw6 = message;
            // rawData6.setValue(lastRaw6.c_str());
          }
        }
        // end of AE 0D
      }
      else {
        Serial.printf("Unknown message (%u): ", result.length() );
        Serial.println(result);
        telnetSend("U: " + result);
      }
}


void sendCommand() {
  if(!sendBuffer.isEmpty()) {
    digitalWrite(RTS_PIN, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);

    delayMicroseconds(delayTime);
    // Serial.println("Sending " + sendBuffer);
    byte byteArray[18] = {0};
    hexCharacterStringToBytes(byteArray, sendBuffer.getHead().c_str());
    // if(digitalRead(PIN_5_PIN) != LOW) {
    //   Serial.println("ERROR: Pin5 went high before command before write");
    // }
    tub.write(byteArray, sizeof(byteArray));
    if(digitalRead(PIN_5_PIN) != LOW) {
      Serial.printf("ERROR: Pin5 went high before command before flush : %u\n", delayTime);
      delayTime = 0;
      sendBuffer.dequeue();
    }
    // tub.flush(true);
    if(digitalRead(PIN_5_PIN) == LOW) {
      // sendBuffer.dequeue(); // TODO: trying to resend now till we see response
      Serial.printf("message sent : %u\n", delayTime);
      // delayTime += 10;
    }
    // else {
    //   Serial.println("ERROR: Pin5 went high before command could be sent after flush");
    // }
    digitalWrite(RTS_PIN, LOW);
    digitalWrite(LED_BUILTIN, LOW);
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

void hexCharacterStringToBytes(byte *byteArray, const char *hexString)
{
  bool oddLength = strlen(hexString) & 1;

  byte currentByte = 0;
  byte byteIndex = 0;

  for (byte charIndex = 0; charIndex < strlen(hexString); charIndex++)
  {
    bool oddCharIndex = charIndex & 1;

    if (oddLength)
    {
      // If the length is odd
      if (oddCharIndex)
      {
        // odd characters go in high nibble
        currentByte = nibble(hexString[charIndex]) << 4;
      }
      else
      {
        // Even characters go into low nibble
        currentByte |= nibble(hexString[charIndex]);
        byteArray[byteIndex++] = currentByte;
        currentByte = 0;
      }
    }
    else
    {
      // If the length is even
      if (!oddCharIndex)
      {
        // Odd characters go into the high nibble
        currentByte = nibble(hexString[charIndex]) << 4;
      }
      else
      {
        // Odd characters go into low nibble
        currentByte |= nibble(hexString[charIndex]);
        byteArray[byteIndex++] = currentByte;
        currentByte = 0;
      }
    }
  }
}

byte nibble(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  return 0;  // Not a valid hexadecimal character
}

