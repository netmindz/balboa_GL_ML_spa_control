#pragma once

#include "esphome.h"
#include "esphome/components/sensor/sensor.h"

// #include "balboaGL.h"

using namespace esphome;

class BalboaGLCommandQueueSensor : public PollingComponent, public sensor::Sensor {
    public:
    // void setSpa(balboaGL* spa) {
    //     this->spa = spa;
    // }
    void update() override {
        publish_state(status.commandQueue);
    }
    // private:
    // balboaGL* spa;
        
};

