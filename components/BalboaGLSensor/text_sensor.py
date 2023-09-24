import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID
from esphome.core import coroutine

CONF_STATE = "state"
CONF_RAW = "raw"


balboa_sensor_ns = cg.esphome_ns.namespace('balboa_sensor')
StateInfo = balboa_sensor_ns.class_('BalboaGLStateSensor', text_sensor.TextSensor, cg.Component)
RawInfo = balboa_sensor_ns.class_('BalboaGLRawSensor', text_sensor.TextSensor, cg.Component)

# CONFIG_SCHEMA = text_sensor.TEXT_SENSOR_SCHEMA.extend({
#     cv.GenerateID(): cv.declare_id(BalboaGLSensor)
# }).extend(cv.COMPONENT_SCHEMA)


CONFIG_SCHEMA = cv.Schema({
    cv.Optional(CONF_STATE): text_sensor.TEXT_SENSOR_SCHEMA.extend({
        cv.GenerateID(): cv.declare_id(StateInfo),
    }),
    cv.Optional(CONF_RAW): text_sensor.TEXT_SENSOR_SCHEMA.extend({
        cv.GenerateID(): cv.declare_id(RawInfo),
    }),
})

@coroutine
def setup_conf(config, key):
    if key in config:
        conf = config[key]
        var = cg.new_Pvariable(conf[CONF_ID])
        yield cg.register_component(var, conf)
        yield text_sensor.register_text_sensor(var, conf)
        
def to_code(config):
    yield setup_conf(config, CONF_STATE)
    yield setup_conf(config, CONF_RAW)
    
