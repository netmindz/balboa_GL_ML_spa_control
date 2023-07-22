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

#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WebSocketsServer.h>
#include <ArduinoQueue.h>
#include <CircularBuffer.h>

#include "constants.h"

// ************************************************************************************************
// Start of config
// ************************************************************************************************

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

// Uncomment if you want fallback to accesspoint (AP) if WIFI-connection could not be established. TODO: Define default.
// #define AP_FALLBACK 



#define WDT_TIMEOUT 30
int delayTime = 40;

// Setup of hardware pins for serial connection
#ifdef ESP32
  #define tubSerial Serial2
  #define RX_PIN 19
  #define TX_PIN 23
  #define RTS_PIN 22 // RS485 direction control, RequestToSend TX or RX, required for MAX485 board.
  #define PIN_5_PIN 18
#else
  SoftwareSerial tubSerial;
  #define RX_PIN D6
  #define TX_PIN D7
  #define PIN_5_PIN D4
  #define RTS_PIN D1 // RS485 direction control, RequestToSend TX or RX, required for MAX485 board.
#endif
 

// ************************************************************************************************
// End of config
// ************************************************************************************************


/* WIFI */
WiFiClient clients[1];
#define MAX_SRV_CLIENTS 2
WiFiServer server(23);
WiFiClient serverClients[MAX_SRV_CLIENTS];

/* WEBSERVER */
WebSocketsServer webSocket = WebSocketsServer(81);
#ifdef ESP32
  WebServer webserver(80);
#else
  ESP8266WebServer webserver(80);
#endif

#define BUFFER_SIZE 23
CircularBuffer<uint8_t, BUFFER_SIZE> Q_in;
CircularBuffer<uint8_t, 27> Q_out;

// struct {
//   uint8_t jet1 :2;
//   uint8_t jet2 :2;
//   uint8_t blower :1;
//   uint8_t light :1;
//   uint8_t restmode:1;
//   uint8_t highrange:1;
//   uint8_t padding :2;
//   uint8_t hour :5;
//   uint8_t minutes :6;
// } SpaState;
// struct {
//   uint8_t lcd :3;
//   uint8_t unit :1;
//   uint8_t pump :1;
//   uint8_t light :1;
//   uint8_t menu:1;
//   uint8_t hour:1;
//   uint8_t minute :2;
//   uint8_t hour :5;
//   uint8_t minutes :6;
// } tubRaw;
struct {
  int pump1State = 0;
  int pump2State = 0;
  bool heaterState = false;
  bool lightState = false;
  float tubpowerCalc = 0;
  float tubTemp = -1;
  float tubTargetTemp = -1;
  float tubTempF = -1;
  int tempUnit = 0;
  float timeToTemp = 0.0;
  int crc = 0;
  bool priming = false;     //TODO
  bool circulating = false; //TODO
  // String timeString = "";
  char timeString[6] = "--:--"; //size one larger than actual string to null terminate \0
  // String lcd = "";
  char lcd[5] = "----"; //size one larger than actual string to null terminate \0
  // String state = "unknown";
  char state[30] = "Unknown";
  // mode  
  String raw = "";

} tubState;

struct {
  int lastUptime = 0;
  bool wifiConnected = 0;
  bool tubSerialConnected = 0;
  bool tubSerialOverflow = 0;
  float espTemperature = 0;
} espState;

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
// ArduinoQueue<uint8_t> sendBufferInt(10); // TODO: might be better bigger for large temp changes. Would need testing


/* Function declarations */
void sendCommand(String command, int count);
void setOption(int currentIndex, int targetIndex, int options, String command);
void calculatePower();
void parseTubSerial();

#include "ha_mqtt.h"

/* ESP SETUP */
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
    tubSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
    while (tubSerial.available() > 0)  { // workarond for bug with hanging during Serial2.begin - https://github.com/espressif/arduino-esp32/issues/5443
      Serial.read();
    }
    Serial.printf("Set serial port as pins %u, %u\n", RX_PIN, TX_PIN); // added here to see if line about was where the hang was
    tubSerial.updateBaudRate(115200);
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


void parseTubSerial(){
    if(Q_in[0] == 0xFA && Q_in[1] == 0x14){   //TODO: Add crc-check too?

      /* BITREAD */
        // Serial.println(bitRead(Q_in[7],0));
        // Serial.println(bitRead(Q_in[7],1));

      /* LCD DISPLAY (2,3,4,5) */
        // std::string lcd = {char(Q_in[2]), char(Q_in[3]), char(Q_in[4]), char(Q_in[5]), '\0'};
        // tubState.lcd = String(lcd.c_str());
        sprintf(tubState.lcd, "%c%c%c%c", Q_in[2], Q_in[3], Q_in[4], Q_in[5]);

        //Check for degrees C (0x43) or F (0x46), then we have a temperature
        if (Q_in[5] == 0x43 || Q_in[5] == 0x46){
          tubState.tempUnit = Q_in[5];
          char temp[4] = {char(Q_in[2]), char(Q_in[3]), char(Q_in[4])};
          
          //When display showing C or F, and menu mode 0x46 we're in temperature setpoint adjustment
          if(Q_in[9] == 0x46){
            tubState.tubTargetTemp = atoi(temp)/10.0;
          }else{
            tubState.tubTemp = atoi(temp)/10.0;
          }
        }else if(tubState.tempUnit == 0x2D){
            //Showing '-' during priming etc
            bool priming = true;  // TODO: verify when it is '-' and add priming to struct?
        }
        // Serial.printf("LCD showing %f\n", tubState.tubTemp);
        



    /* STATUS & MODE (8) */
        switch (Q_in[8] & 0x0F) {
        case 0x04:
          // tubState.state = "Sleep";
          sprintf(tubState.state,"Sleep");
          break;
        case 0x09:
          // tubState.state = "Circulation ?";
          sprintf(tubState.state,"Circulation?");
          break;
        case 0x01:
          // tubState.state = "Standard";
          sprintf(tubState.state,"Standard");
          break;
        case 0x02:
          // tubState.state = "Economy";
          sprintf(tubState.state,"Economy");
          break;
        case 0x0A:
          // tubState.state = "Cleaning"; // TODO: can't tell our actual mode here - could be any of the 3 I think
          sprintf(tubState.state,"Cleaning"); // TODO: can't tell our actual mode here - could be any of the 3 I think
          break;
        case 0x0C:
          // tubState.state = "Circulation in sleep?";
          sprintf(tubState.state,"Circulation in sleep?");
          break;
        case 0x0B:
          // tubState.state = "Std in Eco"; // Was in eco, Swap to STD for 1 hour only
          sprintf(tubState.state,"Std in Eco"); // Was in eco, Swap to STD for 1 hour only
          break;
        case 0x03:
          // tubState.state = "Std in Eco"; // Was in eco, Swap to STD for 1 hour only
          sprintf(tubState.state,"Std in Eco"); // Was in eco, Swap to STD for 1 hour only
          break;
        // default:
          // tubState.state = "Unknown " + Q_in[8].c_str();
        }
        // Serial.println(tubState.state);
      
    /* PUMPS */
        switch (Q_in[6] & 0x0F) { // first 4 bits
        case 0x00:
          tubState.pump1State = 0;
          tubState.pump2State = 0;
          break;
        case 0x01:
          tubState.pump1State = 1;
          tubState.pump2State = 0;
          break;
        case 0x02:
          tubState.pump1State = 2;
          tubState.pump2State = 0;
          break;
        case 0x07:
          tubState.pump1State = 0;
          tubState.pump2State = 1;
          break;
        case 0x08:
          tubState.pump1State = 0;
          tubState.pump2State = 2;
          break;
        case 0x09:
          tubState.pump1State = 1;
          tubState.pump2State = 2;
          break;
        case 0x0A:
          tubState.pump1State = 2;
          tubState.pump2State = 1;
          break;
        default:
          tubState.pump1State = 0;
          tubState.pump2State = 0;
        }
        // Serial.printf("Pump1: %i Pump2: %i\n", tubState.pump1State, tubState.pump2State);

      /* LIGHT AND HEATER */
        switch (Q_in[7] & 0x0F) { // first 4 bits
        case 0x00:
          tubState.lightState = false;
          break;
        case 0x03:
          tubState.lightState = true;
          break;
        default:
          tubState.lightState = false;
        }  

        switch (Q_in[7] >> 4) { //last 4 bits
        case 0x00:
          tubState.heaterState = false;
          break;
        case 0x01:
          tubState.heaterState = true;
          break;
        case 0x02:
          tubState.heaterState = true;
          break;
        default:
          tubState.heaterState = false;
        }
        // Serial.printf("Light: %d Heater: %d\n", tubState.lightState, tubState.heaterState);


    /* MENU (9) */
        switch (Q_in[9]) {
        case 0x00:
          //idle
          break;
        case 0x4C:
          // tubState.state = "Set mode"; //Set mode
          sprintf(tubState.state,"Set mode");
          break;
        case 0x5A:
          // tubState.state = "Standby";   
          sprintf(tubState.state,"Standby");
            break;
        default:
          // tubState.state = "Menu " +Q_in[9]; 
          sprintf(tubState.state,"Menu %i", Q_in[9]);
        }  

    /* (10,11,12,13 unknown) */

    /* TIME (14,15) */
        String s;
        if (Q_in[14] < 10) s = "0"; else s = "";
        s += String(Q_in[14]) + ":";
        if (Q_in[15] < 10) s += "0";
        s += String(Q_in[15]);
        strncpy(tubState.timeString, s.c_str(), 5);



    /* TEMP IN F (16) */
        if(tubState.tempUnit = 0x43){
          // Unit is Celsius, doing conversion
          tubState.tubTempF = (Q_in[16]-32)*.55556;
          tubState.tubTempF = round(tubState.tubTempF * 2)/2; // tweak to round to nearest half
        }else if(tubState.tempUnit = 0x46){
          // Unit is Fahrenheit, no conversion needed.
          tubState.tubTempF = Q_in[16];
        }
        // Serial.printf("Temp from F: %f\n", tubState.tubTempF);


    /* (17,18,19,20,21 unknown) */

    /* CRC (22) */
      tubState.crc = Q_in[22];

    /* POWER */
      calculatePower();
      // Serial.printf("Power: %f kW\n", tubState.tubpowerCalc);

    // TODO: if targetTemp unknown, send command to toggle and receive.
    // TODO: if crc is not equal to last crc, send MQTT (should be ok?)
    // TODO: read FB commands (and set to state or elsewhere? temp up/dwn)
    }
    // Serial.printf("Display: %s Temp: %f%s Target: %f%s State: %s Pump1: %i Pump2: %i Heater: %i Light: %i Power: %f Time: %s T2T: %f T_F: %f CRC: %i", tubState.lcd, tubState.tubTemp,tubState.tubTargetTemp, tubState.tempUnit, tubState.state, tubState.pump1State, tubState.pump2State, tubState.heaterState, tubState.lightState, tubState.tubpowerCalc, tubState.timeString, tubState.timeToTemp, tubState.tubTempF, tubState.crc);
}

  void setTubTargetTemp(){
    if(sendBuffer.isEmpty()){
      sendBuffer.enqueue(COMMAND_UP);
    }
    // if(tubState.tubTargetTemp == -1.0){
    // if (menu == "46") {
}

  void printTubState(){
    Serial.printf("LCD: %s ", tubState.lcd);
    Serial.printf("T: %3.1f ", tubState.tubTemp);
    Serial.printf("Tset: %3.1f ", tubState.tubTargetTemp);
    Serial.printf("State: %s ", tubState.state);
    Serial.printf("P1: %i ", tubState.pump1State);
    Serial.printf("P2: %i ", tubState.pump2State);
    Serial.printf("Heat: %i ", tubState.heaterState);
    Serial.printf("Light: %i ", tubState.lightState);
    Serial.printf("Power: %3.1f ", tubState.tubpowerCalc);
    Serial.printf("Time: %s ", tubState.timeString);
    Serial.printf("T2T: %3.1f ", tubState.timeToTemp);
    Serial.printf("T_F: %3.1f ", tubState.tubTempF);
    Serial.printf("CRC: %X ", tubState.crc);
    Serial.println();
  }

  void calculatePower(){
    float power = 0.0;
    if(tubState.circulating) power += POWER_PUMP_CIRCULATION;
    if(tubState.heaterState) power += POWER_HEATER;
    if(tubState.pump1State == 1) power += POWER_PUMP1_LOW;
    else if(tubState.pump1State == 2) power += POWER_PUMP1_HIGH; 
    if(tubState.pump2State == 1) power += POWER_PUMP2_LOW;
    else if(tubState.pump2State == 2) power += POWER_PUMP2_HIGH; 
    tubState.tubpowerCalc = power;
  }

void sendHAMqtt(){
  // HAArduino: If a new value is the same as previous one, the MQTT message won’t be published.
  tubpower.setValue(tubState.tubpowerCalc);
  rawData.setValue(lastRaw.c_str());
  tubMode.setState(MODE_IDX_SLP); //TODO: Correct this
  // haTime.setValue(tubState.timeString.c_str());
  haTime.setValue(tubState.timeString);
  targetTemp.setValue((float) tubState.tubTargetTemp);
  hvac.setTargetTemperature( (float) tubState.tubTargetTemp);  // supress setting the target while we are changing the target  if(sendBuffer.isEmpty()) {
  temp.setValue((float) tubState.tubTemp);
  timeToTemp.setValue(tubState.timeToTemp);
  // currentState.setValue(tubState.state.c_str());
  currentState.setValue(tubState.state);
  pump1.setState(tubState.pump1State);
  pump2.setState(tubState.pump2State);
  heater.setState(tubState.heaterState);
  light.setState(tubState.lightState);
  uptime.setValue(espState.lastUptime);
 
}

// void print_msg(CircularBuffer<uint8_t, BUFFER_SIZE> &data) {
//   String s;
//   uint8_t x, i;
//   for (i = 0; i < data.size(); i++) {
//     x = Q_in[i];  //TODO should be data[]???
//     if (x < 0x10) s += "0";
//     s += String(x, HEX);
//     s += " ";
//   }
//   Serial.println(s);
// }





// void readTubSerial(){
//   bool panelSelect = digitalRead(PIN_5_PIN); // LOW when we are meant to read data
//   if (tubSerial.available() > 0) {
//     size_t len = tubSerial.available();
//     uint8_t buf[len]; // TODO: swap to fixed buffer to help prevent fragmentation of memory
//     tubSerial.read(buf, len);
//     if (panelSelect == LOW) { // Only read data meant for us
//       for (int i = 0; i < len; i++) {
//         if (buf[i] == 0xFA && len>=i+23) {
//           break;
//           for(int ii=0;i<23){
//             tubRawFA[ii] = buf[i+ii];
//           }
//         }
//         result += String(buf[i], HEX);
//       }  
//     }
//   }
// }

// void readTubSerial(){
//   // if (tubSerial.available() > 0) {
//   //   espState.tubSerialConnected = true;
//     size_t len = tubSerial.available();
//     uint8_t buf[len]; // TODO: swap to fixed buffer to help prevent fragmentation of memory
//     tubSerial.read(buf, len);
//     for (int i = 0; i < len; i++) {
//         if (buf[i] == 0xFA && len>=i+22) {
//           std::copy(buf+i, buf+i+23, tubRawFA);
//           break;
//         }
//     }
//   // }else{
//   //   espState.tubSerialConnected = false;
//   // }
// }

void print_msg(CircularBuffer<uint8_t, BUFFER_SIZE> &data) {
  String s;
  uint8_t x, i;
  //for (i = 0; i < (Q_in[1] + 2); i++) {
  for (i = 0; i < data.size(); i++) {
    x = Q_in[i];
    if (x < 0x10) s += "0";
    s += String(x, HEX);
    s += " ";
  }
  if(s!="") Serial.println(s);
}

String msg_string(CircularBuffer<uint8_t, BUFFER_SIZE> &data) {
  String s;
  uint8_t x, i;
  for (i = 0; i < data.size(); i++) {
    x = Q_in[i];
    if (x < 0x10) s += "0";
    s += String(x, HEX);
    s += " ";
  }
  return s;
}



#define BUFFERSIZE 256
#define SIZE_FA 23  //Size of 0xFA status messages (without commands)
#define SIZE_AE 16  //Size of 0xAE unknown messages (without commands)
#define SIZE_FB 9   //Size of 0xFB command messages

uint8_t tubRawFA[SIZE_FA];
uint8_t tubRawAE[SIZE_AE];
uint8_t tubRawFB[SIZE_FB];

uint8_t tubSerialBuf[BUFFERSIZE]; // Tub serial buffer

bool cmdSentOnLow = false;

void loop(){
  // Read tub pin indicating that tub controller is communcating to us (LOW) or to other panels (HIGH)
  bool panelSelect = digitalRead(PIN_5_PIN); // LOW when we are meant to read data

  // tubSerial
  int buflen = tubSerial.available();
  if(buflen > 0) espState.tubSerialConnected = true;
  else espState.tubSerialConnected = false;

  for(int i=0; i< buflen;i++){
    int x = tubSerial.read();
    Q_in.push(x);
    
    if(Q_in.first() != 0xFA) Q_in.clear();
    else if(Q_in.size() == SIZE_FA){
      // print_msg(Q_in);
      tubState.raw = msg_string(Q_in);
      Serial.println(tubState.raw);
      parseTubSerial();
      printTubState();
      break;
    }
  }
  

 if(panelSelect == LOW && !cmdSentOnLow){
  sendCommand(); // send reply *before* we parse the FA string as we don't want to delay the reply by say sending MQTT updates
  // sendTubCommand(sendBuffer.dequeue());
  cmdSentOnLow=true;    //Set true so no new commands are sent before pin has been toggled
  // parseTubSerial();  
  }else{
    cmdSentOnLow=false;    //Pin is high, so reset 

    //Empty stuff at startup etc
    // espState.tubSerialOverflow = true;
    // Serial.printf("Serial overflow %i", buflen);
    // while(tubSerial.available() > SIZE_FA){  
    //   tubSerial.read();
    // }
  }

  if (WiFi.status() == WL_CONNECTED) espState.wifiConnected = true;
  else espState.wifiConnected = false;

  mqtt.loop();
  ArduinoOTA.handle();
  telnetLoop();
  sendHAMqtt();

  // Only handle status is we aren't trying to send commands, webserver and websocket can both block for a long time
  if(sendBuffer.isEmpty()) {
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


  if (((millis() / 1000) - espState.lastUptime) >= 15) {
    espState.lastUptime = millis() / 1000;
  }

  

  #ifdef ESP32
    esp_task_wdt_reset();
  #endif  

}





// void loop(){
//   // Read tub pin indicating that tub controller is communcating to us (LOW) or to other panels (HIGH)
//   bool panelSelect = digitalRead(PIN_5_PIN); // LOW when we are meant to read data

  
//   // tubSerial
//   int len = tubSerial.available();
//   // Serial.printf("available: %i", len);
//   Serial.println();


//   if (len > 0 && len <= BUFFERSIZE) {
//     tubSerial.read(tubSerialBuf, tubSerial.available());
//     espState.tubSerialConnected = true;
//     espState.tubSerialOverflow = false;

//     // Tub controller communicating to us
//     if(panelSelect == LOW){
//       for (int i = 0; i < len; i++) {
//           if (tubSerialBuf[i] == 0xFA && len>i+SIZE_FA) {
//             std::copy(tubSerialBuf+i, tubSerialBuf+i+SIZE_FA, tubRawFA);
//             i=i-1+SIZE_FA;  //Jump to next message
//             // break; 
//           }
//           // else if (tubSerialBuf[i] == 0xAE && len>i+SIZE_AE) {
//           //   std::copy(tubSerialBuf+i, tubSerialBuf+i+SIZE_AE, tubRawAE);
//           //   i=i-1+SIZE_AE;  //Jump to next message
//           //   // break;
//           // }
//           // else if (tubSerialBuf[i] == 0xFB && len>i+SIZE_FB) {
//           //   std::copy(tubSerialBuf+i, tubSerialBuf+i+SIZE_FB, tubRawFB);
//           //   i=i-1+SIZE_FB;  //Jump to next message
//           //   // break;
//           // }
//       }
//       sendCommand(); // send reply *before* we parse the FA string as we don't want to delay the reply by say sending MQTT updates
//       // parseTubSerial();
//     }
//   }else if(len > BUFFERSIZE){
//     tubSerial.read(tubSerialBuf, sizeof tubSerialBuf);  //Read anyway to escape from buffered stuff at start
//     espState.tubSerialConnected = true;
//     espState.tubSerialOverflow = true;
//     Serial.println("Tub serial connected. Data overflow.");
//   }else{
//     espState.tubSerialOverflow = false;
//     espState.tubSerialConnected = false;
//   }

//   // Tub controller communicating to other panels, means we're in principle idling so do other stuff
//   if(panelSelect == HIGH){
//     if (WiFi.status() == WL_CONNECTED) espState.wifiConnected = true;
//     else espState.wifiConnected = false;

//   Serial.println();
//   for(int i=0; i<sizeof tubRawFA;i++){
//     if(tubRawFA[i]<0x10) Serial.print("0");
//     Serial.print(tubRawFA[i], HEX);
//     Serial.print(" ");
//   }
//   Serial.println();

//     mqtt.loop();
//     ArduinoOTA.handle();
//     telnetLoop();
//     sendHAMqtt();

//     if(sendBuffer.isEmpty()) { // Only handle status is we aren't trying to send commands, webserver and websocket can both block for a long time
//       webserver.handleClient();
//       webSocket.loop();
//       if(webSocket.connectedClients(false) > 0) {
//         String json = getStatusJSON();
//         if (json != lastJSON) {
//           webSocket.broadcastTXT(json);
//           lastJSON = json;
//         }
//       }
//     }
//   }

//   // if (((millis() / 1000) - lastUptime) >= 15) {
//   //   espState.lastUptime = millis() / 1000;
//   // }
//     espState.lastUptime = millis() / 1000;

  

//   #ifdef ESP32
//     esp_task_wdt_reset();
//   #endif  
    
// }


// /* ESP LOOP */
// void loop() {
//   bool panelSelect = digitalRead(PIN_5_PIN); // LOW when we are meant to read data
//   if (tubSerial.available() > 0) {
//     size_t len = tubSerial.available();
//     //    Serial.printf("bytes avail = %u\n", len);
//     uint8_t buf[len]; // TODO: swap to fixed buffer to help prevent fragmentation of memory
//     tubSerial.read(buf, len);
//     if (panelSelect == LOW) { // Only read data meant for us
//       for (int i = 0; i < len; i++) {
//         if (buf[i] < 0x10) {
//           result += '0';
//         }
//         result += String(buf[i], HEX);
//       }
//       if(msgLength == 0 && result.length() == 2) {
//         String messageType = result.substring(0, 2);
//         if(messageType == "fa") {
//           msgLength = 46;
//         }
//         else if(messageType == "ae") {
//           msgLength = 32;
//         }
//         else {
//           Serial.print("Unknown message length for ");
//           Serial.println(messageType);
//         }
//       }          
//       else if(result.length() == msgLength) {
//          if(result.length() == 46 ) {
//           sendCommand(); // send reply *before* we parse the FA string as we don't want to delay the reply by say sending MQTT updates
//         }
//         handleMessage();
//       }
//     }
//     else {
//       // Serial.print("H");
//       result = "";
//       msgLength = 0;
//     }
//   }

//   if(panelSelect == HIGH) { // Controller talking to other topside panels - we are in effect idle

//     mqtt.loop();
//     ArduinoOTA.handle();

//     telnetLoop();


//     if(sendBuffer.isEmpty()) { // Only handle status is we aren't trying to send commands, webserver and websocket can both block for a long time

//       webserver.handleClient();
//       webSocket.loop();

//       if(webSocket.connectedClients(false) > 0) {
//         String json = getStatusJSON();
//         if (json != lastJSON) {
//           webSocket.broadcastTXT(json);
//           lastJSON = json;
//         }
//       }
//     }

//   }

//   if (((millis() / 1000) - lastUptime) >= 15) {
//     lastUptime = millis() / 1000;
//     uptime.setValue(lastUptime);
//   }
  
// #ifdef ESP32
//   esp_task_wdt_reset();
// #endif
// }

// void handleMessage() {
  
//       //      Serial.print("message = ");
//       //      Serial.println(result);


//       if (result.substring(0, 4) == "fa14") {

//         // Serial.println("FA 14");
//         // telnetSend(result);

//         // fa1433343043 = header + 340C = 34.0C

//         // If messages is temp or ---- for temp, it is status message
//         if (result.substring(10, 12) == "43" || result.substring(10, 12) == "2d") {

//           tubState.tubpowerCalc = 0;
//           String pump = result.substring(13, 14);

//           if (pump == "0") {
//             tubState.pump1State = 0;
//             tubState.pump2State = 0;
//           }
//           else if (pump == "1"){
//             tubState.pump1State = 1;
//             tubState.pump2State = 0;
//             tubState.tubpowerCalc += POWER_PUMP1_LOW;
//           }
//           else if (pump == "2"){
//             tubState.pump1State = PUMP1_STATE_HIGH;
//             tubState.pump2State = 0;
//             tubState.tubpowerCalc += POWER_PUMP1_HIGH;
//           }
//           else if (pump == "7") {
//             tubState.pump1State = 0;
//             tubState.pump2State = 1;
//             tubState.tubpowerCalc += POWER_PUMP2_LOW;
//           }

//           else if (pump == "8") {
//             tubState.pump1State = 0;
//             tubState.pump2State = PUMP2_STATE_HIGH;
//             tubState.tubpowerCalc += POWER_PUMP2_HIGH;
//           }

//           else if (pump == "9") {
//             tubState.pump1State = 1;
//             tubState.pump2State = 2;
//             tubState.tubpowerCalc += POWER_PUMP1_LOW;
//             tubState.tubpowerCalc += POWER_PUMP2_HIGH;
//           }
//           else if (pump == "a") {
//             tubState.pump1State = 2;
//             tubState.pump2State = 1;
//             tubState.tubpowerCalc += POWER_PUMP1_HIGH;
//             tubState.tubpowerCalc += POWER_PUMP2_LOW;            
//           }

//           String heater = result.substring(14, 15);
//           if (heater == "0") {
//             tubState.heaterState = false;
//           }
//           else if (heater == "1") {
//             tubState.heaterState = true;
//             tubState.tubpowerCalc += POWER_HEATER;
//           }
//           else if (heater == "2") {
//             tubState.heaterState = true; // heater off, verifying temp change, but creates noisy state if we return false
//             tubState.tubpowerCalc += POWER_HEATER;
//           }

//           tubpower.setValue(tubState.tubpowerCalc);
          
//           String light = result.substring(15, 16);
//           if (light == "0") {
//             tubState.lightState = false;
//           }
//           else if (light == "3") {
//             tubState.lightState = true;
//           }

//           // Ignore last 2 bytes as possibly checksum, given we have temp earlier making look more complex than perhaps it is
//           String newRaw = result.substring(17, 44);
//           if (lastRaw != newRaw) {

//             lastRaw = newRaw;
//             rawData.setValue(lastRaw.c_str());

//             String s = result.substring(17, 18);
//             if (s == "4") {
//               tubState.state = "Sleep";
//               tubMode.setState(MODE_IDX_SLP);
//             }
//             else if (s == "9") {
//               tubState.state = "Circulation ?";
//               tubMode.setState(MODE_IDX_STD); // TODO: confirm
//             }
//             else if (s == "1") {
//               tubState.state = "Standard";
//               tubMode.setState(MODE_IDX_STD);
//             }
//             else if (s == "2") {
//               tubState.state = "Economy";
//               tubMode.setState(MODE_IDX_ECO);
//             }
//             else if (s == "a") {
//               tubState.state = "Cleaning"; // TODO: can't tell our actual mode here - could be any of the 3 I think
//             }
//             else if (s == "c") {
//               tubState.state = "Circulation in sleep?";
//               tubMode.setState(MODE_IDX_SLP);
//             }
//             else if (s == "b" || s == "3") {
//               tubState.state = "Std in Eco"; // Was in eco, Swap to STD for 1 hour only
//               tubMode.setState(MODE_IDX_STD);
//             }
//             else {
//               tubState.state = "Unknown " + s;
//             }

//             String menu = result.substring(18, 20);
//             if (menu == "00") {
//               // idle
//             }
//             else if (menu == "4c") {
//               tubState.state = "Set Mode";
//             }
//             else if (menu == "5a") {
//               tubState.state = "Standby?"; // WT: not tested to confirm if this is the act of setting Standby or just seen when in standby
//             }
//             else { // 46 for set temp, but also other things like filter time
//               tubState.state = "menu " + menu;
//             }

//             // 94600008002ffffff0200000000f5


//             if(result.substring(28, 32) != "ffff") {
//               tubState.timeString = HexString2TimeString(result.substring(28, 32));              
//             }
//             else {
//               tubState.timeString = "--:--";
//             }
//             haTime.setValue(tubState.timeString.c_str());
            
//             // temp up - ff0100000000?? - end varies

//             // temp down - ff0200000000?? - end varies

//             String cmd = result.substring(34, 44);
//             if (cmd == "0000000000")  {
//               // none
//             }
//             else if (cmd.substring(0, 4) == "01") {
//               tubState.state = "Temp Up";
//             }
//             else if (cmd.substring(0, 4) == "02") {
//               tubState.state = "Temp Down";
//             }
//             else {
//               telnetSend("CMD: " + cmd);
//             }
//             if (!lastRaw3.equals(cmd)) {
//               // Controller responded to command
//               sendBuffer.dequeue();
//               Serial.printf("YAY: command response : %u\n", delayTime);
//             }

//             if (!lastRaw3.equals(cmd) && cmd != "0000000000") { // ignore idle command
//               lastRaw3 = cmd;
//               rawData3.setValue(lastRaw3.c_str());
//             }

//             if (result.substring(10, 12) == "43") { // "C"
//               double tmp = (HexString2ASCIIString(result.substring(4, 10)).toDouble() / 10);
//               if (menu == "46") {
//                 tubState.tubTargetTemp = tmp;
//                 targetTemp.setValue((float) tubState.tubTargetTemp);
//                 if(sendBuffer.isEmpty()) {  // supress setting the target while we are changing the target
//                   hvac.setTargetTemperature( (float) tubState.tubTargetTemp);
//                 }
//                 Serial.printf("Sent target temp data %f\n", tubState.tubTargetTemp);
//               }
//               else {
//                 if (tubState.tubTemp != tmp) {
//                   tubState.tubTemp = tmp;
//                   temp.setValue((float) tubState.tubTemp);
//                   hvac.setCurrentCurrentTemperature((float) tubState.tubTemp);
//                   Serial.printf("Sent temp data %f\n", tubState.tubTemp);
//                 }
//                 if(tubState.heaterState && (tubState.tubTemp < tubState.tubTargetTemp)) {
//                   // double tempDiff = (tubState.tubTargetTemp - tubState.tubTemp);
//                   // float timeToTempValue =   (tempDiff * MINUTES_PER_DEGC);
//                   tubState.timeToTemp = ((tubState.tubTargetTemp - tubState.tubTemp) * MINUTES_PER_DEGC);
//                   timeToTemp.setValue(tubState.timeToTemp);
//                 }
//                 else {
//                   timeToTemp.setValue(0);
//                 }
//               }
//             }
//             else if (result.substring(10, 12) == "2d") { // "-"
//               //          Serial.println("temp = unknown");
//               //          telnetSend("temp = unknown");
//             }
//             else {
//               Serial.println("non-temp " + result);
//               telnetSend("non-temp " + result);
//             }


//             currentState.setValue(tubState.state.c_str());
//           }
//         }
//         else {
//           // FA but not temp data
//           lastRaw2 = result.substring(4, 28);
//           rawData2.setValue(lastRaw2.c_str());
//         }

//         if(result.length() >= 64) { // "Long" messages only
//           String tail = result.substring(46, 64);
//           if (tail != lastRaw7) {
//             lastRaw7 = tail;
//             rawData7.setValue(lastRaw7.c_str());
//           }
//         }

//         pump1.setState(tubState.pump1State);
//         pump2.setState(tubState.pump2State);
//         heater.setState(tubState.heaterState);
//         light.setState(tubState.lightState);


//         // end of FA14
//       }
//       else if (result.substring(0, 4) == "ae0d") {

//         // Serial.println("AE 0D");
//         // telnetSend(result);

//         String message = result.substring(0, 32); // ignore any FB ending

//         if (result.substring(0, 6) == "ae0d01" && message != "ae0d010000000000000000000000005a") {
//           if (!lastRaw4.equals(message)) {
//             lastRaw4 = message;
//             // rawData4.setValue(lastRaw4.c_str());
//           }
//         }
//         else if (result.substring(0, 6) == "ae0d02" && message != "ae0d02000000000000000000000000c3") {
//           if (!lastRaw5.equals(message)) {
//             lastRaw5 = message;
//             // rawData5.setValue(lastRaw5.c_str());
//           }
//         }
//         else if (result.substring(0, 6) == "ae0d03" && message != "ae0d03000000000000000000000000b4") {
//           if (!lastRaw6.equals(message)) {
//             lastRaw6 = message;
//             // rawData6.setValue(lastRaw6.c_str());
//           }
//         }
//         // end of AE 0D
//       }
//       else {
//         Serial.printf("Unknown message (%u): ", result.length() );
//         Serial.println(result);
//         telnetSend("U: " + result);
//       }
// }

void sendCommand(String command, int count) {
  Serial.printf("Sending %s - %u times\n", command.c_str(), count);
  for(int i = 0; i < count; i++) {
    sendBuffer.enqueue(command.c_str()); 
  }
}

void setOption(int currentIndex, int targetIndex, int options, String command) {
  if(targetIndex > currentIndex) {
    sendCommand(command, (targetIndex - currentIndex));
  }
  else if(currentIndex != targetIndex) {
    int presses = (options - currentIndex) + targetIndex;
    sendCommand(command, presses);
  }
}

void sendTubCommand(uint8_t* cmd) {
    digitalWrite(RTS_PIN, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);

    delayMicroseconds(delayTime);
    tubSerial.write(cmd, sizeof cmd);
    if(digitalRead(PIN_5_PIN) == LOW) {
      Serial.printf("Message sent to tub: %X with delay %u\n", cmd, delayTime);
    }else{
      Serial.printf("ERROR: Pin5 went high before command before flush : %u\n", delayTime);
    }

    digitalWrite(RTS_PIN, LOW);
    digitalWrite(LED_BUILTIN, LOW);
}

void sendCommand() {
  if(!sendBuffer.isEmpty()) {
    digitalWrite(RTS_PIN, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);

    delayMicroseconds(delayTime);
    // Serial.println("Sending " + sendBuffer);
    byte byteArray[18] = {0};
    hexCharacterStringToBytes(byteArray, sendBuffer.dequeue().c_str());
    // if(digitalRead(PIN_5_PIN) != LOW) {
    //   Serial.println("ERROR: Pin5 went high before command before write");
    // }
    tubSerial.write(byteArray, sizeof(byteArray));
    if(digitalRead(PIN_5_PIN) != LOW) {
      Serial.printf("ERROR: Pin5 went high before command before flush : %u\n", delayTime);
      delayTime = 0;
      // sendBuffer.dequeue();
    }
    // tubSerial.flush(true);
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

