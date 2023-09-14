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
#include <ArduinoOTA.h>
#include <ArduinoQueue.h>
#include <WebSocketsServer.h>
#include <WiFiUdp.h>

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

// Perform measurements or read nameplate values on your tub to define the power [kW]
// for each device in order to calculate tub power usage
const float POWER_HEATER = 2.8;
const float POWER_PUMP_CIRCULATION = 0.3;
const float POWER_PUMP1_LOW = 0.31;
const float POWER_PUMP1_HIGH = 1.3;
const float POWER_PUMP2_LOW = 0.3;
const float POWER_PUMP2_HIGH = 0.6;

// Tweak for your tub - would be nice to auto-learn in the future to allow for outside temp etc
const int MINUTES_PER_DEGC = 45;

int delayTime = 40;


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
#define RTS_PIN 22  // RS485 direction control, RequestToSend TX or RX, required for MAX485 board.
#define PIN_5_PIN 18
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

#include "constants.h"

WiFiClient clients[1];

HADevice device(mac, sizeof(mac));
HAMqtt mqtt(clients[0], device, 30);
HASensorNumber temp("temp", HANumber::PrecisionP1);
HASensorNumber targetTemp("targetTemp", HANumber::PrecisionP1);
HASensorNumber timeToTemp("timeToTemp");
HASensor currentState("status");
HASensor haTime("time");
HASensor rawData("raw");
HASensor rawData2("raw2");
HASensor rawData3("raw3");
HASensor rawData7("raw7");
HASelect tubMode("mode");
HASensorNumber uptime("uptime");
HASelect pump1("pump1");
HASelect pump2("pump2");
HABinarySensor heater("heater");
HASwitch light("light");
HASensorNumber tubpower("tubpower", HANumber::PrecisionP1);

HAButton btnUp("up");
HAButton btnDown("down");
HAButton btnMode("btnMode");

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

int pump1State = 0;
int pump2State = 0;
boolean heaterState = false;
boolean lightState = false;
float tubpowerCalc = 0;
double tubTemp = -1;
double tubTargetTemp = -1;
String state = "unknown";

ArduinoQueue<String> sendBuffer(10);  // TODO: might be better bigger for large temp changes. Would need testing

void sendCommand(String command, int count) {
    Serial.printf("Sending %s - %u times\n", command.c_str(), count);
    for (int i = 0; i < count; i++) {
        sendBuffer.enqueue(command.c_str());
    }
}

void setOption(int currentIndex, int targetIndex, int options, String command = COMMAND_DOWN) {
    if (targetIndex > currentIndex) {
        sendCommand(command, (targetIndex - currentIndex));
    } else if (currentIndex != targetIndex) {
        int presses = (options - currentIndex) + targetIndex;
        sendCommand(command, presses);
    }
}

void onSwitchStateChanged(bool state, HASwitch* sender) {
    Serial.printf("Switch %s changed - ", sender->getName());
    if (state != lightState) {
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
    setOption(currentIndex, index, options, command);
}

void onModeSwitchStateChanged(int8_t index, HASelect* sender) {
    Serial.printf("Mode Switch changed - %u\n", index);
    int currentIndex = sender->getCurrentState();
    int options = 3;
    sendBuffer.enqueue(COMMAND_CHANGE_MODE);
    setOption(currentIndex, index, options);
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
    } else {
        Serial.printf("Unknown button %s\n", name);
    }
}

void onTargetTemperatureCommand(HANumeric temperature, HAHVAC* sender) {
    float temperatureFloat = temperature.toFloat();

    Serial.print("Target temperature: ");
    Serial.println(temperatureFloat);

    if (tubTargetTemp < 0) {
        Serial.print("ERROR: can't adjust target as current value not known");
        sendBuffer.enqueue(
            COMMAND_UP);  // Enter set temp mode - won't change, but should allow us to capture the set target value
        return;
    }

    int target = temperatureFloat * 2;  // 0.5 inc so double
    int current = tubTargetTemp * 2;
    sendBuffer.enqueue(COMMAND_UP);  // Enter set temp mode
    sendBuffer.enqueue(COMMAND_EMPTY);

    if (temperatureFloat > tubTargetTemp) {
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
    }     
    pixels.show();
#endif
}

boolean isConnected = false;
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

    pinMode(RTS_PIN, OUTPUT);
    Serial.printf("Setting pin %u LOW\n", RTS_PIN);
    digitalWrite(RTS_PIN, LOW);
    pinMode(PIN_5_PIN, INPUT);
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
    device.setSoftwareVersion("0.2.1");
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
    timeToTemp.setUnitOfMeasurement("min");
    timeToTemp.setDeviceClass("duration");
    timeToTemp.setIcon("mdi:clock");

    currentState.setName("Status");

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

    hvac.onTargetTemperatureCommand(onTargetTemperatureCommand);
    hvac.setModes(HAHVAC::AutoMode);
    hvac.setMode(HAHVAC::AutoMode);
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
uint32_t lastUptime = 0;
String timeString = "";
int msgLength = 0;
boolean panelDetected = false;

void handleBytes(size_t len, uint8_t buf[]);

void loop() {
    bool panelSelect = digitalRead(PIN_5_PIN);  // LOW when we are meant to read data
    if (tub.available() > 0) {
        setPixel(STATUS_OK);
        size_t len = tub.available();
        //    Serial.printf("bytes avail = %u\n", len);
        uint8_t buf[len];  // TODO: swap to fixed buffer to help prevent fragmentation of memory
        tub.read(buf, len);
        if (panelSelect == LOW) {  // Only read data meant for us
            handleBytes(len, buf);
        } else {
            // Serial.print("H");
            result = "";
            msgLength = 0;
        }
    }

    if (panelSelect == HIGH || !panelDetected) {  // Controller talking to other topside panels - we are in effect idle

        if(panelSelect == HIGH) {
            panelDetected = true;
        }
        else {
            state = "Panel select (pin5) not detected";
            currentState.setValue(state.c_str());
        }

        mqtt.loop();
        ArduinoOTA.handle();

        telnetLoop();

        if (sendBuffer.isEmpty()) {  // Only handle status is we aren't trying to send commands, webserver and websocket
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

void handleBytes(size_t len, uint8_t buf[]) {
    for (int i = 0; i < len; i++) {
        if (buf[i] < 0x10) {
            result += '0';
        }
        result += String(buf[i], HEX);
    }
    if (msgLength == 0 && result.length() == 2) {
        String messageType = result.substring(0, 2);
        if (messageType == "fa") {
            msgLength = 46;
        } else if (messageType == "ae") {
            msgLength = 32;
        } else {
            Serial.print("Unknown message length for ");
            Serial.println(messageType);
        }
    } else if (result.length() == msgLength) {
        if (result.length() == 46) {
            sendCommand();  // send reply *before* we parse the FA string as we don't want to delay the reply by
                            // say sending MQTT updates
        }
        handleMessage();
    }
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

            if (pump == "0") {  // Pump 1 Off - Pump 2 Off - 0b0000
                pump1State = 0;
                pump2State = 0;
            } else if (pump == "1") {  // Pump 1 Low - Pump 2 Off - 0b0001
                pump1State = 1;
                pump2State = 0;
                tubpowerCalc += POWER_PUMP1_LOW;
            } else if (pump == "2") {  // Pump 1 High - Pump 2 Off - 0b0010
                pump1State = PUMP1_STATE_HIGH;
                pump2State = 0;
                tubpowerCalc += POWER_PUMP1_HIGH;
            } else if (pump == "4") {  // Pump 1 Off - Pump 2 Low - 0b0100
                pump1State = 0;
                pump2State = 1;
                tubpowerCalc += POWER_PUMP2_LOW;
            } else if (pump == "5") {  // Pump 1 Low - Pump 2 Low - 0b0101
                pump1State = 1;
                pump2State = 1;
                tubpowerCalc += POWER_PUMP1_LOW;
                tubpowerCalc += POWER_PUMP2_LOW;
            } else if (pump == "6") {  // Pump 1 High - Pump 2 Low - 0b0110
                pump1State = PUMP1_STATE_HIGH;
                pump2State = 1;
                tubpowerCalc += POWER_PUMP1_HIGH;
                tubpowerCalc += POWER_PUMP2_LOW;
            } else if (pump == "8") {  // Pump 1 Off - Pump 2 High - 0b1000
                pump1State = 0;
                pump2State = PUMP2_STATE_HIGH;
                tubpowerCalc += POWER_PUMP2_HIGH;
            } else if (pump == "9") {  // Pump 1 Low - Pump 2 High - 0b1001
                pump1State = 1;
                pump2State = PUMP2_STATE_HIGH;
                tubpowerCalc += POWER_PUMP1_LOW;
                tubpowerCalc += POWER_PUMP2_HIGH;
            } else if (pump == "a") {  // Pump 1 High - Pump 2 HIGH = 0b1010
                pump1State = PUMP1_STATE_HIGH;
                pump2State = PUMP2_STATE_HIGH;
                tubpowerCalc += POWER_PUMP1_HIGH;
                tubpowerCalc += POWER_PUMP2_HIGH;
            }

            String heater = result.substring(14, 15);
            if (heater == "0") {
                heaterState = false;
            } else if (heater == "1") {
                heaterState = true;
                tubpowerCalc += POWER_HEATER;
            } else if (heater == "2") {
                heaterState = true;  // heater off, verifying temp change, but creates noisy state if we return false
                tubpowerCalc += POWER_HEATER;
            }

            tubpower.setValue(tubpowerCalc);

            String light = result.substring(15, 16);
            if (light == "0") {
                lightState = false;
            } else if (light == "3") {
                lightState = true;
            }

            // Ignore last 2 bytes as possibly checksum, given we have temp earlier making look more complex than
            // perhaps it is
            String newRaw = result.substring(17, 44);
            if (lastRaw != newRaw) {
                lastRaw = newRaw;
                rawData.setValue(lastRaw.c_str());

                String s = result.substring(17, 18);
                if (s == "4") {
                    state = "Sleep";
                    tubMode.setState(MODE_IDX_SLP);
                } else if (s == "9") {
                    state = "Circulation ?";
                    tubMode.setState(MODE_IDX_STD);  // TODO: confirm
                } else if (s == "1") {
                    state = "Standard";
                    tubMode.setState(MODE_IDX_STD);
                } else if (s == "2") {
                    state = "Economy";
                    tubMode.setState(MODE_IDX_ECO);
                } else if (s == "a") {
                    state = "Cleaning";  // TODO: can't tell our actual mode here - could be any of the 3 I think
                } else if (s == "c") {
                    state = "Circulation in sleep?";
                    tubMode.setState(MODE_IDX_SLP);
                } else if (s == "b" || s == "3") {
                    state = "Std in Eco";  // Was in eco, Swap to STD for 1 hour only
                    tubMode.setState(MODE_IDX_STD);
                } else {
                    state = "Unknown " + s;
                }

                String menu = result.substring(18, 20);
                if (menu == "00") {
                    // idle
                } else if (menu == "4c") {
                    state = "Set Mode";
                } else if (menu == "5a") {
                    state = "Standby?";  // WT: not tested to confirm if this is the act of setting Standby or just seen
                                         // when in standby
                } else {                 // 46 for set temp, but also other things like filter time
                    state = "menu " + menu;
                }

                // 94600008002ffffff0200000000f5

                if (result.substring(28, 32) != "ffff") {
                    timeString = HexString2TimeString(result.substring(28, 32));
                } else {
                    timeString = "--:--";
                }
                haTime.setValue(timeString.c_str());

                // temp up - ff0100000000?? - end varies

                // temp down - ff0200000000?? - end varies

                String cmd = result.substring(34, 44);
                if (cmd == "0000000000") {
                    // none
                } else if (cmd.substring(0, 4) == "01") {
                    state = "Temp Up";
                } else if (cmd.substring(0, 4) == "02") {
                    state = "Temp Down";
                } else {
                    telnetSend("CMD: " + cmd);
                }
                if (!lastRaw3.equals(cmd)) {
                    // Controller responded to command
                    sendBuffer.dequeue();
                    Serial.printf("YAY: command response : %u\n", delayTime);
                }

                if (!lastRaw3.equals(cmd) && cmd != "0000000000") {  // ignore idle command
                    lastRaw3 = cmd;
                    rawData3.setValue(lastRaw3.c_str());
                }

                if (result.substring(10, 12) == "43") {  // "C"
                    double tmp = (HexString2ASCIIString(result.substring(4, 10)).toDouble() / 10);
                    if (menu == "46") {
                        tubTargetTemp = tmp;
                        targetTemp.setValue((float)tubTargetTemp);
                        if (sendBuffer.isEmpty()) {  // supress setting the target while we are changing the target
                            hvac.setTargetTemperature((float)tubTargetTemp);
                        }
                        Serial.printf("Sent target temp data %f\n", tubTargetTemp);
                    } else {
                        if (tubTemp != tmp) {
                            tubTemp = tmp;
                            temp.setValue((float)tubTemp);
                            hvac.setCurrentCurrentTemperature((float)tubTemp);
                            Serial.printf("Sent temp data %f\n", tubTemp);
                        }
                        if (heaterState && (tubTemp < tubTargetTemp)) {
                            double tempDiff = (tubTargetTemp - tubTemp);
                            float timeToTempValue = (tempDiff * MINUTES_PER_DEGC);
                            timeToTemp.setValue(timeToTempValue);
                        } else {
                            timeToTemp.setValue((float) 0);
                        }
                    }
                } else if (result.substring(10, 12) == "2d") {  // "-"
                                                                //          Serial.println("temp = unknown");
                                                                //          telnetSend("temp = unknown");
                } else {
                    Serial.println("non-temp " + result);
                    telnetSend("non-temp " + result);
                }

                currentState.setValue(state.c_str());
            }
        } else {
            // FA but not temp data
            lastRaw2 = result.substring(4, 28);
            rawData2.setValue(lastRaw2.c_str());
        }

        if (result.length() >= 64) {  // "Long" messages only
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
    } else if (result.substring(0, 4) == "ae0d") {
        // Serial.println("AE 0D");
        // telnetSend(result);

        String message = result.substring(0, 32);  // ignore any FB ending

        if (result.substring(0, 6) == "ae0d01" && message != "ae0d010000000000000000000000005a") {
            if (!lastRaw4.equals(message)) {
                lastRaw4 = message;
                // rawData4.setValue(lastRaw4.c_str());
            }
        } else if (result.substring(0, 6) == "ae0d02" && message != "ae0d02000000000000000000000000c3") {
            if (!lastRaw5.equals(message)) {
                lastRaw5 = message;
                // rawData5.setValue(lastRaw5.c_str());
            }
        } else if (result.substring(0, 6) == "ae0d03" && message != "ae0d03000000000000000000000000b4") {
            if (!lastRaw6.equals(message)) {
                lastRaw6 = message;
                // rawData6.setValue(lastRaw6.c_str());
            }
        }
        // end of AE 0D
    } else {
        Serial.printf("Unknown message (%u): ", result.length());
        Serial.println(result);
        telnetSend("U: " + result);
    }
}

void sendCommand() {
    if (!sendBuffer.isEmpty()) {
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
        if (digitalRead(PIN_5_PIN) != LOW) {
            Serial.printf("ERROR: Pin5 went high before command before flush : %u\n", delayTime);
            delayTime = 0;
            sendBuffer.dequeue();
        }
        // tub.flush(true);
        if (digitalRead(PIN_5_PIN) == LOW) {
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

String HexString2TimeString(String hexstring) {
    // Convert "HHMM" in HEX to "HH:MM" with decimal representation
    String time = "";
    int hour = strtol(hexstring.substring(0, 2).c_str(), NULL, 16);
    int minute = strtol(hexstring.substring(2, 4).c_str(), NULL, 16);

    if (hour < 10) time.concat("0");  // Add leading zero
    time.concat(hour);
    time.concat(":");
    if (minute < 10) time.concat("0");  // Add leading zero
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
        if (b == '\0') break;
        temp += b;
    }
    return temp;
}

void hexCharacterStringToBytes(byte* byteArray, const char* hexString) {
    bool oddLength = strlen(hexString) & 1;

    byte currentByte = 0;
    byte byteIndex = 0;

    for (byte charIndex = 0; charIndex < strlen(hexString); charIndex++) {
        bool oddCharIndex = charIndex & 1;

        if (oddLength) {
            // If the length is odd
            if (oddCharIndex) {
                // odd characters go in high nibble
                currentByte = nibble(hexString[charIndex]) << 4;
            } else {
                // Even characters go into low nibble
                currentByte |= nibble(hexString[charIndex]);
                byteArray[byteIndex++] = currentByte;
                currentByte = 0;
            }
        } else {
            // If the length is even
            if (!oddCharIndex) {
                // Odd characters go into the high nibble
                currentByte = nibble(hexString[charIndex]) << 4;
            } else {
                // Odd characters go into low nibble
                currentByte |= nibble(hexString[charIndex]);
                byteArray[byteIndex++] = currentByte;
                currentByte = 0;
            }
        }
    }
}

byte nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';

    if (c >= 'a' && c <= 'f') return c - 'a' + 10;

    if (c >= 'A' && c <= 'F') return c - 'A' + 10;

    return 0;  // Not a valid hexadecimal character
}
