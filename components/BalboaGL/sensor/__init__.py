import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID
from esphome.core import coroutine

from .. import balboagl_ns, CONF_BALBOA_ID, BalboaGL

AUTO_LOAD = ["text_sensor"]

DEPENDENCIES = ["BalboaGL"]

CONF_QUEUE = "commandQueue"
CONF_STATE = "state"
CONF_RAW = "raw"
CONF_LCD = "lcd"


CommandQueueInfo = balboagl_ns.class_(
    "BalboaGLCommandQueueSensor", sensor.TextSensor, cg.PollingComponent
)

CommandQueueInfo = cg.esphome_ns.class_('BalboaGLCommandQueueSensor', sensor.Sensor, cg.Component)
# RawInfo = balboa_sensor_ns.class_('BalboaGLRawSensor', text_sensor.TextSensor, cg.Component)
# LCDInfo = balboa_sensor_ns.class_('BalboaGLLCDSensor', text_sensor.TextSensor, cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_QUEUE): sensor.sensor_schema(
            CommandQueueInfo, 
        ).extend(cv.polling_component_schema("1s")),
        
    # cv.Optional(CONF_RAW): text_sensor.TEXT_SENSOR_SCHEMA.extend({
    #     cv.GenerateID(): cv.declare_id(RawInfo),
    # }),
    # cv.Optional(CONF_LCD): text_sensor.TEXT_SENSOR_SCHEMA.extend({
    #     cv.GenerateID(): cv.declare_id(LCDInfo),
    # }),
    }
)

@coroutine
def setup_conf(config, key):
    if key in config:
        conf = config[key]
        var = cg.new_Pvariable(conf[CONF_ID])
        yield cg.register_component(var, conf)
        yield sensor.register_sensor(var, conf)

def to_code(config):
    yield setup_conf(config, CONF_STATE)
    # yield setup_conf(config, CONF_RAW)
    # yield setup_conf(config, CONF_LCD)
