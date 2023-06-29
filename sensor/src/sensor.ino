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
// #include <ArduinoHA.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WebSocketsServer.h>
#include <ArduinoQueue.h>


// ************************************************************************************************
// Start of config
// ************************************************************************************************

#define WDT_TIMEOUT 30

// Should we run as AP if we can't connect to WIFI?
// #define AP_FALLBACK 


#include "wifi_secrets.h"
// Create file with the following
// *************************************************************************
// #define SECRET_SSID "";                            /* Replace with your SSID */
// #define SECRET_PSK "";                             /* Replace with your WPA2 passphrase */
// #define BROKER_ADDR IPAddress(192,168,178,42)      /* Replace with your MQTT BROKER IP-address */
// byte mac[] = {0x00, 0x10, 0xFA, 0x6E, 0x38, 0x4A}; /* Replace with your mac-address or leave unchanged if you don't have multiple tubs */
// *************************************************************************

const char ssid[] = SECRET_SSID;
const char passphrase[] = SECRET_PSK;







int delayTime = 40;


#ifdef ESP32
#define tubserial Serial2
#define RX_PIN 19
#define TX_PIN 23
#define RTS_PIN 22 // RS485 direction control, RequestToSend TX or RX, required for MAX485 board.
#define PIN_5_PIN 18
#else
SoftwareSerial tubserial;
#define RX_PIN D6
#define TX_PIN D7
#define PIN_5_PIN D4
#define RTS_PIN D1 // RS485 direction control, RequestToSend TX or RX, required for MAX485 board.
#endif
 
// Uncomment if you have dual-speed pump
#define PUMP1_DUAL_SPEED
#define PUMP2_DUAL_SPEED

// ************************************************************************************************
// End of config
// ************************************************************************************************

#include "constants.h"



WiFiClient clients[1];

#define MAX_SRV_CLIENTS 2
WiFiServer server(23);
WiFiClient serverClients[MAX_SRV_CLIENTS];

WebSocketsServer webSocket = WebSocketsServer(81);
#ifdef ESP32
WebServer webserver(80);
#else
ESP8266WebServer webserver(80);
#endif

struct {
  int pump1State = 0;
  int pump2State = 0;
  boolean heaterState = false;
  boolean lightState = false;
  float tubpowerCalc = 0;
  double tubTemp = -1;
  double tubTargetTemp = -1;
  String state = "unknown";
  String timeString = "";
} tubState;

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
int msgLength = 0;
boolean isConnected = false;
ArduinoQueue<String> sendBuffer(10); // TODO: might be better bigger for large temp changes. Would need testing

void sendCommand(String command, int count) {
  Serial.printf("Sending %s - %u times\n", command.c_str(), count);
  for(int i = 0; i < count; i++) {
    sendBuffer.enqueue(command.c_str()); 
  }
}

void setOption(int currentIndex, int targetIndex, int options, String command = COMMAND_DOWN) {
  if(targetIndex > currentIndex) {
    sendCommand(command, (targetIndex - currentIndex));
  }
  else if(currentIndex != targetIndex) {
    int presses = (options - currentIndex) + targetIndex;
    sendCommand(command, presses);
  }
}

#include "ha_mqtt.h"


void setup() {
  /* Start serial for debugging */
  Serial.begin(115200);
  delay(1000);

  /* Switch on ESP LED during setup */
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  /* Setup, start and verify WIFI-connection */
  setupWifi();

  /* Setup and start serial connection to hottub */
  pinMode(RTS_PIN, OUTPUT);
  Serial.printf("Setting pin %u LOW\n", RTS_PIN);
  digitalWrite(RTS_PIN, LOW);
  pinMode(PIN_5_PIN, INPUT);  
  #ifdef ESP32
    Serial.printf("Setting serial port as pins %u, %u\n", RX_PIN, TX_PIN);
    tubserial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
    while (tubserial.available() > 0)  { // workarond for bug with hanging during Serial2.begin - https://github.com/espressif/arduino-esp32/issues/5443
      Serial.read();
    }
    Serial.printf("Set serial port as pins %u, %u\n", RX_PIN, TX_PIN); // added here to see if line about was where the hang was
    tubserial.updateBaudRate(115200);
  #endif

  /* start OTA */
  ArduinoOTA.setHostname("hottub-sensor");
  ArduinoOTA.begin();

  /* start telnet server */
  server.begin();
  server.setNoDelay(true);

  /* start web */
  webserver.on("/", handleRoot);
  webserver.on("/send", handleSend);
  webserver.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  /* setup Home Assistant MQTT */
  setupHaMqtt();
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

void loop() {
  bool panelSelect = digitalRead(PIN_5_PIN); // LOW when we are meant to read data
  if (tubserial.available() > 0) {
    size_t len = tubserial.available();
    //    Serial.printf("bytes avail = %u\n", len);
    uint8_t buf[len]; // TODO: swap to fixed buffer to help prevent fragmentation of memory
    tubserial.read(buf, len);
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
          sendCommand(); // send reply *before* we parse the FA string as we don't want to delay the reply by say sending MQTT updates
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

  if(panelSelect == HIGH) { // Controller talking to other topside panels - we are in effect idle

    mqtt.loop();
    ArduinoOTA.handle();

    telnetLoop();


    if(sendBuffer.isEmpty()) { // Only handle status is we aren't trying to send commands, webserver and websocket can both block for a long time

      webserver.handleClient();
      webSocket.loop();

      if(webSocket.connectedClients(false) > 0) {
        String json = getStatusJSON();
        if (json != lastJSON) {
          webSocket.broadcastTXT(json);
          lastJSON = json;
        }
      }
    }

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

          tubState.tubpowerCalc = 0;
          String pump = result.substring(13, 14);

          if (pump == "0") {
            tubState.pump1State = 0;
            tubState.pump2State = 0;
          }
          else if (pump == "1"){
            tubState.pump1State = 1;
            tubState.pump2State = 0;
            tubState.tubpowerCalc += POWER_PUMP1_LOW;
          }
          else if (pump == "2"){
            tubState.pump1State = PUMP1_STATE_HIGH;
            tubState.pump2State = 0;
            tubState.tubpowerCalc += POWER_PUMP1_HIGH;
          }
          else if (pump == "7") {
            tubState.pump1State = 0;
            tubState.pump2State = 1;
            tubState.tubpowerCalc += POWER_PUMP2_LOW;
          }

          else if (pump == "8") {
            tubState.pump1State = 0;
            tubState.pump2State = PUMP2_STATE_HIGH;
            tubState.tubpowerCalc += POWER_PUMP2_HIGH;
          }

          else if (pump == "9") {
            tubState.pump1State = 1;
            tubState.pump2State = 2;
            tubState.tubpowerCalc += POWER_PUMP1_LOW;
            tubState.tubpowerCalc += POWER_PUMP2_HIGH;
          }
          else if (pump == "a") {
            tubState.pump1State = 2;
            tubState.pump2State = 1;
            tubState.tubpowerCalc += POWER_PUMP1_HIGH;
            tubState.tubpowerCalc += POWER_PUMP2_LOW;            
          }

          String heater = result.substring(14, 15);
          if (heater == "0") {
            tubState.heaterState = false;
          }
          else if (heater == "1") {
            tubState.heaterState = true;
            tubState.tubpowerCalc += POWER_HEATER;
          }
          else if (heater == "2") {
            tubState.heaterState = true; // heater off, verifying temp change, but creates noisy state if we return false
            tubState.tubpowerCalc += POWER_HEATER;
          }

          tubpower.setValue(tubState.tubpowerCalc);
          
          String light = result.substring(15, 16);
          if (light == "0") {
            tubState.lightState = false;
          }
          else if (light == "3") {
            tubState.lightState = true;
          }

          // Ignore last 2 bytes as possibly checksum, given we have temp earlier making look more complex than perhaps it is
          String newRaw = result.substring(17, 44);
          if (lastRaw != newRaw) {

            lastRaw = newRaw;
            rawData.setValue(lastRaw.c_str());

            String s = result.substring(17, 18);
            if (s == "4") {
              tubState.state = "Sleep";
              tubMode.setState(MODE_IDX_SLP);
            }
            else if (s == "9") {
              tubState.state = "Circulation ?";
              tubMode.setState(MODE_IDX_STD); // TODO: confirm
            }
            else if (s == "1") {
              tubState.state = "Standard";
              tubMode.setState(MODE_IDX_STD);
            }
            else if (s == "2") {
              tubState.state = "Economy";
              tubMode.setState(MODE_IDX_ECO);
            }
            else if (s == "a") {
              tubState.state = "Cleaning"; // TODO: can't tell our actual mode here - could be any of the 3 I think
            }
            else if (s == "c") {
              tubState.state = "Circulation in sleep?";
              tubMode.setState(MODE_IDX_SLP);
            }
            else if (s == "b" || s == "3") {
              tubState.state = "Std in Eco"; // Was in eco, Swap to STD for 1 hour only
              tubMode.setState(MODE_IDX_STD);
            }
            else {
              tubState.state = "Unknown " + s;
            }

            String menu = result.substring(18, 20);
            if (menu == "00") {
              // idle
            }
            else if (menu == "4c") {
              tubState.state = "Set Mode";
            }
            else if (menu == "5a") {
              tubState.state = "Standby?"; // WT: not tested to confirm if this is the act of setting Standby or just seen when in standby
            }
            else { // 46 for set temp, but also other things like filter time
              tubState.state = "menu " + menu;
            }

            // 94600008002ffffff0200000000f5


            if(result.substring(28, 32) != "ffff") {
              tubState.timeString = HexString2TimeString(result.substring(28, 32));              
            }
            else {
              tubState.timeString = "--:--";
            }
            haTime.setValue(tubState.timeString.c_str());
            
            // temp up - ff0100000000?? - end varies

            // temp down - ff0200000000?? - end varies

            String cmd = result.substring(34, 44);
            if (cmd == "0000000000")  {
              // none
            }
            else if (cmd.substring(0, 4) == "01") {
              tubState.state = "Temp Up";
            }
            else if (cmd.substring(0, 4) == "02") {
              tubState.state = "Temp Down";
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
                tubState.tubTargetTemp = tmp;
                targetTemp.setValue((float) tubState.tubTargetTemp);
                if(sendBuffer.isEmpty()) {  // supress setting the target while we are changing the target
                  hvac.setTargetTemperature( (float) tubState.tubTargetTemp);
                }
                Serial.printf("Sent target temp data %f\n", tubState.tubTargetTemp);
              }
              else {
                if (tubState.tubTemp != tmp) {
                  tubState.tubTemp = tmp;
                  temp.setValue((float) tubState.tubTemp);
                  hvac.setCurrentCurrentTemperature((float) tubState.tubTemp);
                  Serial.printf("Sent temp data %f\n", tubState.tubTemp);
                }
                if(tubState.heaterState && (tubState.tubTemp < tubState.tubTargetTemp)) {
                  double tempDiff = (tubState.tubTargetTemp - tubState.tubTemp);
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


            currentState.setValue(tubState.state.c_str());
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

        pump1.setState(tubState.pump1State);
        pump2.setState(tubState.pump2State);
        heater.setState(tubState.heaterState);
        light.setState(tubState.lightState);


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
    tubserial.write(byteArray, sizeof(byteArray));
    if(digitalRead(PIN_5_PIN) != LOW) {
      Serial.printf("ERROR: Pin5 went high before command before flush : %u\n", delayTime);
      delayTime = 0;
      sendBuffer.dequeue();
    }
    // tubserial.flush(true);
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

void setupWifi(){
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

