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