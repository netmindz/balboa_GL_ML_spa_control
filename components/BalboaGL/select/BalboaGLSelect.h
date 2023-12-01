#pragma once

#include "esphome/core/component.h"
#include "esphome/components/select/select.h"

#include "balboaGL.h"

namespace esphome {
    namespace balboa_select {

    class BalboaGLPump1Select : public Component, public select::Select {
            public:
            void set_spa(balboaGL* spa) {
                this->spa = spa;
            }
            void control(const std::string &value) override {

            }
            private:
            balboaGL* spa;

            
        };

    class BalboaGLPump2Select : public Component, public select::Select {
            public:
            void set_spa(balboaGL* spa) {
                this->spa = spa;
            }
            void control(const std::string &value) override {
                
            }
            private:
            balboaGL* spa;
            
        };

    }
}