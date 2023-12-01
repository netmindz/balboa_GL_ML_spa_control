#pragma once

#include "esphome/components/button/button.h"
#include "balboaGL.h"

using namespace esphome;

class UpButton : public PollingComponent, public button::Button {
 public:
  void set_spa(balboaGL* spa) {
      this->spa = spa;
  }
  void press_action() {
    this->spa->buttonPressUp();
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
    this->spa->buttonPressDown();
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
    this->spa->buttonPressMode();
  }
  void update() {}
  private:
        balboaGL* spa;

};
