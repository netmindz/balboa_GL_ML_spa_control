import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import (
    CONF_ID,
    CONF_OPTIONS,
)


CONF_PUMP1 = "pump1"
CONF_PUMP2 = "pump2"

select_ns = cg.esphome_ns.namespace('balboa_select')
Pump1Select = select_ns.class_('BalboaGLPump1Select', select.Select, cg.Component)
Pump2Select = select_ns.class_('BalboaGLPump2Select', select.Select, cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.Optional(CONF_PUMP1): select.SELECT_SCHEMA.extend({
        cv.GenerateID(): cv.declare_id(Pump1Select),
        cv.Required(CONF_OPTIONS): cv.All(
                cv.ensure_list(cv.string_strict), cv.Length(min=1)
        ),
    }),
    cv.Optional(CONF_PUMP2): select.SELECT_SCHEMA.extend({
        cv.GenerateID(): cv.declare_id(Pump2Select),
        cv.Required(CONF_OPTIONS): cv.All(
                cv.ensure_list(cv.string_strict), cv.Length(min=1)
        ),
    }),
  
})

def setup_conf(config, key):
    if key in config:
        conf = config[key]
        var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(var, conf)
        await select.register_select(var, config, options=list())

def to_code(config):
    await setup_conf(config, CONF_PUMP1)
    await setup_conf(config, CONF_PUMP2)
        # cg.add(cg.RawExpression("balboaglclimate->set_spa(balboagl->get_spa())"))

