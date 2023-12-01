#pragma once

#include "esphome.h"
#include "esphome/components/sensor/sensor.h"

// #include "balboaGL.h"

using namespace esphome;

class BalboaGLCommandQueueSensor : public PollingComponent, public sensor::Sensor {
    public:
    void update() override {
        if(status.commandQueue != this->last_value) {
            this->last_value = status.commandQueue;
            publish_state(status.commandQueue);
        }
    }
    private:
        u_int8_t last_value;        
};

