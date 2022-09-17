#define RX_PIN 19
#define TX_PIN 23
#define DIGITAL_PIN 18

void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:
  Serial2.begin(115200, SERIAL_8N1,RX_PIN,TX_PIN);
  pinMode(DIGITAL_PIN, INPUT);
  Serial.println("Setup Complete");
}

String tmp;
bool digitalState;
int i = 0;
void loop() {
  digitalState = digitalRead(DIGITAL_PIN);
  if (Serial2.available() > 0) {
    size_t len = Serial2.available();
//    Serial.printf("%u %s\n", len, (String) digitalState);
    uint8_t buf[len];
    Serial2.read(buf, len);
//    handleBytes(buf, len);    
    buildString(buf, len);    
  }
  if(i % 10 == 0) {
    if(tmp.length() == 46) {
      String sendBuffer = "fb0603450e0009f6f6"; // toggle light
      Serial.println("Sending " + sendBuffer);
      byte byteArray[18] = {0};
      hexCharacterStringToBytes(byteArray, sendBuffer.c_str());
      Serial2.write(byteArray, sizeof(byteArray));
    }
  }
  i++;
   if(digitalState == LOW) {
    if(tmp != "") {
      Serial.print("tmp = ");
      Serial.println(tmp);
      tmp = "";
    }
  }

}

void buildString(uint8_t buf[], size_t len) {
  for (int i = 0; i < len; i++) {
    if (buf[i] < 0x10) {
        tmp += '0';
     }
    tmp += String(buf[i], HEX);
  }
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
