#pragma once

#include "esphome/components/button/button.h"
#include "constants.h"

using namespace esphome;

class UpButton : public PollingComponent, public button::Button {
 public:
  void set_spa(balboaGL* spa) {
      this->spa = spa;
  }
  void press_action() {
    this->spa->queueCommand(COMMAND_UP, 1);
  }
  void update() {}
  private:
        balboaGL* spa;

};

class DownButton : public PollingComponent, public button::Button {
 public:
  void set_spa(balboaGL* spa) {
      this->spa = spa;
  }
  void press_action() {
    this->spa->queueCommand(COMMAND_DOWN, 1);
  }
  void update() {}
  private:
        balboaGL* spa;

};

class ModeButton : public PollingComponent, public button::Button {
 public:
  void set_spa(balboaGL* spa) {
      this->spa = spa;
  }
  void press_action() {
    this->spa->queueCommand(COMMAND_MODE, 1);
  }
  void update() {}
  private:
        balboaGL* spa;

};
