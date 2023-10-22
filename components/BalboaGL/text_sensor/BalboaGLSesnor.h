#pragma once

#include "esphome.h"
#include "esphome/components/text_sensor/text_sensor.h"

// #include "balboaGL.h"

using namespace esphome;

class BalboaGLStateSensor : public PollingComponent, public text_sensor::TextSensor {
    public:
    // void setSpa(balboaGL* spa) {
    //     this->spa = spa;
    // }
    void update() override {
        publish_state(status.state.c_str());
    }
    // private:
    // balboaGL* spa;
        
};

