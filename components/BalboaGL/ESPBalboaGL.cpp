#include <string>
#include "ESPBalboaGL.h"
using namespace esphome;

void telnetSend(String msg) {
    ESP_LOGI(TAG, msg.c_str());
}
/**
 * Create a new BalboaGL object
 *
 * Args:
 *   hw_serial: pointer to an Arduino HardwareSerial instance
 *   poll_interval: polling interval in milliseconds
 */
BalboaGL::BalboaGL(
        HardwareSerial* hw_serial
) :
    hw_serial_{hw_serial}
{
    
    // this->rawSensor = new text_sensor::TextSensor();
    // this->stateSensor = new text_sensor::TextSensor();

    // this->lightSwitch = new balboa_switch::BalboaGLLightSwitch();

    // this->pump1 = new balboa_select::BalboaGLPump1Select();
    // this->pump2 = new balboa_select::BalboaGLPump12elect();
}

void BalboaGL::check_logger_conflict_() {
#ifdef USE_LOGGER
    if (this->get_hw_serial_() == logger::global_logger->get_hw_serial()) {
        ESP_LOGW(TAG, "  You're using the same serial port for logging"
                " and the BalboaGL component. Please disable"
                " logging over the serial port by setting"
                " logger:baud_rate to 0.");
    }
#endif
}

void BalboaGL::loop() {
    // This will be called every "update_interval" milliseconds.
    // ESP_LOGV(TAG, "Loop called.");
    size_t len = this->spa->readSerial();
    ESP_LOGV(TAG, "Read %u bytes", len);
    bool panelSelect = digitalRead(this->spa->getPanelSelectPin());
    if(panelSelect == HIGH) {
        ESP_LOGV(TAG, "PanelSelect == HIGH");
        return;
    }
    else {
        ESP_LOGD(TAG, "PanelSelect == LOW");
    }

    ESP_LOGD(TAG, "Current Temp: %f", status.temp);
    ESP_LOGD(TAG, "Target Temp: %f", status.targetTemp);
    // this->current_temperature = status.temp;
    // this->target_temperature = status.targetTemp;
    // switch(status.mode) {
    //     case MODE_IDX_STD:
    //         this->custom_preset = std::string("STD");
    //         break;
    //     case MODE_IDX_ECO:
    //         this->custom_preset = std::string("ECO");
    //         break;
    //     case MODE_IDX_SLP:
    //         this->custom_preset = std::string("Sleep");
    //         break;
    //     default:
    //         this->custom_preset = std::string("UNKNOWN");
    //         break;
    // }


    static String lastRaw = "0";
    if(status.rawData != lastRaw) {
        ESP_LOGD(TAG, "Raw: %s", status.rawData.c_str());
        lastRaw = status.rawData;

        // this->pump1->publish_state(pump_mode[status.pump1]);
        // this->pump2->publish_state(pump_mode[status.pump2]);

        // this->lightSwitch->publish_state(status.light);

        // this->stateSensor->publish_state("testing"); // status.state.c_str());
        // this->rawSensor->publish_state(status.rawData.c_str());
        // this->lcdSensor->update();
        
        // this->publish_state();
    }

}

// void BalboaGL::hpSettingsChanged() {
//     heatpumpSettings currentSettings = hp->getSettings();

//     if (currentSettings.power == NULL) {
//         /*
//          * We should always get a valid pointer here once the HeatPump
//          * component fully initializes. If HeatPump hasn't read the settings
//          * from the unit yet (hp->connect() doesn't do this, sadly), we'll need
//          * to punt on the update. Likely not an issue when run in callback
//          * mode, but that isn't working right yet.
//          */
//         ESP_LOGW(TAG, "Waiting for HeatPump to read the settings the first time.");
//         esphome::delay(10);
//         return;
//     }

//     /*
//      * ************ HANDLE POWER AND MODE CHANGES ***********
//      * https://github.com/geoffdavis/HeatPump/blob/stream/src/HeatPump.h#L125
//      * const char* POWER_MAP[2]       = {"OFF", "ON"};
//      * const char* MODE_MAP[5]        = {"HEAT", "DRY", "COOL", "FAN", "AUTO"};
//      */
//     if (strcmp(currentSettings.power, "ON") == 0) {
//         if (strcmp(currentSettings.mode, "HEAT") == 0) {
//             this->mode = climate::CLIMATE_MODE_HEAT;
//             if (heat_setpoint != currentSettings.temperature) {
//                 heat_setpoint = currentSettings.temperature;
//                 save(currentSettings.temperature, heat_storage);
//             }
//             this->action = climate::CLIMATE_ACTION_IDLE;
//         } else if (strcmp(currentSettings.mode, "DRY") == 0) {
//             this->mode = climate::CLIMATE_MODE_DRY;
//             this->action = climate::CLIMATE_ACTION_DRYING;
//         } else if (strcmp(currentSettings.mode, "COOL") == 0) {
//             this->mode = climate::CLIMATE_MODE_COOL;
//             if (cool_setpoint != currentSettings.temperature) {
//                 cool_setpoint = currentSettings.temperature;
//                 save(currentSettings.temperature, cool_storage);
//             }
//             this->action = climate::CLIMATE_ACTION_IDLE;
//         } else if (strcmp(currentSettings.mode, "FAN") == 0) {
//             this->mode = climate::CLIMATE_MODE_FAN_ONLY;
//             this->action = climate::CLIMATE_ACTION_FAN;
//         } else if (strcmp(currentSettings.mode, "AUTO") == 0) {
//             this->mode = climate::CLIMATE_MODE_HEAT_COOL;
//             if (auto_setpoint != currentSettings.temperature) {
//                 auto_setpoint = currentSettings.temperature;
//                 save(currentSettings.temperature, auto_storage);
//             }
//             this->action = climate::CLIMATE_ACTION_IDLE;
//         } else {
//             ESP_LOGW(
//                     TAG,
//                     "Unknown climate mode value %s received from HeatPump",
//                     currentSettings.mode
//             );
//         }
//     } else {
//         this->mode = climate::CLIMATE_MODE_OFF;
//         this->action = climate::CLIMATE_ACTION_OFF;
//     }

//     ESP_LOGI(TAG, "Climate mode is: %i", this->mode);

//     /*
//      * ******* HANDLE FAN CHANGES ********
//      *
//      * const char* FAN_MAP[6]         = {"AUTO", "QUIET", "1", "2", "3", "4"};
//      */
//     if (strcmp(currentSettings.fan, "QUIET") == 0) {
//         this->fan_mode = climate::CLIMATE_FAN_DIFFUSE;
//     } else if (strcmp(currentSettings.fan, "1") == 0) {
//             this->fan_mode = climate::CLIMATE_FAN_LOW;
//     } else if (strcmp(currentSettings.fan, "2") == 0) {
//             this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
//     } else if (strcmp(currentSettings.fan, "3") == 0) {
//             this->fan_mode = climate::CLIMATE_FAN_MIDDLE;
//     } else if (strcmp(currentSettings.fan, "4") == 0) {
//             this->fan_mode = climate::CLIMATE_FAN_HIGH;
//     } else { //case "AUTO" or default:
//         this->fan_mode = climate::CLIMATE_FAN_AUTO;
//     }
//     ESP_LOGI(TAG, "Fan mode is: %i", this->fan_mode);

//     /* ******** HANDLE MITSUBISHI VANE CHANGES ********
//      * const char* VANE_MAP[7]        = {"AUTO", "1", "2", "3", "4", "5", "SWING"};
//      */
//     if (strcmp(currentSettings.vane, "SWING") == 0) {
//         this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
//     }
//     else {
//         this->swing_mode = climate::CLIMATE_SWING_OFF;
//     }
//     ESP_LOGI(TAG, "Swing mode is: %i", this->swing_mode);



//     /*
//      * ******** HANDLE TARGET TEMPERATURE CHANGES ********
//      */
//     this->target_temperature = currentSettings.temperature;
//     ESP_LOGI(TAG, "Target temp is: %f", this->target_temperature);

//     /*
//      * ******** Publish state back to ESPHome. ********
//      */
//     this->publish_state();
// }

// /**
//  * Report changes in the current temperature sensed by the HeatPump.
//  */
// void BalboaGL::hpStatusChanged(heatpumpStatus currentStatus) {
//     this->current_temperature = currentStatus.roomTemperature;
//     switch (this->mode) {
//         case climate::CLIMATE_MODE_HEAT:
//             if (currentStatus.operating) {
//                 this->action = climate::CLIMATE_ACTION_HEATING;
//             }
//             else {
//                 this->action = climate::CLIMATE_ACTION_IDLE;
//             }
//             break;
//         case climate::CLIMATE_MODE_COOL:
//             if (currentStatus.operating) {
//                 this->action = climate::CLIMATE_ACTION_COOLING;
//             }
//             else {
//                 this->action = climate::CLIMATE_ACTION_IDLE;
//             }
//             break;
//         case climate::CLIMATE_MODE_HEAT_COOL:
//             this->action = climate::CLIMATE_ACTION_IDLE;
//             if (currentStatus.operating) {
//               if (this->current_temperature > this->target_temperature) {
//                   this->action = climate::CLIMATE_ACTION_COOLING;
//               } else if (this->current_temperature < this->target_temperature) {
//                   this->action = climate::CLIMATE_ACTION_HEATING;
//               }
//             }
//             break;
//         case climate::CLIMATE_MODE_DRY:
//             if (currentStatus.operating) {
//                 this->action = climate::CLIMATE_ACTION_DRYING;
//             }
//             else {
//                 this->action = climate::CLIMATE_ACTION_IDLE;
//             }
//             break;
//         case climate::CLIMATE_MODE_FAN_ONLY:
//             this->action = climate::CLIMATE_ACTION_FAN;
//             break;
//         default:
//             this->action = climate::CLIMATE_ACTION_OFF;
//     }

//     this->publish_state();
// }

// void BalboaGL::set_remote_temperature(float temp) {
//     ESP_LOGD(TAG, "Setting remote temp: %.1f", temp);
//     this->hp->setRemoteTemperature(temp);
// }

void BalboaGL::setup() {
    // This will be called by App.setup()
    // this->banner();
    ESP_LOGCONFIG(TAG, "Setting up UART...");
    if (!this->get_hw_serial_()) {
        ESP_LOGCONFIG(
                TAG,
                "No HardwareSerial was provided. "
                "Software serial ports are unsupported by this component."
        );
        this->mark_failed();
        return;
    }
    this->check_logger_conflict_();

    ESP_LOGCONFIG(TAG, "Initialize new balboaGL object.");

    ESP_LOGI(TAG, "Serial begin rx,tx = %u,%u", this->rx_pin, this->tx_pin);
    hw_serial_->begin(115200, SERIAL_8N1, rx_pin, tx_pin);
    this->spa = new balboaGL(hw_serial_, rts_pin, panel_select_pin);
    this->spa->attachPanelInterrupt(); 
    // this->current_temperature = NAN;
    // this->target_temperature = NAN;

    // this->lightSwitch->setSpa(spa);

    // this->pump1->setSpa(spa);
    // this->pump2->setSpa(spa);

//     ESP_LOGCONFIG(
//             TAG,
//             "hw_serial(%p) is &Serial(%p)? %s",
//             this->get_hw_serial_(),
//             &Serial,
//             YESNO(this->get_hw_serial_() == &Serial)
//     );

//     ESP_LOGCONFIG(TAG, "Calling hp->connect(%p)", this->get_hw_serial_());

//     if (hp->connect(this->get_hw_serial_(), this->baud_, -1, -1)) {
//         hp->sync();
//     }
//     else {
//         ESP_LOGCONFIG(
//                 TAG,
//                 "Connection to HeatPump failed."
//                 " Marking BalboaGL component as failed."
//         );
//         this->mark_failed();
//     }

//     // create various setpoint persistence:
//     cool_storage = global_preferences->make_preference<uint8_t>(this->get_object_id_hash() + 1);
//     heat_storage = global_preferences->make_preference<uint8_t>(this->get_object_id_hash() + 2);
//     auto_storage = global_preferences->make_preference<uint8_t>(this->get_object_id_hash() + 3);

//     // load values from storage:
//     cool_setpoint = load(cool_storage);
//     heat_setpoint = load(heat_storage);
//     auto_setpoint = load(auto_storage);

    ESP_LOGCONFIG(TAG, "End of seutp");
    this->dump_config();
}

void BalboaGL::pause() {
    ESP_LOGI(TAG, "pause");
    this->spa->detachPanelInterrupt();
}

// /**
//  * The ESP only has a few bytes of rtc storage, so instead
//  * of storing floats directly, we'll store the number of
//  * TEMPERATURE_STEPs from MIN_TEMPERATURE.
//  **/
// void BalboaGL::save(float value, ESPPreferenceObject& storage) {
//     uint8_t steps = (value - ESPMHP_MIN_TEMPERATURE) / ESPMHP_TEMPERATURE_STEP;
//     storage.save(&steps);
// }

// optional<float> BalboaGL::load(ESPPreferenceObject& storage) {
//     uint8_t steps = 0;
//     if (!storage.load(&steps)) {
//         return {};
//     }
//     return ESPMHP_MIN_TEMPERATURE + (steps * ESPMHP_TEMPERATURE_STEP);
// }

void BalboaGL::dump_config() {
    // this->banner();
    ESP_LOGI(TAG, " rx,tx = %u,%u", this->rx_pin, this->tx_pin);
    ESP_LOGI(TAG, " rts_pin = %u", this->rts_pin);
    ESP_LOGI(TAG, " panel_select_pin = %u", this->panel_select_pin);
//     ESP_LOGI(TAG, "  Supports HEAT: %s", YESNO(true));
//     ESP_LOGI(TAG, "  Supports COOL: %s", YESNO(true));
//     ESP_LOGI(TAG, "  Supports AWAY mode: %s", YESNO(false));
//     ESP_LOGI(TAG, "  Saved heat: %.1f", heat_setpoint.value_or(-1));
//     ESP_LOGI(TAG, "  Saved cool: %.1f", cool_setpoint.value_or(-1));
//     ESP_LOGI(TAG, "  Saved auto: %.1f", auto_setpoint.value_or(-1));
}

// void BalboaGL::dump_state() {
//     LOG_CLIMATE("", "BalboaGL Climate", this);
//     ESP_LOGI(TAG, "HELLO");
// }

void BalboaGL::set_rx_pin(int pin) {
    this->rx_pin = pin;
}

void BalboaGL::set_tx_pin(int pin) {
    this->tx_pin = pin;
}
void BalboaGL::set_rts_pin(int pin) {
    this->rts_pin = pin;
}

void BalboaGL::set_panel_select_pin(int pin) {
    this->panel_select_pin = pin;
}
