
#ifndef BALBOAGL_H
#define BALBOAGL_H

#include <ArduinoQueue.h>

int delayTime = 40;

#include "constants.h"

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


String result = "";
int msgLength = 0;

void handleMessage();
void sendCommand();
String HexString2ASCIIString(String hexstring);
void hexCharacterStringToBytes(byte* byteArray, const char* hexString);
String HexString2TimeString(String hexstring);
void telnetSend(String message);

void handleBytes(size_t len, uint8_t buf[]) {

    for (int i = 0; i < len; i++) {
        if (buf[i] < 0x10) {
            result += '0';
        }
        result += String(buf[i], HEX);
    }
    if (msgLength == 0 && result.length() >= 2) {
        String messageType = result.substring(0, 2);
        if (messageType == "fa") {
            msgLength = FA_MESSAGE_LENGTH;
        } else if (messageType == "ae") {
            msgLength = 32;
        } else {
            Serial.print("Unknown message length for ");
            Serial.println(messageType);
        }
    } else if (result.length() >= msgLength) {
        if (msgLength == FA_MESSAGE_LENGTH) {
            sendCommand();  // send reply *before* we parse the FA string as we don't want to delay the reply by
                            // say sending MQTT updates
        }
        handleMessage();
    }
}

void handleMessage() {
    
    // Set as static, so only allocating memory once, not per method call - as when was global
    static int pump1State = 0;
    static int pump2State = 0;
    static boolean heaterState = false;
    static boolean lightState = false;
    static float tubpowerCalc = 0;
    static double tubTemp = -1;
    static double tubTargetTemp = -1;
    static String state = "unknown";
    static String lastRaw = "";
    static String lastRaw2 = "";
    static String lastRaw3 = "";
    static String lastRaw4 = "";
    static String lastRaw5 = "";
    static String lastRaw6 = "";
    static String lastRaw7 = "";
    static String timeString = "";

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

            status.power = tubpowerCalc;

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
                status.rawData = lastRaw.c_str();

                String s = result.substring(17, 18);
                if (s == "4") {
                    state = "Sleep";
                    status.mode = MODE_IDX_SLP;
                } else if (s == "9") {
                    state = "Circulation ?";
                    status.mode = MODE_IDX_STD;  // TODO: confirm
                } else if (s == "1") {
                    state = "Standard";
                    status.mode = MODE_IDX_STD;
                } else if (s == "2") {
                    state = "Economy";
                    status.mode = MODE_IDX_ECO;
                } else if (s == "a") {
                    state = "Cleaning";  // TODO: can't tell our actual mode here - could be any of the 3 I think
                } else if (s == "c") {
                    state = "Circulation in sleep?";
                    status.mode = MODE_IDX_SLP;
                } else if (s == "b" || s == "3") {
                    state = "Std in Eco";  // Was in eco, Swap to STD for 1 hour only
                    status.mode = MODE_IDX_STD;
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
                status.time = timeString.c_str();

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
                    status.rawData3 = lastRaw3.c_str();
                }

                if (result.substring(10, 12) == "43") {  // "C"
                    double tmp = (HexString2ASCIIString(result.substring(4, 10)).toDouble() / 10);
                    if (menu == "46") {
                        tubTargetTemp = tmp;
                        status.targetTemp = (float)tubTargetTemp;
                        Serial.printf("Sent target temp data %f\n", tubTargetTemp);
                    } else {
                        if (tubTemp != tmp) {
                            tubTemp = tmp;
                            status.temp = (float)tubTemp;
                            Serial.printf("Sent temp data %f\n", tubTemp);
                        }
                        if (heaterState && (tubTemp < tubTargetTemp)) {
                            double tempDiff = (tubTargetTemp - tubTemp);
                            float timeToTempValue = (tempDiff * MINUTES_PER_DEGC);
                            status.timeToTemp = timeToTempValue;
                        } else {
                            status.timeToTemp = 0;
                        }
                    }
                } else if (result.substring(10, 12) == "2d") {  // "-"
                                                                //          Serial.println("temp = unknown");
                                                                //          telnetSend("temp = unknown");
                } else {
                    Serial.println("non-temp " + result);
                    telnetSend("non-temp " + result);
                }

                status.state = state.c_str();
            }
        } else {
            // FA but not temp data
            lastRaw2 = result.substring(4, 28);
            status.rawData2 = lastRaw2.c_str();
        }

        if (result.length() >= 64) {  // "Long" messages only
            String tail = result.substring(46, 64);
            if (tail != lastRaw7) {
                lastRaw7 = tail;
                status.rawData7 = lastRaw7.c_str();
            }
        }

        status.pump1 = pump1State;
        status.pump2 = pump2State;
        status.heater = heaterState;
        status.light = lightState;

        // end of FA14
    } else if (result.substring(0, 4) == "ae0d") {
        // Serial.println("AE 0D");
        // telnetSend(result);

        String message = result.substring(0, 32);  // ignore any FB ending

        if (result.substring(0, 6) == "ae0d01" && message != "ae0d010000000000000000000000005a") {
            if (!lastRaw4.equals(message)) {
                lastRaw4 = message;
                // status.rawData4 = lastRaw4.c_str();
            }
        } else if (result.substring(0, 6) == "ae0d02" && message != "ae0d02000000000000000000000000c3") {
            if (!lastRaw5.equals(message)) {
                lastRaw5 = message;
                // status.rawData5 = lastRaw5.c_str();
            }
        } else if (result.substring(0, 6) == "ae0d03" && message != "ae0d03000000000000000000000000b4") {
            if (!lastRaw6.equals(message)) {
                lastRaw6 = message;
                // status.rawData6 = lastRaw6.c_str();
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
            // delayTime = 0;
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

byte nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;  // Not a valid hexadecimal character
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

#endif
