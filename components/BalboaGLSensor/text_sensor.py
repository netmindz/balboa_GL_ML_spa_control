import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID

balboa_sensor_ns = cg.esphome_ns.namespace('balboa_sensor')
StateInfo = balboa_sensor_ns.class_('BalboaGLStateSensor', text_sensor.TextSensor, cg.Component)
RawInfo = balboa_sensor_ns.class_('BalboaGLRawSensor', text_sensor.TextSensor, cg.Component)

# CONFIG_SCHEMA = text_sensor.TEXT_SENSOR_SCHEMA.extend({
#     cv.GenerateID(): cv.declare_id(BalboaGLSensor)
# }).extend(cv.COMPONENT_SCHEMA)


CONFIG_SCHEMA = cv.Schema({
    cv.Optional("state"): text_sensor.TEXT_SENSOR_SCHEMA.extend({
        cv.GenerateID(): cv.declare_id(StateInfo),
    }),
    cv.Optional("raw"): text_sensor.TEXT_SENSOR_SCHEMA.extend({
        cv.GenerateID(): cv.declare_id(RawInfo),
    }),
})

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield text_sensor.register_text_sensor(var, config)
    yield cg.register_component(var, config)
    
