#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
  namespace empty_text_sensor {

  using namespace text_sensor;

  class BalboaGLSensor : public TextSensor, public Component {
  public:
    void setup() override;
    void update() override;
    TextSensor *status_sensor = new TextSensor();

  };

  }
}  
