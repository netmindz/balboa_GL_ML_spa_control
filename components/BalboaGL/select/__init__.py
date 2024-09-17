import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import (
    CONF_ID,
    CONF_OPTIONS,
)
from .. import balboagl_ns, CONF_BALBOA_ID, BalboaGL



CONF_PUMP1 = "pump1"
CONF_PUMP2 = "pump2"

select_ns = cg.esphome_ns.namespace('balboa_select')
Pump1Select = select_ns.class_('BalboaGLPump1Select', select.Select, cg.Component)
Pump2Select = select_ns.class_('BalboaGLPump2Select', select.Select, cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_BALBOA_ID): cv.use_id(BalboaGL),
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

async def setup_conf(config, key):
    if key in config:
        conf = config[key]
        options_map = conf[CONF_OPTIONS]
        var = await select.new_select(conf, options=["off","high"])
        await cg.register_component(var, conf)
        paren = await cg.get_variable(config[CONF_BALBOA_ID])
        cg.add(cg.RawStatement(f"{var}->set_spa({paren}->get_spa());"))

async def to_code(config):
    await setup_conf(config, CONF_PUMP1)
    await setup_conf(config, CONF_PUMP2)
