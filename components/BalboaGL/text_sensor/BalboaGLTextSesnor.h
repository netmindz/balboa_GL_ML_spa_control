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
        if(status.state != this->last_value) {
            this->last_value = status.state;
            publish_state(last_value.c_str());
        }
    }
    std::string unique_id() override { return "state"; }
    private:
      String last_value;
        
};

class BalboaGLRawSensor : public PollingComponent, public text_sensor::TextSensor {
    public:
    // void setSpa(balboaGL* spa) {
    //     this->spa = spa;
    // }
    void update() override {
        if(status.rawData != this->last_value) {
            this->last_value = status.rawData;
            publish_state(last_value.c_str());
        }
    }
    std::string unique_id() override { return "raw"; }
    private:
      String last_value;
        
};

class BalboaGLLCDSensor : public PollingComponent, public text_sensor::TextSensor {
    public:
    // void setSpa(balboaGL* spa) {
    //     this->spa = spa;
    // }
    void update() override {
        if(status.lcd != this->last_value) {
            this->last_value = status.lcd;
            publish_state(last_value.c_str());
        }
    }
    std::string unique_id() override { return "lcd"; }
    private:
      char last_value[5];
        
};

