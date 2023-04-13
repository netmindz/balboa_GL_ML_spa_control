// Example code of command sending from https://github.com/TiSiAi based of orginal investigations by netmindz

// Note: any reference to Keyboard here actually means main/top panel

//Enable for analyse Original KeyBoard > no TX Data Enable
//#define DEBUG_ORIGINAL_KEYBOARD 1

unsigned long lastExternalSerialDataValidTime = 2000UL;


//WARNING
//Replace with your PIN GPIO for RX/TX
#define rxPinExternalSerialData 19
#define txPinExternalSerialData 23

#define RTS_PIN 22  // RS485 direction control, RequestToSend TX or RX, required for MAX485 board.


//WARNING
//Pin Keyboard
//>> modify with your PIN5 from your Keyboard
#define PIN_KEY_BOARD_1 22
#define PIN_KEY_BOARD_2 5  //my original Keyboard for debug FB Frame 


//Keyboard Command
//WARNING. replace 0x66,0x66,0x66,0x66  with your keyboard UID ( analyse your frame FB COMMAND to find it )
// FB FF 2A 39 00 00 FF 74
// fb 06 03 45 0e 00 02 fd 50
// fb 06 03 45 0e 00 09 f6 f6
uint8_t keyboardCommand_UP[] = { 0xFB,0x06,0x66,0x66,0x66,0x66,0x01,0xFE,0x52};
uint8_t keyboardCommand_DOWN[] = { 0xFB,0x06,0x66,0x66,0x66,0x66,0x02,0xFD,0x64};
uint8_t keyboardCommand_TOGGLE[] = { 0xFB,0x06,0x66,0x66,0x66,0x66,0x09,0xF6,0xC2};
uint8_t keyboardCommand_TIME[] = { 0xFB,0x06,0x66,0x66,0x66,0x66,0x03,0xFC,0x76};
uint8_t keyboardCommand_CHANGE_MODE[] = { 0xFB,0x06,0x66,0x66,0x66,0x66,0x04,0xFB,0x08};

uint8_t keyboardCommand_BLOWER[] = { 0xFB,0x06,0x66,0x66,0x66,0x66,0x0A,0xF5,0xF4};

uint8_t keyboardCommand_JET1[] = { 0xFB,0x06,0x66,0x66,0x66,0x66,0x06,0xF9,0x2C};
uint8_t keyboardCommand_JET2[] = { 0xFB,0x06,0x66,0x66,0x66,0x66,0x07,0xF8,0x3E};

uint8_t keyboardCommand_EMPTY[] ={ 0xFB,0x06,0x66,0x66,0x66,0x66,0x00,0xFF,0x40};



int gpioKeyboardPin = PIN_KEY_BOARD_1; //Default ( my virtual new Keyboard with ESP32)

//Buffer pour la commande vers le SPA
#define maxSizeCommandBufferLength 16
int iCurrentCommandBufferLength=0;
uint8_t commandBuffer[maxSizeCommandBufferLength];

void clearCommandBuffer();


#define maxSizeDataBufferLength 200
int iLastPinState =0;

int iCurrentBufferLength=0;
int iPrecBufferLength=0;
uint8_t serialDataBuffer[maxSizeDataBufferLength];


unsigned long lastTimeLastData = 0;
unsigned long timeBeetweenFrame = 290UL;  //290us
unsigned long timeBeetweenFrame_original_keyboard = 1000UL;  //on laisse 1 ms au clavier d'origine

int nbFrameProcess = 0;
unsigned long lastTimeLastDataAnalysing = 0;
unsigned long timeBeetweenFrameAnalysing = 1*3000000UL;  //1ms

HardwareSerial SerialExternalDataTub(2); // pins set during begin

int iPinState=-1;

//Function declaration
inline uint8_t crc8(uint8_t *data,uint8_t dataLength,uint8_t crcStart,uint8_t crcEnd);
void setCommand_SetTime();
void setCommand_SetProg();
void setCommand_SetUp();
void setCommand_SetDown();
void setCommand_SetJet1();
void setCommand_SetJet2();
void setCommand_SetLight();
void setCommand_SetBlower();



void setup() 
{
#ifdef DEBUG_ORIGINAL_KEYBOARD
    gpioKeyboardPin = PIN_KEY_BOARD_2;
#endif

  pinMode(PIN_KEY_BOARD_1, INPUT);
  pinMode(PIN_KEY_BOARD_2, INPUT);

  pinMode(RTS_PIN, OUTPUT);
  Serial.printf("Setting RTS pin %u LOW\n", RTS_PIN);
  digitalWrite(RTS_PIN, LOW);
  
  clearDataBuffer();
  clearCommandBuffer();
  
  Serial.begin(115200);

  delay(1000);

  SerialExternalDataTub.setTimeout(2000);
  SerialExternalDataTub.begin(115200,SERIAL_8N1,rxPinExternalSerialData,txPinExternalSerialData,false);
  
  pinMode(RTS_PIN, OUTPUT);
  Serial.printf("Setting RTS pin %u LOW\n", RTS_PIN);
  digitalWrite(RTS_PIN, LOW);

}


void clearDataBuffer()
{
  iCurrentBufferLength=0;
  memset(serialDataBuffer,0,maxSizeDataBufferLength);
}

//Buffer utilisé pour envoyé une commande au SPA
void clearCommandBuffer()
{
  iCurrentCommandBufferLength=0;
  memset(commandBuffer,0,maxSizeCommandBufferLength);
}


void loop() 
{

  ProcessSerialDataBuffer();

   //Frame end time different if we analyze a keyboard or if it's ours
   //When we analyze a keyboard, we must wait for the return command to be sent (> 1ms)
   //Otherwise about 300us
   unsigned long dynamicTimeBeetweenFrame = (gpioKeyboardPin==PIN_KEY_BOARD_1)?timeBeetweenFrame:timeBeetweenFrame_original_keyboard;
  
  if(((micros()-lastTimeLastData) > dynamicTimeBeetweenFrame) && (iCurrentBufferLength>1))
  {
    //if(iPinState>0) if use opto coupler ( the signal is reverse )
    if(iPinState<=0)
    {

      //Sample to prepare a command  ( add a command from a push button or wifi command, BLE )
      //setCommand_SetUp();

      //Immediately send the order to the SPA when an order is prepared
      if(iCurrentCommandBufferLength>0)
      {
        sendCommandBuffer(commandBuffer,iCurrentCommandBufferLength);
        clearCommandBuffer();
      }

      //Print Data Frame
      PrintSerialBuffer(false);
    }
    
    clearDataBuffer();
  }
}


//Envoi de la commande sur la liaison série
void sendCommandBuffer(uint8_t* buffer,int bufferLength)
{
  //Security, no command sent if you are in debug mode of the Physical keyboard
  if(gpioKeyboardPin!=PIN_KEY_BOARD_1)
  {
    Serial.println("Command not sended > Original KeyBoard");
    return;
  }

  if(bufferLength<=0)
  {
    Serial.println("Command not sended > No Data");
    return;
  }
  digitalWrite(RTS_PIN, HIGH);

  SerialExternalDataTub.write(buffer,bufferLength);
  
  Serial.print("SPA Cmd: ");
  for(int i=0;i<bufferLength;i++)
  {
    printHexa2Digit((byte)buffer[i],false);
  }
  Serial.println("");
  digitalWrite(RTS_PIN, LOW);
} 



void PrintSerialBuffer(bool bActiveFilter)
{
  int iData=0;
  int iDataBuffer = 0;

  bool bFilter=false;

 
  if(iCurrentBufferLength>3)
  {
    //Enable if you want filter frame
    /*
    if((serialDataBuffer[0]==0xFA)&&(serialDataBuffer[1]==0x14))
    {
      bFilter=true;
    }
    */
    //Enable if you want filter frame
    /*
    if(serialDataBuffer[0]==0xAE)
    {
      bFilter=true;
    }
    */
  }

  if(bFilter && bActiveFilter)
  {
    return;
  }

  
  for(iDataBuffer=0;iDataBuffer<iCurrentBufferLength;iDataBuffer++)
  {
    iData=serialDataBuffer[iDataBuffer];
    printHexa2Digit((byte)iData,false);
  }
  if(iCurrentBufferLength>0)
  {
    Serial.print(" <");
    Serial.println(iCurrentBufferLength);
  }
}

//Empty all serial data
void EmptySerialDataBuffer()
{
  while(SerialExternalDataTub.available()) 
  { 
    SerialExternalDataTub.read();
  }
  clearDataBuffer();
}

bool ProcessSerialDataBuffer()
{
  int bPass=false;
  while(SerialExternalDataTub.available()) 
  { 
    if(!bPass)
    {
      bPass=true;
    }
    
    int iData = SerialExternalDataTub.read();
    if(iCurrentBufferLength<maxSizeDataBufferLength)
    {
      serialDataBuffer[iCurrentBufferLength]=(byte)iData;
      iCurrentBufferLength++;
    }
    else
    {
      EmptySerialDataBuffer();
      Serial.println("Overflow");
    }
  }

  //Si on a eu des données
  if(bPass)
  {
    lastTimeLastData = micros();
    iPinState = digitalRead(gpioKeyboardPin);
  }
}

void setCommand_SetTime()
{
  clearCommandBuffer();
  int lengthCommand = sizeof(keyboardCommand_TIME);
  memcpy(commandBuffer,keyboardCommand_TIME,lengthCommand);
  iCurrentCommandBufferLength=lengthCommand;
  //Serial.println("CMD_SET_TIME");
}
void setCommand_SetProg()
{
  clearCommandBuffer();
  int lengthCommand = sizeof(keyboardCommand_CHANGE_MODE);
  memcpy(commandBuffer,keyboardCommand_CHANGE_MODE,lengthCommand);
  iCurrentCommandBufferLength=lengthCommand;
  //Serial.println("CMD_SET_PROG");
}
void setCommand_SetUp()
{
  clearCommandBuffer();
  int lengthCommand = sizeof(keyboardCommand_UP);
  memcpy(commandBuffer,keyboardCommand_UP,lengthCommand);
  iCurrentCommandBufferLength=lengthCommand;
  //Serial.println("CMD_SET_UP");
}
void setCommand_SetDown()
{
  clearCommandBuffer();
  int lengthCommand = sizeof(keyboardCommand_DOWN);
  memcpy(commandBuffer,keyboardCommand_DOWN,lengthCommand);
  iCurrentCommandBufferLength=lengthCommand;
  //Serial.println("CMD_SET_DOWN");
}
void setCommand_SetJet1()
{
  clearCommandBuffer();
  int lengthCommand = sizeof(keyboardCommand_JET1);
  memcpy(commandBuffer,keyboardCommand_JET1,lengthCommand);
  iCurrentCommandBufferLength=lengthCommand;
  //Serial.println("CMD_SET_JET1");
}
void setCommand_SetJet2()
{
  clearCommandBuffer();
  int lengthCommand = sizeof(keyboardCommand_JET2);
  memcpy(commandBuffer,keyboardCommand_JET2,lengthCommand);
  iCurrentCommandBufferLength=lengthCommand;
  //Serial.println("CMD_SET_JET2");
}
void setCommand_SetLight()
{
  clearCommandBuffer();
  int lengthCommand = sizeof(keyboardCommand_TOGGLE);
  memcpy(commandBuffer,keyboardCommand_TOGGLE,lengthCommand);
  iCurrentCommandBufferLength=lengthCommand;
  //Serial.println("CMD_SET_LIGHT");
}
void setCommand_SetBlower()
{
  clearCommandBuffer();
  int lengthCommand = sizeof(keyboardCommand_BLOWER);
  memcpy(commandBuffer,keyboardCommand_BLOWER,lengthCommand);
  iCurrentCommandBufferLength=lengthCommand;
  //Serial.println("CMD_SET_BLOWER");
}

void printHexa2Digit(byte iVal,bool bSplit)
{
  char buff[3];
  if(!bSplit)
  {
    sprintf(buff,"%02X",iVal);
    Serial.print(buff);
  }
  else
  {
    sprintf(buff,"%X",((iVal&0xF0)>>4));
    Serial.print(buff);
    Serial.print(" ");
    sprintf(buff,"%1X",iVal&0X0F);
    Serial.print(buff);
  }  

}


inline uint8_t crc8(uint8_t *data,uint8_t dataLength,uint8_t crcStart,uint8_t crcEnd)
{
  unsigned long crc;
  int i, bit;
  uint8_t length = dataLength;

  crc = crcStart;
  for ( i = 0 ; i < length ; i++ ) {
    crc ^= data[i];
    for ( bit = 0 ; bit < 8 ; bit++ ) {
      if ( (crc & 0x80) != 0 ) {
        crc <<= 1;
        crc ^= 0x7;
      }
      else {
        crc <<= 1;
      }
    }
  }
  return crc ^ crcEnd;
}

