#define COMMAND_UP          "fb0603450e0001fe66"
#define COMMAND_DOWN        "fb0603450e0002fd50"
//#define COMMAND_TOGGLE      "fb066666666609f6c2"
#define COMMAND_TIME        "fb0664d4060003fc76"
#define COMMAND_CHANGE_MODE "fb0603450e0004fb3c"
#define COMMAND_AUX         "fb06666666660af5f4" // TODO: FIXME - Not actual value
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

#ifdef AUX_DUAL_SPEED
#define AUX_STATE_HIGH 2
#else
#define AUX_STATE_HIGH 1
#endif 
