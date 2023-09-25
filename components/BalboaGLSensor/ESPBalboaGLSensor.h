#pragma once

#include "esphome.h"
#include "esphome/components/text_sensor/text_sensor.h"


namespace esphome {
  namespace balboa_sensor {

  static const char* TAG = "BalboaGLSensor"; // Logging tag

  using namespace text_sensor;

  class BalboaGLStateSensor : public TextSensor, public PollingComponent {
    public:
      BalboaGLStateSensor() : PollingComponent(15000) { }
      // void setup() {};
      void update() {
        std::string state = status.state.c_str();
        ESP_LOGD(TAG, state.c_str());
        this->publish_state(state.c_str());
      }

  };

  class BalboaGLRawSensor : public TextSensor, public PollingComponent {
    public:
      BalboaGLRawSensor() : PollingComponent(15000) { }
      // void setup() {};
      void update() {
        this->publish_state(status.rawData.c_str());
      }

  };

  }
}  
