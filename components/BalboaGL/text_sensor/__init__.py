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
RawInfo = balboagl_ns.class_('BalboaGLRawSensor', text_sensor.Sensor, cg.Component)
LCDInfo = balboagl_ns.class_('BalboaGLLCDSensor', text_sensor.Sensor, cg.Component)

CONFIG_SCHEMA = (
    text_sensor.text_sensor_schema(StateInfo)
    .extend(
        {
            cv.GenerateID(CONF_BALBOA_ID): cv.use_id(BalboaGL),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_STATE): text_sensor.text_sensor_schema(StateInfo,
        ).extend(cv.polling_component_schema("1s")),
        
        cv.Optional(CONF_RAW): text_sensor.text_sensor_schema(RawInfo,
        ).extend(cv.polling_component_schema("1s")),

        cv.Optional(CONF_LCD): text_sensor.text_sensor_schema(LCDInfo,
        ).extend(cv.polling_component_schema("1s")),
    }
)

async def setup_conf(config, key):
    if key in config:
        conf = config[key]
        var = await text_sensor.new_text_sensor(conf)
        await cg.register_component(var, conf)

async def to_code(config):
    await setup_conf(config, CONF_STATE)
