#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"

#include "balboaGL.h"

static const char* SWITCH_TAG = "BalboaGLSwitch"; // Logging tag

using namespace esphome;

    class BalboaGLLightSwitch : public Component, public switch_::Switch {
            public:
            void set_spa(balboaGL* spa) {
                this->spa = spa;
            }
            void write_state(bool state) override {
                if(state) {
                    ESP_LOGI(SWITCH_TAG, "LightSwitch write_state(true)");
                }
                else {
                    ESP_LOGI(SWITCH_TAG, "LightSwitch write_state(false)");
                }
                // This will be called every time the user requests a state change.

                spa->setLight(state);

                // Acknowledge new state by publishing it
                // publish_state(state);
                ESP_LOGD(SWITCH_TAG, "LightSwitch write_state complete");
            }
            void loop() override {
                if(status.light != this->last_state) {
                    this->last_state = status.light;
                    publish_state(this->last_state);
                }
            }

        private:
            balboaGL* spa;
            bool last_state;
        };