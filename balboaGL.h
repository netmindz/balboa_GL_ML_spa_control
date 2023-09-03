
#ifndef BALBOAGL_H
#define BALBOAGL_H

#include <Arduino.h>
#include <ArduinoQueue.h>

#include "constants.h"

ArduinoQueue<String> sendBuffer(10);  // TODO: might be better bigger for large temp changes. Would need testing

String result = "";
int msgLength = 0;


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
} status;

void sendCommand(String command, int count);

void setOption(int currentIndex, int targetIndex, int options, String command = COMMAND_DOWN);

void handleBytes(size_t len, uint8_t buf[]);

void handleMessage();

void sendCommand();

String HexString2TimeString(String hexstring);

String HexString2ASCIIString(String hexstring);

byte nibble(char c);

void hexCharacterStringToBytes(byte* byteArray, const char* hexString);

#endif
