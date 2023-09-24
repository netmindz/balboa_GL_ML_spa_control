#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {

class BalboaGLSensor : public text_sensor::TextSensor, public Component {
 public:
  void setup() override;
  void update() override;
};

}  // namespace esphome
