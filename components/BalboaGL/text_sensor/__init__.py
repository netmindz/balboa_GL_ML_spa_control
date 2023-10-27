import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID
from esphome.core import coroutine

from .. import balboagl_ns, CONF_BALBOA_ID, BalboaGL

AUTO_LOAD = ["text_sensor"]

DEPENDENCIES = ["BalboaGL"]

CONF_STATE = "state"
CONF_RAW = "raw"
CONF_LCD = "lcd"


StateInfo = cg.esphome_ns.class_('BalboaGLStateSensor', text_sensor.TextSensor, cg.Component)
# RawInfo = balboagl_ns.class_('BalboaGLRawSensor', sensor.Sensor, cg.Component)
# LCDInfo = balboagl_ns.class_('BalboaGLLCDSensor', sensor.Sensor, cg.Component)

CONFIG_SCHEMA = text_sensor.TEXT_SENSOR_SCHEMA.extend(
    {

    cv.GenerateID(): cv.declare_id(StateInfo),
    cv.GenerateID(CONF_BALBOA_ID): cv.use_id(BalboaGL),
        
    # cv.Optional(CONF_RAW): text_sensor.TEXT_SENSOR_SCHEMA.extend({
    #     cv.GenerateID(): cv.declare_id(RawInfo),
    # }),
    # cv.Optional(CONF_LCD): text_sensor.TEXT_SENSOR_SCHEMA.extend({
    #     cv.GenerateID(): cv.declare_id(LCDInfo),
    # }),
}).extend(cv.COMPONENT_SCHEMA)

@coroutine
def setup_conf(config, key):
    if key in config:
        conf = config[key]
        var = cg.new_Pvariable(conf[CONF_ID])
        yield cg.register_component(var, conf)
        yield text_sensor.register_text_sensor(var, conf)

def to_code(config):
    yield setup_conf(config, CONF_STATE)
    # yield setup_conf(config, CONF_RAW)
    # yield setup_conf(config, CONF_LCD)
