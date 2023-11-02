#pragma once

#include "esphome/components/button/button.h"

using namespace esphome;

class UpButton : public PollingComponent, public button::Button {
 public:
  void press_action() {}
  void update() {}

};

class DownButton : public PollingComponent, public button::Button {
 public:
  void press_action() {}
  void update() {}
};

class ModeButton : public PollingComponent, public button::Button {
 public:
  void press_action() {}
  void update() {}

};
