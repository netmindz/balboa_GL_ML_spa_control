import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components.logger import HARDWARE_UART_TO_SERIAL
from esphome.const import (
    CONF_ID,
    CONF_HARDWARE_UART,
    CONF_UPDATE_INTERVAL,
    CONF_RX_PIN,
    CONF_TX_PIN,
    PLATFORM_ESP32,
)
from esphome.core import CORE, coroutine

AUTO_LOAD = ["climate","switch","select","sensor","text_sensor"]

CODEOWNERS = ["@netmindz"]

CONF_SUPPORTS = "supports"

CONF_ENABLE_PIN = "enable_pin"
CONF_PANEL_SELECT_PIN = "panel_select_pin"

CONF_DELAY_TIME = "delay_time"

balboagl_ns = cg.esphome_ns.namespace("balboagl")
BalboaGL = cg.global_ns.class_(
    "BalboaGL", cg.Component
)


def valid_uart(uart):
    if CORE.is_esp32:
        uarts = [ "UART1", "UART2"]
    else:
        raise NotImplementedError

    return cv.one_of(*uarts, upper=True)(uart)


CONF_BALBOA_ID = "balboa_id"

CONFIG_SCHEMA = (
    cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(BalboaGL),
        cv.Optional(CONF_HARDWARE_UART, default="UART1"): valid_uart,
        # If polling interval is greater than 9 seconds, the HeatPump library
        # reconnects, but doesn't then follow up with our data request.
    #    cv.Optional(CONF_UPDATE_INTERVAL, default="100ms"): cv.All(
    #        cv.update_interval, cv.Range(max=cv.TimePeriod(milliseconds=9000))
    #    ),
        cv.Required(CONF_RX_PIN): cv.int_range(),
        cv.Required(CONF_TX_PIN): cv.int_range(),
        cv.Required(CONF_PANEL_SELECT_PIN): cv.int_range(),
        cv.Optional(CONF_ENABLE_PIN): cv.int_range(),
        cv.Optional(CONF_DELAY_TIME): cv.int_range(),
        # Optionally override the supported ClimateTraits.
        cv.Optional(CONF_SUPPORTS, default={}): cv.Schema(
            {
            }
        ),
    })
).extend(cv.COMPONENT_SCHEMA)


@coroutine
def to_code(config):
    serial = cg.global_ns.Serial1
    # HARDWARE_UART_TO_SERIAL[PLATFORM_ESP32][config[CONF_HARDWARE_UART]]
    var = cg.new_Pvariable(config[CONF_ID], cg.RawExpression(f"&{serial}"))

    cg.add_define("tubUART 1") # TODO make dynamic

    if CONF_RX_PIN in config:
        cg.add(var.set_rx_pin(config[CONF_RX_PIN]))

    if CONF_TX_PIN in config:
        cg.add(var.set_tx_pin(config[CONF_TX_PIN]))

    if CONF_ENABLE_PIN in config:
        cg.add(var.set_rts_pin(config[CONF_ENABLE_PIN]))

    if CONF_PANEL_SELECT_PIN in config:
        cg.add(var.set_panel_select_pin(config[CONF_PANEL_SELECT_PIN]))

    if CONF_DELAY_TIME in config:
        cg.add(var.set_delay_time(config[CONF_DELAY_TIME]))


    yield cg.register_component(var, config)

    cg.add_library(
        name="ArduinoQueue",
        repository="https://github.com/EinarArnason/ArduinoQueue.git",
        version="1.2.5",
    )
    
    cg.add_library(
        name="CircularBuffer", # TODO: should really pull in a dep of balboaGL
        repository="https://github.com/rlogiacco/CircularBuffer.git",
        version="1.3.3",
    )
    cg.add_library(
        name="balboaGL",
        repository="https://github.com/netmindz/balboaGL.git",
        version="af714231217e5f35be7c4bb77a3e417a2bcbff49",
    )
