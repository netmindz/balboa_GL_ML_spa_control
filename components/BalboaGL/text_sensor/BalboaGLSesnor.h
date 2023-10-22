#pragma once

#include "esphome.h"

#include "balboaGL.h"

namespace esphome {
    namespace balboa_sensor {

    class BalboaGLStateSensor : public Component, public TextSensor {
            public:
            void setSpa(balboaGL* spa) {
                this->spa = spa;
            }
            void update() override {
                publish_state(status.state.c_ctr());
            }
            private:
            balboaGL* spa;
            
        };

}