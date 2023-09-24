#include <string>
#include "ESPBalboaGLSensor.h"


BalboaGLSensor::BalboaGLSensor() : PollingComponent(15000) { }

void BalboaGLSensor::setup()  {
}

void BalboaGLSensor::update()  {
  status_sensor->publish_state(status.state.c_str());
}
