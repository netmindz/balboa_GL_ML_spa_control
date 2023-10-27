import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID
from esphome.core import coroutine

from .. import balboagl_ns, CONF_BALBOA_ID, BalboaGL

AUTO_LOAD = ["text_sensor"]

DEPENDENCIES = ["BalboaGL"]

CONF_QUEUE = "commandQueue"

CommandQueueInfo = balboagl_ns.class_('BalboaGLCommandQueueSensor', sensor.Sensor, cg.Component)

CONFIG_SCHEMA = (
    sensor.sensor_schema(CommandQueueInfo)
    .extend(
        {
            cv.GenerateID(CONF_BALBOA_ID): cv.use_id(BalboaGL),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_QUEUE): sensor.sensor_schema(CommandQueueInfo,
        ).extend(cv.polling_component_schema("1s")),
        
    # cv.Optional(CONF_RAW): text_sensor.TEXT_SENSOR_SCHEMA.extend({
    #     cv.GenerateID(): cv.declare_id(RawInfo),
    # }),
    # cv.Optional(CONF_LCD): text_sensor.TEXT_SENSOR_SCHEMA.extend({
    #     cv.GenerateID(): cv.declare_id(LCDInfo),
    # }),
    }
)

async def setup_conf(config, key):
    if key in config:
        conf = config[key]
        var = await sensor.new_sensor(conf)
        await cg.register_component(var, conf)

async def to_code(config):
    await setup_conf(config, CONF_QUEUE)
