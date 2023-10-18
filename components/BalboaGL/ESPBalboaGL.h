#ifndef ESPMHP_H
#define ESPMHP_H

#include "esphome.h"
#include "esphome/core/preferences.h"
// #include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/log.h"
#include "esp_log.h"

#include "BalboaGLSelect.h"
#include "BalboaGLSwitch.h"
#include "ESPBalboaGL.h"

static const char* TAG = "BalboaGL"; // Logging tag

// // Perform measurements or read nameplate values on your tub to define the power [kW]
// // for each device in order to calculate tub power usage
// const float POWER_HEATER = 2.8;
// const float POWER_PUMP_CIRCULATION = 0.3;
// const float POWER_PUMP1_LOW = 0.31;
// const float POWER_PUMP1_HIGH = 1.3;
// const float POWER_PUMP2_LOW = 0.3;
// const float POWER_PUMP2_HIGH = 0.6;

// // Tweak for your tub - would be nice to auto-learn in the future to allow for outside temp etc
// const int MINUTES_PER_DEGC = 45;

const uint32_t POLL_INTERVAL_DEFAULT = 10000;

#include "balboaGL.h"

using namespace esphome;

class BalboaGL : public PollingComponent {
 public:
      BalboaGL(
            HardwareSerial* hw_serial
        );

        // print the current configuration
        void dump_config() override;

        // // handle a change in settings as detected by the HeatPump library.
        // void hpSettingsChanged();

        // // Handle a change in status as detected by the HeatPump library.
        // void hpStatusChanged(heatpumpStatus currentStatus);

        // Set up the component, initializing the balboaGL object.
        void setup() override;

        // This is called every poll_interval.
        void update() override;

        // Debugging function to print the object's state.
        // void dump_state();

        void set_rx_pin(int pin);

        void set_tx_pin(int pin);

        void set_rts_pin(int pin);

        void set_panel_select_pin(int pin);

        float get_setup_priority() const override { return esphome::setup_priority::AFTER_WIFI; }

  protected:
        
        //Accessor method for the HardwareSerial pointer
        HardwareSerial* get_hw_serial_() {
            return this->hw_serial_;
        }

        //Print a warning message if we're using the sole hardware UART on an
        //ESP8266 or UART0 on ESP32
        void check_logger_conflict_();

        esphome::balboa_select::BalboaGLPump1Select* pump1;
        esphome::balboa_select::BalboaGLPump2Select* pump2;
        
        esphome::balboa_switch::BalboaGLLightSwitch* lightSwitch;

        esphome::text_sensor::TextSensor* stateSensor;
        esphome::text_sensor::TextSensor* rawSensor;
      //   esphome::text_sensor::TextSensor* lcdSensor;

        int rx_pin = -1;
        int tx_pin = -1;
        int rts_pin = -1;
        int panel_select_pin = -1;

  private:
        // Retrieve the HardwareSerial pointer from friend and subclasses.
        HardwareSerial *hw_serial_; 

        balboaGL* spa;
};
#endif