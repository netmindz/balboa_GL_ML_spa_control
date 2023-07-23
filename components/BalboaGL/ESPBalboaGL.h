#include "esphome.h"
#include "balboaGL.h"

// using namespace esphome;

class BalboaGL : public Component, public Climate {
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
