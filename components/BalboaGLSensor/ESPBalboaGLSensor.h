#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
  namespace balboa_sensor {

  using namespace text_sensor;

  class BalboaGLSensor : public TextSensor, public Component {
    public:
      BalboaGLSensor::BalboaGLSensor() : PollingComponent(15000) { }
      void setup();
      void update();
      TextSensor *status_sensor = new TextSensor();

  };

  }
}  
