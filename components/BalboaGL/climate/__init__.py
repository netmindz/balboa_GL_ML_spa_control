import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.components.logger import HARDWARE_UART_TO_SERIAL
from esphome.const import (
    CONF_ID,
    CONF_UPDATE_INTERVAL,
    CONF_MODE,
    CONF_FAN_MODE,
    CONF_SWING_MODE,
)
from esphome.core import CORE
from .. import balboagl_ns, CONF_BALBOA_ID, BalboaGL

AUTO_LOAD = ["climate"]

DEPENDENCIES = ["BalboaGL"]


CONF_SUPPORTS = "supports"
DEFAULT_CLIMATE_MODES = ["HEAT","AUTO"]
DEFAULT_FAN_MODES = ["OFF"]
DEFAULT_SWING_MODES = ["OFF"]

BalboaGLClimate = balboagl_ns.class_(
    "BalboaGLClimate", climate.Climate, cg.PollingComponent
)


CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(BalboaGLClimate),
        cv.GenerateID(CONF_BALBOA_ID): cv.use_id(BalboaGL),
        # If polling interval is greater than 9 seconds, the HeatPump library
        # reconnects, but doesn't then follow up with our data request.
       cv.Optional(CONF_UPDATE_INTERVAL, default="100ms"): cv.All(
           cv.update_interval, cv.Range(max=cv.TimePeriod(milliseconds=9000))
       ),
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


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

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



    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    paren = await cg.get_variable(config[CONF_BALBOA_ID])
    # cg.add(var.set_balboa_parent(paren))