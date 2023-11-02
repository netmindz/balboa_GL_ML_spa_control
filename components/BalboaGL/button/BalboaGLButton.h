#pragma once

#include "esphome/components/button/button.h"

using namespace esphome;

class UpButton : public button::Button {
 public:
  void press_action() {}

};

class DownButton : public button::Button {
 public:
  void press_action() {}

};

class ModeButton : public button::Button {
 public:
  void press_action() {}

};
