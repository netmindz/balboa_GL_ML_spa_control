#include <string>
#include "ESPBalboaGL.h"
using namespace esphome;

void telnetSend(String msg) {
    ESP_LOGI(TAG, msg.c_str());
}

void log(const char *format, ...) {
    char loc_buf[64];
    char * temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
    va_end(copy);
    if(len < 0) {
        va_end(arg);
    };
    if(len >= sizeof(loc_buf)){
        temp = (char*) malloc(len+1);
        if(temp == NULL) {
            va_end(arg);
        }
        len = vsnprintf(temp, len+1, format, arg);
    }
    va_end(arg);
    std::string str = reinterpret_cast<char *>(temp);
    ESP_LOGI(TAG, str.c_str());
    if(temp != loc_buf){
        free(temp);
    }

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
    int sanity = 0;
    do {
        size_t len = this->spa->readSerial();
        // ESP_LOGV(TAG, "Read %u bytes", len);
        static String lastRaw = "0";
        if(status.rawData != lastRaw) {
            ESP_LOGD(TAG, "Raw: %s", status.rawData.c_str());
            lastRaw = status.rawData;
            // this->publish_state();
        }
        sanity++;
    }
    while((status.commandQueue > 0) && (sanity < 10));

}

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
    this->spa = new balboaGL(hw_serial_, rts_pin, panel_select_pin, ESP_LOG_VERBOSE);
    this->spa->attachPanelInterrupt();
    if(delay_time > -1) this->spa->set_delay_time(delay_time);

    this->high_freq_.start(); // no wait on main loop

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

void BalboaGL::set_delay_time(int delay) {
    this->delay_time;
}