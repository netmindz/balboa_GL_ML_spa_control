#include "esphome.h"
#include "esphome/core/preferences.h"

#include "constants.h"


// Perform measurements or read nameplate values on your tub to define the power [kW]
// for each device in order to calculate tub power usage
const float POWER_HEATER = 2.8;
const float POWER_PUMP_CIRCULATION = 0.3;
const float POWER_PUMP1_LOW = 0.31;
const float POWER_PUMP1_HIGH = 1.3;
const float POWER_PUMP2_LOW = 0.3;
const float POWER_PUMP2_HIGH = 0.6;

// Tweak for your tub - would be nice to auto-learn in the future to allow for outside temp etc
const int MINUTES_PER_DEGC = 45;


#define tub Serial2
#define RX_PIN 19
#define TX_PIN 23
#define RTS_PIN 22  // RS485 direction control, RequestToSend TX or RX, required for MAX485 board.
#define PIN_5_PIN 18
#define LED_BUILTIN 2


#include "balboaGL.h"

using namespace esphome;

class BalboaGL : public Component, public climate::Climate {
 public:
  BalboaGL(
            HardwareSerial* hw_serial,
            uint32_t poll_interval=ESPMHP_POLL_INTERVAL_DEFAULT
        );
  private:
        // Retrieve the HardwareSerial pointer from friend and subclasses.
        HardwareSerial *hw_serial_; 
}
