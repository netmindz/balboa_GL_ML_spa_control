#define RX_PIN 19
#define TX_PIN 23
#define PORT_4_PIN 26
#define PIN_5_PIN 18
#define RTS_PIN 22  // RS485 direction control, RequestToSend TX or RX, required for MAX485 board.


void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  pinMode(PORT_4_PIN, INPUT);
  pinMode(PIN_5_PIN, INPUT);

  attachInterrupt(PIN_5_PIN, readMessage, FALLING);
  // attachInterrupt(digitalPinToInterrupt(PIN_5_PIN), clearBuffer, HIGH);

  // MAX485
  pinMode(RTS_PIN, OUTPUT);
  Serial.printf("Setting RTS pin %u LOW\n", RTS_PIN);
  digitalWrite(RTS_PIN, LOW);
  Serial.println("Setup Complete");
}

String tmp;
int i = 0;

void loop() {

  // // bool something = digitalRead(PORT_4_PIN);
  // bool panelSelect = digitalRead(PIN_5_PIN);  // LOW when we are meant to read data

  // if (panelSelect == LOW) {  // Only read data meant for us
  //                            // Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  // } else {
  //   if (tmp != "") {
  //     Serial.print("EOM  = ");
  //     Serial.println(tmp);
  //   }
  //   // Serial2.end();
  //   Serial2.flush();  // send tx buffer and flush rx
  //   tmp = "";
  // }
}

void IRAM_ATTR readMessage() {
    Serial.println("readMessage");

    Serial2.flush(); // Clear data from other main panels from buffer

    // delayMicroseconds(100); // Wait for message to be fully in the RX buffer after the pin changed LOW
    
    size_t len = Serial2.available();

    Serial.printf("aval = %u\n", len);

    uint8_t buf[len];
    Serial2.read(buf, len);

    // buildString(buf, len);
    // if (tmp.length() == 46 && tmp.substring(0, 4) == "fa14") {
    //   i++;
    //   Serial.print("FA14 = ");
    //   Serial.println(tmp);
    //   // sendCommand();
    // }
    // else if (tmp.length() == 32 && tmp.substring(0, 4) == "ae0d") {
    //   Serial.print("AEOD = ");
    //   Serial.println(tmp);
    //   // sendCommand();
    // }
    // else {
    //   Serial.printf("EOM %u = ", tmp.length());
    //   Serial.println(tmp);
    // }

}

void sendCommand() {
  String sendBuffer = "fb0603450e0009f6f6";  // toggle light
  i++;
  if (i % 100 == 0) {
    Serial.printf("availableForWrite before send = %u\n", Serial2.availableForWrite());
    unsigned long startTime = micros();
    digitalWrite(RTS_PIN, HIGH);
    logTimeDiff("high", startTime);
    // delayMicroseconds(100);
    // Serial.println("Sending " + sendBuffer);
    byte byteArray[18] = { 0 };
    hexCharacterStringToBytes(byteArray, sendBuffer.c_str());
    // if(digitalRead(PIN_5_PIN) != LOW) {
    //   Serial.println("ERROR: Pin5 went high before command before write");
    // }
    Serial2.write(byteArray, sizeof(byteArray));
    logTimeDiff("Send write time", startTime);
    // Serial2.write(sendBuffer.c_str());
    // if(digitalRead(PIN_5_PIN) != LOW) {
    //   Serial.println("ERROR: Pin5 went high before command before flush");
    // }
    Serial.printf("availableForWrite before flush = %u\n", Serial2.availableForWrite());
    Serial2.flush(true);
    logTimeDiff("Send write time", startTime);
    Serial.printf("availableForWrite post flush = %u\n", Serial2.availableForWrite());
    if (digitalRead(PIN_5_PIN) == LOW) {
      sendBuffer = "";
      Serial.println("YAY: message sent");
    } else {
      Serial.println("ERROR: Pin5 went high before command could be sent after flush");
    }
    digitalWrite(RTS_PIN, LOW);
    logTimeDiff("Send time", startTime);
  }
}

void logTimeDiff(String msg, long startTime) {
  Serial.print(msg + " ");
  Serial.println(micros() - startTime);
}

void buildString(uint8_t buf[], size_t len) {
  for (int i = 0; i < len; i++) {
    if (buf[i] < 0x10) {
      tmp += '0';
    }
    tmp += String(buf[i], HEX);
  }
}

void hexCharacterStringToBytes(byte *byteArray, const char *hexString) {
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
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  return 0;  // Not a valid hexadecimal character
}