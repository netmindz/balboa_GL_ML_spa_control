//Keyboard Command
//WARNING. replace 0x66,0x66,0x66,0x66  with your keyboard UID ( analyse your frame FB COMMAND to find it )
// #define COMMAND_EMPTY       "fb06 03450e00 00 ff74"
// #define COMMAND_UP          "fb06 03450e00 01 fe66"
// #define COMMAND_DOWN        "fb06 03450e00 02 fd50"
// #define COMMAND_TIME        "fb06 66666666 03 fc76"
// #define COMMAND_CHANGE_MODE "fb06 03450e00 04 fb3c"
// WHAT ABOUT 5? (tmjo temp toggle?)
// #define COMMAND_JET1        "fb06 03430600 06 f9a2" // TODO: fix me - swapped to tmjo's panel as mine was one char short in my notes
// #define COMMAND_JET2        "fb06 03430600 07 f8b0" // TODO: fix me - swapped to tmjo's panel as mine was one char short in my notes
// WHAT ABOUT 8?
// #define COMMAND_TOGGLE      "fb06 66666666 09 f6c2"  //SAME AS LIGHT??
// #define COMMAND_LIGHT       "fb06 03450e00 09 f6f6"
// #define COMMAND_BLOWER      "fb06 66666666 0a f5f4"
// WHAT ABOUT B
// WHAT ABOUT C
// WHAT ABOUT D
// WHAT ABOUT F


uint8_t CMD_EMPTY[] =       {0xFB,0x06,0x03,0x45,0x0e,0x00,0x00,0xFF,0x74};
uint8_t CMD_UP[] =          {0xFB,0x06,0x03,0x45,0x0e,0x00,0x01,0xFE,0x66};
uint8_t CMD_DOWN[] =        {0xFB,0x06,0x03,0x45,0x0e,0x00,0x02,0xFD,0x50};
uint8_t CMD_TIME[] =        {0xFB,0x06,0x66,0x66,0x66,0x66,0x03,0xFC,0x76};     //VERIFY
uint8_t CMD_CHANGE_MODE[] = {0xFB,0x06,0x03,0x45,0x0e,0x00,0x04,0xFB,0x3C};
// 5? (tmjo temp toggle??)
uint8_t CMD_JET1[] =        {0xFB,0x06,0x03,0x45,0x0e,0x00,0x06,0xF9,0xA2};
uint8_t CMD_JET2[] =        {0xFB,0x06,0x03,0x45,0x0e,0x00,0x07,0xF8,0xB0};
// 8?
uint8_t CMD_TOGGLE[] =      {0xFB,0x06,0x66,0x66,0x66,0x66,0x09,0xF6,0xC2};     //VERIFY
uint8_t CMD_LIGHT[] =       {0xFB,0x06,0x03,0x45,0x0e,0x00,0x09,0xF6,0xF6};
uint8_t CMD_BLOWER[] =      {0xFB,0x06,0x03,0x45,0x0e,0x00,0x0A,0xF5,0xF4};     //VERIFY
// B
// C
// D
// E
// F


#define COMMAND_UP          "fb0603450e0001fe66"
#define COMMAND_DOWN        "fb0603450e0002fd50"
//#define COMMAND_TOGGLE      "fb066666666609f6c2"
// #define COMMAND_TIME        "fb066666666603fc76"
#define COMMAND_CHANGE_MODE "fb0603450e0004fb3c"
// #define COMMAND_BLOWER      "fb06666666660af5f4"
#define COMMAND_JET1        "fb060343060006f9a2" // TODO: fix me - swapped to tmjo's panel as mine was one char short in my notes
#define COMMAND_JET2        "fb060343060007f8b0" // TODO: fix me - swapped to tmjo's panel as mine was one char short in my notes
#define COMMAND_EMPTY       "fb0603450e0000ff74"
#define COMMAND_LIGHT       "fb0603450e0009f6f6"




// Std;Eco;Sleep
#define MODE_IDX_STD 0
#define MODE_IDX_ECO 1
#define MODE_IDX_SLP 2

// Uncomment if you have dual-speed pump
#define PUMP1_DUAL_SPEED
#define PUMP2_DUAL_SPEED


#ifdef PUMP1_DUAL_SPEED
#define PUMP1_STATE_HIGH 2
#else
#define PUMP1_STATE_HIGH 1
#endif 

#ifdef PUMP2_DUAL_SPEED
#define PUMP2_STATE_HIGH 2
#else
#define PUMP2_STATE_HIGH 1
#endif 


// Perform measurements or read nameplate values on your tub to define the power [kW]
// for each device in order to calculate tub power usage
const float POWER_HEATER = 2.8;
const float POWER_PUMP_CIRCULATION = 0.3;
const float POWER_PUMP1_LOW = 0.31;
const float POWER_PUMP1_HIGH = 1.3;
const float POWER_PUMP2_LOW = 0.3;
const float POWER_PUMP2_HIGH = 0.6;

const int MINUTES_PER_DEGC = 45; // Tweak for your tub - would be nice to auto-learn in the future to allow for outside temp etc