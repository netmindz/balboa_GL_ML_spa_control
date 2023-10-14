import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.components.logger import HARDWARE_UART_TO_SERIAL
from esphome.const import (
    CONF_ID,
    CONF_HARDWARE_UART,
    CONF_UPDATE_INTERVAL,
    CONF_MODE,
    CONF_FAN_MODE,
    CONF_SWING_MODE,
    CONF_RX_PIN,
    CONF_TX_PIN,
)
from esphome.core import CORE, coroutine

AUTO_LOAD = ["climate"]

CONF_SUPPORTS = "supports"
DEFAULT_CLIMATE_MODES = ["HEAT","AUTO"]
DEFAULT_FAN_MODES = ["OFF"]
DEFAULT_SWING_MODES = ["OFF"]

CONF_ENABLE_PIN = "enable_pin"
CONF_PANEL_SELECT_PIN = "panel_select_pin"

BalboaGL = cg.global_ns.class_(
    "BalboaGL", climate.Climate, cg.PollingComponent
)


def valid_uart(uart):
    if CORE.is_esp32:
        uarts = [ "UART1", "UART2"]
    else:
        raise NotImplementedError

    return cv.one_of(*uarts, upper=True)(uart)


CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(BalboaGL),
        cv.Optional(CONF_HARDWARE_UART, default="UART1"): valid_uart,
        # If polling interval is greater than 9 seconds, the HeatPump library
        # reconnects, but doesn't then follow up with our data request.
       cv.Optional(CONF_UPDATE_INTERVAL, default="100ms"): cv.All(
           cv.update_interval, cv.Range(max=cv.TimePeriod(milliseconds=9000))
       ),
        cv.Required(CONF_RX_PIN): cv.int_range(),
        cv.Required(CONF_TX_PIN): cv.int_range(),
        cv.Required(CONF_PANEL_SELECT_PIN): cv.int_range(),
        cv.Optional(CONF_ENABLE_PIN): cv.int_range(),
        # Optionally override the supported ClimateTraits.
        cv.Optional(CONF_SUPPORTS, default={}): cv.Schema(
            {
                cv.Optional(CONF_MODE, default=DEFAULT_CLIMATE_MODES):
                    cv.ensure_list(climate.validate_climate_mode),
                cv.Optional(CONF_FAN_MODE, default=DEFAULT_FAN_MODES):
                    cv.ensure_list(climate.validate_climate_fan_mode),
                cv.Optional(CONF_SWING_MODE, default=DEFAULT_SWING_MODES):
                    cv.ensure_list(climate.validate_climate_swing_mode),
            }
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


@coroutine
def to_code(config):
    serial = HARDWARE_UART_TO_SERIAL[config[CONF_HARDWARE_UART]]
    var = cg.new_Pvariable(config[CONF_ID], cg.RawExpression(f"&{serial}"))

    supports = config[CONF_SUPPORTS]
    traits = var.config_traits()

    for mode in supports[CONF_MODE]:
        if mode == "OFF":
            continue
        cg.add(traits.add_supported_mode(climate.CLIMATE_MODES[mode]))

    for mode in supports[CONF_FAN_MODE]:
        cg.add(traits.add_supported_fan_mode(climate.CLIMATE_FAN_MODES[mode]))

    for mode in supports[CONF_SWING_MODE]:
        cg.add(traits.add_supported_swing_mode(
            climate.CLIMATE_SWING_MODES[mode]
        ))

    yield cg.register_component(var, config)
    yield climate.register_climate(var, config)
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
        version="1c90d08f819ba2f55a5cc4bd36c82cdf846e4c7e",
    )
