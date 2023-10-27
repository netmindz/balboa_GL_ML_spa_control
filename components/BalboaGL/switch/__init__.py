import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID

from .. import balboa_ns, CONF_BALBOA_ID, BalboaGL

DEPENDENCIES = ["BalboaGL"]

LightSwitch = cg.esphome_ns.class_('LightSwitch', switch.Switch, cg.Component)

CONFIG_SCHEMA = switch.SWITCH_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(LightSwitch),
        cv.GenerateID(CONF_BALBOA_ID): cv.use_id(BalboaGL),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)

    paren = await cg.get_variable(config[CONF_BALBOA_ID])
#    cg.add(var.set_balboa_parent(paren))
#    cg.add(cg.RawExpression("balboaglclimate->set_spa(balboagl->get_spa())"))

