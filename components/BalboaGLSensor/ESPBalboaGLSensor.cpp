#include <string>
#include "ESPBalboaGLSensor.h"

using namespace text_sensor;

class BalboaGLSensor : public PollingComponent {
 public:
  TextSensor *status_sensor = new TextSensor();

  BalboaGLSensor() : PollingComponent(15000) { }

  void setup() override {
  }

  void update() override {
    status_sensor->publish_state(status.state.c_str());
  }
};
