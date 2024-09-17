#ifndef GLCLIMATE_H
#define GLCLIMATE_H

#include "esphome.h"
#include "esphome/core/preferences.h"
#include "esphome/components/climate/climate.h"
#include "esphome/core/log.h"
#include "esp_log.h"
#include "../ESPBalboaGL.h"

#include "balboaGL.h"

using namespace esphome;

class BalboaGLClimate : public PollingComponent, public climate::Climate {
 public:
        void set_spa(balboaGL* spa) {
            this->spa = spa;
        }
        // print the current configuration
        void dump_config() override;

        // Set up the component, initializing the balboaGL object.
        void setup() override;

        // This is called every poll_interval.
        void update() override;

        // Configure the climate object with traits that we support.
        climate::ClimateTraits traits() override;

        // Get a mutable reference to the traits that we support.
        climate::ClimateTraits& config_traits();

        // Debugging function to print the object's state.
        // void dump_state();

        // Handle a request from the user to change settings.
        void control(const climate::ClimateCall &call) override;

        // // Use the temperature from an external sensor. Use
        // // set_remote_temp(0) to switch back to the internal sensor.
        // void set_remote_temperature(float);

  protected:
        // The ClimateTraits supported by this HeatPump.
        climate::ClimateTraits traits_;

  private:
        balboaGL* spa;
};

#endif