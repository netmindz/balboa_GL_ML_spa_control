
#ifndef BALBOAGL_H
#define BALBOAGL_H

#include <Arduino.h>
#include <ArduinoQueue.h>

#include "constants.h"

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

extern int RTS_PIN;
extern int PIN_5_PIN;

#define tub Serial2


extern ArduinoQueue<String> sendBuffer;

extern String result;
extern int msgLength;

struct BalboaStatus {
    float power;
    String rawData;
    String rawData2; // TODO: better name
    String rawData3; // TODO: better name
    String rawData7; // TODO: better name
    float targetTemp;
    float temp;
    float timeToTemp;
    int mode;
    int pump1;
    int pump2;
    String time;
    boolean heater;
    boolean light;
    String state;
};

extern struct BalboaStatus status;

void sendCommand(String command, int count);

void setOption(int currentIndex, int targetIndex, int options, String command);

void handleBytes(size_t len, uint8_t buf[]);

void handleMessage();

void sendCommand();

String HexString2TimeString(String hexstring);

String HexString2ASCIIString(String hexstring);

byte nibble(char c);

void hexCharacterStringToBytes(byte* byteArray, const char* hexString);

void telnetSend(String message);

#endif
