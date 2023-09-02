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
  void setup() override {
    // This will be called by App.setup()
  }
  void control(const ClimateCall &call) override {
    if (call.get_mode().has_value()) {
      // User requested mode change
      ClimateMode mode = *call.get_mode();
      // Send mode to hardware
      // ...

      // Publish updated state
      this->mode = mode;
      this->publish_state();
    }
    if (call.get_target_temperature().has_value()) {
      // User requested target temperature change
      float temp = *call.get_target_temperature();
      // Send target temp to climate
      // ...
    }
  }
  ClimateTraits traits() override {
    // The capabilities of the climate device
    auto traits = climate::ClimateTraits();
    traits.set_supports_current_temperature(true);
    traits.set_supported_modes({climate::CLIMATE_MODE_HEAT_COOL});
    return traits;
  }
}
