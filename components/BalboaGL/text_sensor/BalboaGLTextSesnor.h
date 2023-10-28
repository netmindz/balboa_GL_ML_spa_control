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
    std::string unique_id() override { return "state"; }
    // private:
    // balboaGL* spa;
        
};

class BalboaGLRawSensor : public PollingComponent, public text_sensor::TextSensor {
    public:
    // void setSpa(balboaGL* spa) {
    //     this->spa = spa;
    // }
    void update() override {
        publish_state(status.rawData.c_str());
    }
    std::string unique_id() override { return "raw"; }
    // private:
    // balboaGL* spa;
        
};

class BalboaGLLCDSensor : public PollingComponent, public text_sensor::TextSensor {
    public:
    // void setSpa(balboaGL* spa) {
    //     this->spa = spa;
    // }
    void update() override {
        publish_state(status.lcd);
    }
    std::string unique_id() override { return "lcd"; }
    // private:
    // balboaGL* spa;
        
};

