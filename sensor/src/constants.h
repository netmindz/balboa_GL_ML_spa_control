#define COMMAND_UP          "fb0664d4060001fe52"
#define COMMAND_DOWN        "fb0664d4060002fd64"
#define COMMAND_TIME        "fb0664d4060003fc76"
#define COMMAND_CHANGE_MODE "fb0664d4060004fb08"
#define COMMAND_JET1        "fb0664d4060006f92c"
#define COMMAND_JET2        "fb0664d4060007f83e"
#define COMMAND_EMPTY       "fb0664d4060000ff40"
#define COMMAND_LIGHT       "fb0664d4060009f6c2"



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