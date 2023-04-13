// Super basic example to just try and send a command when we see the pin5 go low

//WARNING
//Replace with your PIN GPIO for RX/TX
#define rxPinExternalSerialData 19
#define txPinExternalSerialData 23

#define RTS_PIN 22  // RS485 direction control, RequestToSend TX or RX, required for MAX485 board.

#define PIN_5_PIN 18

//Buffer pour la commande vers le SPA
#define maxSizeCommandBufferLength 16
int iCurrentCommandBufferLength=0;
uint8_t commandBuffer[maxSizeCommandBufferLength];

uint8_t keyboardCommand_UP[] = { 0xFB,0x06,0x66,0x66,0x66,0x66,0x01,0xFE,0x52};
uint8_t keyboardCommand_LIGHT[] = { 0xFB,0x06,0x03,0x45,0x0E,0x00,0x09,0xF6,0xF6};

HardwareSerial SerialExternalDataTub(2); // pins set during begin


void setup() {
  Serial.begin(115200);

  pinMode(PIN_5_PIN, INPUT);

  pinMode(RTS_PIN, OUTPUT);
  Serial.printf("Setting RTS pin %u HIGH\n", RTS_PIN);
  digitalWrite(RTS_PIN, HIGH);

  SerialExternalDataTub.begin(115200,SERIAL_8N1,rxPinExternalSerialData,txPinExternalSerialData,false);


  int lengthCommand = sizeof(keyboardCommand_LIGHT);
  memcpy(commandBuffer,keyboardCommand_LIGHT,lengthCommand);
  iCurrentCommandBufferLength=lengthCommand;

}

int cmdDelay = 0;
void loop() {
  // Serial.printf("pin:%u\n", digitalRead(PIN_5_PIN));
  if(digitalRead(PIN_5_PIN) == LOW) {
    delayMicroseconds(cmdDelay);
    SerialExternalDataTub.write(commandBuffer, iCurrentCommandBufferLength);
    int pinState = digitalRead(PIN_5_PIN);
    Serial.printf("Send command after %u delay", cmdDelay);
    if(pinState == HIGH) {
      Serial.println(" LATE");
    }
    else {
      Serial.println(" OK");
    }
    cmdDelay += 10;
    if(cmdDelay > 5000) cmdDelay = 0;
    delay(1000); // wait to give chance to see if that value was the magic delay
  }
  else {
    // Serial.println("HIGH");
  }
}
