#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"

#include "balboaGL.h"

using namespace esphome;

    class BalboaGLLightSwitch : public Component, public switch_::Switch {
            public:
            void set_spa(balboaGL* spa) {
                this->spa = spa;
            }
            void write_state(bool state) override {
                if(state) {
                    ESP_LOGI(TAG, "BalboaGLLightSwitch write_state(true)");
                }
                else {
                    ESP_LOGI(TAG, "BalboaGLLightSwitch write_state(false)");
                }
                // This will be called every time the user requests a state change.

                spa->setLight(state);

                // Acknowledge new state by publishing it
                // publish_state(state);
                ESP_LOGD(TAG, "BalboaGLLightSwitch write_state complete");
            }
            private:
            balboaGL* spa;
        };