#include <string>
#include "ESPBalboaGLSensor.h"
#include "balboaGL.h"

namespace esphome {
  namespace balboa_sensor {

    void BalboaGLSensor::setup()  {
    }

    void BalboaGLSensor::update()  {
      std::string state = "test";
      ESP_LOGD(TAG, status.state.c_str());
      status_sensor->publish_state(state);
    }
  }
}