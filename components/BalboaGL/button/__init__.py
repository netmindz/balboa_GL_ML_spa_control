import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import (
    ICON_LIGHTBULB,
)
from .. import balboagl_ns, CONF_BALBOA_ID, BalboaGL

AUTO_LOAD = ["button"]

DEPENDENCIES = ["BalboaGL"]

UpButton = cg.esphome_ns.class_("UpButton", button.Button, cg.PollingComponent)
DownButton = cg.esphome_ns.class_("DownButton", button.Button, cg.PollingComponent)
ModeButton = cg.esphome_ns.class_("ModeButton", button.Button, cg.PollingComponent)

CONF_UP = "up"
CONF_DOWN = "down"
CONF_MODE = "mode"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_BALBOA_ID): cv.use_id(BalboaGL),

        cv.Optional(CONF_UP): button.button_schema(UpButton,
                                                                  
        ),
        cv.Optional(CONF_DOWN): button.button_schema(DownButton,
                                                                  
        ),
        cv.Optional(CONF_MODE): button.button_schema(ModeButton,
                                                                  
        ),
    }
)

async def setup_conf(config, key):
    if key in config:
        conf = config[key]
        var = await button.new_button(conf)
        await cg.register_component(var, conf)
        paren = await cg.get_variable(config[CONF_BALBOA_ID])
        cg.add(cg.RawStatement(f"{var}->set_spa({paren}->get_spa());"))

async def to_code(config):
    await setup_conf(config, CONF_UP)
    await setup_conf(config, CONF_DOWN)
    await setup_conf(config, CONF_MODE)