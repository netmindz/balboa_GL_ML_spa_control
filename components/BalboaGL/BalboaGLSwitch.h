#pragma once


#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"

namespace esphome {
    namespace balboa_switch {

    class BalboaGLLightSwitch : public Component, public switch_::Switch {
            public:
            void setup() override {
                // This will be called by App.setup()
                // pinMode(5, INPUT);
            }
            void write_state(bool state) override {
                // This will be called every time the user requests a state change.

                // digitalWrite(5, state);

                // Acknowledge new state by publishing it
                // publish_state(state);
            }
        };

    }
}