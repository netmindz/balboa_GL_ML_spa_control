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
