import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID

select_ns = cg.esphome_ns.namespace('balboa_select')
Pump1Select = select_ns.class_('BalboaGLPump1Select', select.Select, cg.Component)

CONFIG_SCHEMA = select.SWITCH_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(Pump1Select)
}).extend(cv.COMPONENT_SCHEMA)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield select.tch.register_switch(var, config)
