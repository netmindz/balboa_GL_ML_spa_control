#ifdef ESP32
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <esp_task_wdt.h>
#else
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266mDNS.h>
#include <SoftwareSerial.h>  // - https://github.com/plerup/espsoftwareserial
#endif
#include <ArduinoHA.h>
#include <ArduinoQueue.h>
#include <WebSocketsServer.h>
#include <WiFiUdp.h>
#include <WebOTA.h>


// ************************************************************************************************
// Start of config
// ************************************************************************************************

#define WDT_TIMEOUT 30

// Should we run as AP if we can't connect to WIFI?
// #define AP_FALLBACK

#include "wifi_secrets.h"
// Create file with the following
// *************************************************************************
// #define SECRET_SSID "";  /* Replace with your SSID */
// #define SECRET_PSK "";   /* Replace with your WPA2 passphrase */
// *************************************************************************

const char ssid[] = SECRET_SSID;
const char passphrase[] = SECRET_PSK;

#define BROKER_ADDR IPAddress(192, 168, 178, 42)  // Set the IP of your MQTT server
// #define BROKER_USERNAME "my-username"
// #define BROKER_PASSWORD "my-password"

byte mac[] = {0x00, 0x10, 0xFA, 0x6E, 0x38, 0x4A};  // Leave this value, unless you own multiple hot tubs


#ifdef RSC3
#define tub Serial1
#define RX_PIN 3
#define TX_PIN 10
#define RTS_PIN 5  // RS485 direction control, RequestToSend TX or RX, required for MAX485 board.
#define PIN_5_PIN 6

#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels(1, 4, NEO_GRB + NEO_KHZ800);

#elif ESP32
#define tub Serial2
#define RX_PIN 19
#define TX_PIN 23
#define RTS_PIN_DEF 22  // RS485 direction control, RequestToSend TX or RX, required for MAX485 board.
#define PIN_5_PIN_DEF 18
#else
SoftwareSerial tub;
#define RX_PIN D6
#define TX_PIN D7
#define PIN_5_PIN D4
#define RTS_PIN D1  // RS485 direction control, RequestToSend TX or RX, required for MAX485 board.
#endif

// Uncomment if you have dual-speed pump
// #define PUMP1_DUAL_SPEED
// #define PUMP2_DUAL_SPEED

// ************************************************************************************************
// End of config
// ************************************************************************************************

#include <balboaGL.h>

balboaGL spa(&tub, RTS_PIN_DEF, PIN_5_PIN_DEF);

WiFiClient clients[1];

HADevice device(mac, sizeof(mac));
HAMqtt mqtt(clients[0], device, 30);
HASensorNumber temp("temp", HANumber::PrecisionP1);
HASensorNumber tempFromF("tempFromF", HANumber::PrecisionP1);
HASensorNumber targetTemp("targetTemp", HANumber::PrecisionP1);
HASensorNumber timeToTemp("timeToTemp");
HASensor currentState("status");
HASensor lcd("lcd");
HASensor haTime("time");
HASensor rawData("raw");
HASensor rawData2("raw2");
HASensor rawData3("raw3");
HASensor fbData("fb");
HASelect tubMode("mode");
HASensorNumber uptime("uptime");
HASelect pump1("pump1");
HASelect pump2("pump2");
HABinarySensor heater("heater");
HASwitch light("light");
HASensorNumber tubpower("tubpower", HANumber::PrecisionP1);
HASensorNumber commandQueueSize("commandQueueSize");

HAButton btnUp("up");
HAButton btnDown("down");
HAButton btnMode("btnMode");
HAButton btnTime("btnTime");

// Not really HVAC device, but only way to get controls to set
HAHVAC hvac("temp", HAHVAC::TargetTemperatureFeature);

#define MAX_SRV_CLIENTS 2
WiFiServer server(23);
WiFiClient serverClients[MAX_SRV_CLIENTS];

WebSocketsServer webSocket = WebSocketsServer(81);
#ifdef ESP32
WebServer webserver(80);
#else
ESP8266WebServer webserver(80);
#endif

String lastJSON = "";
int lastUptime = 0;

#include "telnet.h"
#include "webstatus.h"

void onSwitchStateChanged(bool state, HASwitch* sender) {
    Serial.printf("Switch %s changed - ", sender->getName());
    if (state != status.light) {
        Serial.println("Toggle");
        sendBuffer.enqueue(COMMAND_LIGHT);
    } else {
        Serial.println("No change needed");
    }
}

void onPumpSwitchStateChanged(int8_t index, HASelect* sender) {
    Serial.printf("onPumpSwitchStateChanged %s %u\n", sender->getName(), index);
    int currentIndex = sender->getCurrentState();
    String command = COMMAND_JET1;
    int options = PUMP1_STATE_HIGH + 1;
    if (sender->getName() == "Pump2") {
        command = COMMAND_JET2;
        options = PUMP2_STATE_HIGH + 1;
    }
    spa.setOption(currentIndex, index, options, command);
}

void onModeSwitchStateChanged(int8_t index, HASelect* sender) {
    Serial.printf("Mode Switch changed - %u\n", index);
    int currentIndex = sender->getCurrentState();
    int options = 3;
    sendBuffer.enqueue(COMMAND_CHANGE_MODE);
    spa.setOption(currentIndex, index, options, COMMAND_DOWN);
    sendBuffer.enqueue(COMMAND_CHANGE_MODE);
}

void onButtonPress(HAButton* sender) {
    String name = sender->getName();
    Serial.printf("Button press - %s\n", name);
    if (name == "Up") {
        sendBuffer.enqueue(COMMAND_UP);
    } else if (name == "Down") {
        sendBuffer.enqueue(COMMAND_DOWN);
    } else if (name == "Mode") {
        sendBuffer.enqueue(COMMAND_CHANGE_MODE);
    } else if (name == "Time") {
        sendBuffer.enqueue(COMMAND_TIME);
    } else {
        Serial.printf("Unknown button %s\n", name);
    }
}

void onTargetTemperatureCommand(HANumeric temperature, HAHVAC* sender) {
    float temperatureFloat = temperature.toFloat();

    Serial.print("Target temperature: ");
    Serial.println(temperatureFloat);

    if (status.targetTemp <= 0) {
        Serial.print("ERROR: can't adjust target as current value not known");
        sendBuffer.enqueue(
            COMMAND_UP);  // Enter set temp mode - won't change, but should allow us to capture the set target value
        return;
    }

    int target = temperatureFloat * 2;  // 0.5 inc so double
    int current = status.targetTemp * 2;
    sendBuffer.enqueue(COMMAND_UP);  // Enter set temp mode
    sendBuffer.enqueue(COMMAND_EMPTY);

    if (temperatureFloat > status.targetTemp) {
        for (int i = 0; i < (target - current); i++) {
            Serial.println("Raise the temp");
            sendBuffer.enqueue(COMMAND_UP);
            // sendBuffer.enqueue(COMMAND_EMPTY);
        }
    } else {
        for (int i = 0; i < (current - target); i++) {
            Serial.println("Lower the temp");
            sendBuffer.enqueue(COMMAND_DOWN);
            // sendBuffer.enqueue(COMMAND_EMPTY);
        }
    }

    // sender->setTargetTemperature(temperature); // report target temperature back to the HA panel - better to see what
    // the control unit reports that assume our commands worked
}

void updateHAStatus() {

    static String lastRaw = "";
    if(status.rawData == lastRaw) {
        return;
    }
    lastRaw = status.rawData;

    tubpower.setValue(status.power);
    rawData.setValue(status.rawData.c_str());
    haTime.setValue(status.time.c_str());
    rawData3.setValue(status.rawData3.c_str());
    targetTemp.setValue(status.targetTemp);
    temp.setValue(status.temp);
    tempFromF.setValue(status.tempFromF);
    timeToTemp.setValue(status.timeToTemp);
    currentState.setValue(status.state.c_str());
    lcd.setValue(status.lcd);
    rawData2.setValue(status.rawData2.c_str());
    fbData.setValue(status.rawData7.c_str());
    // rawData4.setValue(lastRaw4.c_str());
    // rawData5.setValue(lastRaw5.c_str());
    // rawData6.setValue(lastRaw6.c_str());

    if (sendBuffer.isEmpty()) {
        hvac.setTargetTemperature(status.targetTemp);
    }
    hvac.setCurrentCurrentTemperature(status.temp);

    tubMode.setState(status.mode);

    pump1.setState(status.pump1);
    pump2.setState(status.pump2);
    heater.setState(status.heater);
    light.setState(status.light);

}

void setPixel(uint8_t color) {
#ifdef RSC3
    switch(color) {
        case 0:
            pixels.setPixelColor(0, pixels.Color(255,0,0));
            break;
        case 1:
            pixels.setPixelColor(0, pixels.Color(0,255,0));
            break;
        case 2:
            pixels.setPixelColor(0, pixels.Color(0,0,255));
            break;
        case 3:
            pixels.setPixelColor(0, pixels.Color(255,255,0));
            break;
        case 4:
            pixels.setPixelColor(0, pixels.Color(255,0,255));
            break;
    }     
    pixels.show();
#endif
}


void setup() {
    Serial.begin(115200);
    delay(1000);

#ifdef RSC3
    pixels.begin();
    pixels.setBrightness(255);
    setPixel(STATUS_BOOT);
#else    
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
#endif

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
        } else {
            delay(500);
            Serial.print(".");
        }
    }
    setPixel(STATUS_WIFI);

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

#ifdef ESP32
    Serial.printf("Setting serial port as pins %u, %u\n", RX_PIN, TX_PIN);
    tub.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
    while (tub.available() > 0) {  // workarond for bug with hanging during Serial2.begin -
                                   // https://github.com/espressif/arduino-esp32/issues/5443
        Serial.read();
    }
    Serial.printf("Set serial port as pins %u, %u\n", RX_PIN,
                  TX_PIN);  // added here to see if line about was where the hang was
    tub.updateBaudRate(115200);
#endif
    // enable interrupt for pin5 falling level change so we can clear the rx buffer
    // everytime our panel is selected
    attachInterrupt(digitalPinToInterrupt(spa.getPanelSelectPin()), clearRXBuffer, FALLING);

    init_wifi(ssid, passphrase, "hottub-sensor");
    webota.init(8080, "/update");

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
    device.setSoftwareVersion("0.2.3");
    device.setManufacturer("Balboa");
    device.setModel("GL2000");

    // configure sensor (optional)
    temp.setUnitOfMeasurement("°C");
    temp.setDeviceClass("temperature");
    temp.setName("Tub temperature");

    tempFromF.setUnitOfMeasurement("°C");
    tempFromF.setDeviceClass("temperature");
    tempFromF.setName("Tub temperature from F");

    targetTemp.setUnitOfMeasurement("°C");
    targetTemp.setDeviceClass("temperature");
    targetTemp.setName("Target Tub temp");

    timeToTemp.setName("Time to temp");
    timeToTemp.setUnitOfMeasurement("min");
    timeToTemp.setDeviceClass("duration");
    timeToTemp.setIcon("mdi:clock");

    currentState.setName("Status");
    lcd.setName("LCD");

    pump1.setName("Pump1");
#ifdef PUMP1_DUAL_SPEED
    pump1.setOptions("Off;Medium;High");
#else
    pump1.setOptions("Off;High");
#endif
    pump1.setIcon("mdi:chart-bubble");
    pump1.onCommand(onPumpSwitchStateChanged);

    pump2.setName("Pump2");
#ifdef PUMP2_DUAL_SPEED
    pump2.setOptions("Off;Medium;High");
#else
    pump2.setOptions("Off;High");
#endif
    pump2.setIcon("mdi:chart-bubble");
    pump2.onCommand(onPumpSwitchStateChanged);

    heater.setName("Heater");
    heater.setIcon("mdi:radiator");
    light.setName("Light");
    light.onCommand(onSwitchStateChanged);

    haTime.setName("Time");

    rawData.setName("Raw data");
    rawData2.setName("CMD");
    rawData3.setName("post temp: ");
    fbData.setName("FB");
    commandQueueSize.setName("Command Queue");

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
    btnTime.setName("Time");

    btnUp.onCommand(onButtonPress);
    btnDown.onCommand(onButtonPress);
    btnMode.onCommand(onButtonPress);
    btnTime.onCommand(onButtonPress);

    hvac.onTargetTemperatureCommand(onTargetTemperatureCommand);
    hvac.setModes(HAHVAC::AutoMode);
    hvac.setMode(HAHVAC::AutoMode, true);
    hvac.setName("Target Temp");
    hvac.setMinTemp(26);
    hvac.setMaxTemp(40);
    hvac.setTempStep(0.5);

#ifdef BROKER_USERNAME
    mqtt.begin(BROKER_ADDR, BROKER_USERNAME, BROKER_PASSWORD);
#else
    mqtt.begin(BROKER_ADDR);
#endif

#ifdef ESP32
    Serial.println("Configuring WDT...");
    esp_task_wdt_init(WDT_TIMEOUT, true);  // enable panic so ESP32 restarts
    esp_task_wdt_add(NULL);                // add current thread to WDT watch
#endif
    Serial.println("End of setup");
    digitalWrite(LED_BUILTIN, LOW);

    sendBuffer.enqueue(COMMAND_DOWN); // trigger set temp to capture target
}

boolean panelSelectDetected = false;
void loop() {
    
    spa.readSerial();

    bool panelSelect = digitalRead(spa.getPanelSelectPin());  // LOW when we are meant to read data

    if (panelSelect == HIGH || !panelSelectDetected) {  // Controller talking to other topside panels - we are in effect idle

        if(panelSelect == HIGH) {
            panelSelectDetected = true;
        }
        else {
            status.state = "Panel select (pin5) not detected";
        }

        updateHAStatus();

        mqtt.loop();
        
        detachInterrupt(digitalPinToInterrupt(spa.getPanelSelectPin()));
        webota.handle();
        attachInterrupt(digitalPinToInterrupt(spa.getPanelSelectPin()), clearRXBuffer, FALLING);

        telnetLoop();

        if (sendBuffer.isEmpty() || !panelSelectDetected) {  // Only handle status is we aren't trying to send commands, webserver and websocket
                                     // can both block for a long time

            webserver.handleClient();
            webSocket.loop();

            if (webSocket.connectedClients(false) > 0) {
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


