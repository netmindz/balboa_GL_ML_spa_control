#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"

#include "balboaGL.h"

namespace esphome {
    namespace balboa_switch {

    class BalboaGLLightSwitch : public Component, public switch_::Switch {
            public:
            void setSpa(balboaGL* spa) {
                this->spa = spa;
            }
            void write_state(bool state) override {
                // This will be called every time the user requests a state change.

                spa->setLight(state);

                // Acknowledge new state by publishing it
                // publish_state(state);
            }
            private:
            balboaGL* spa;
        };

    }
}