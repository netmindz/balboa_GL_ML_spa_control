import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import (
)
from .. import balboagl_ns, CONF_BALBOA_ID, BalboaGL

AUTO_LOAD = ["climate"]

DEPENDENCIES = ["BalboaGL"]

UpButton = cg.esphome_ns.class_("UpButton", button.Button)
DownButton = cg.esphome_ns.class_("DownButton", button.Button)
ModeButton = cg.esphome_ns.class_("ModeButton", button.Button)

CONF_UP = "up"
CONF_DOWN = "down"
CONF_MODE = "mode"

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_BALBOA_ID): cv.use_id(BalboaGL),
    cv.Optional(CONF_UP): button.button_schema(
        UpButton,
        # icon=ICON_RESTART_ALERT,
    ),
    cv.Optional(CONF_DOWN): button.button_schema(
        DownButton,
        # device_class=DEVICE_CLASS_RESTART,
        # entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        # icon=ICON_RESTART,
    ),
    cv.Optional(CONF_MODE): button.button_schema(
        ModeButton,
        # entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        # icon=ICON_DATABASE,
    ),
}


async def setup_conf(config, key):
    if key in config:
        conf = config[key]
        var = await button.new_button(conf)
        await cg.register_component(var, conf)

async def to_code(config):
    await setup_conf(config, CONF_UP)
    await setup_conf(config, CONF_DOWN)
    await setup_conf(config, CONF_MODE)