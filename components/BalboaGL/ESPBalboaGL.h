#include "esphome.h"
#include "esphome/core/preferences.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/climate/climate.h"


#ifndef ESPMHP_H
#define ESPMHP_H

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

// TODO: hard coded values for T-RSC3
#define RX_PIN_DEF 3
#define TX_PIN_DEF 10
#define RTS_PIN_DEF 5  // RS485 direction control, RequestToSend TX or RX, required for MAX485 board.
#define PIN_5_PIN_DEF 6

#include "balboaGL.h"

using namespace esphome;

class BalboaGL : public PollingComponent, public climate::Climate {
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

        void set_rx_pin(int pin);

        void set_tx_pin(int pin);

        float get_setup_priority() const override { return esphome::setup_priority::AFTER_WIFI; }

  protected:
        // The ClimateTraits supported by this HeatPump.
        climate::ClimateTraits traits_;

        //Accessor method for the HardwareSerial pointer
        HardwareSerial* get_hw_serial_() {
            return this->hw_serial_;
        }

        //Print a warning message if we're using the sole hardware UART on an
        //ESP8266 or UART0 on ESP32
        void check_logger_conflict_();

        int rx_pin = -1;
        int tx_pin = -1;

  private:
        // Retrieve the HardwareSerial pointer from friend and subclasses.
        HardwareSerial *hw_serial_; 

        balboaGL* spa;
};
#endif