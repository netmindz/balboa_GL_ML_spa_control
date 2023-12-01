import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import (
    CONF_ID,
    ENTITY_CATEGORY_DIAGNOSTIC
)

from .. import balboagl_ns, CONF_BALBOA_ID, BalboaGL

AUTO_LOAD = ["text_sensor"]

DEPENDENCIES = ["BalboaGL"]

CONF_STATE = "state"
CONF_RAW = "raw"
CONF_LCD = "lcd"


StateInfo = cg.esphome_ns.class_('BalboaGLStateSensor', text_sensor.TextSensor, cg.PollingComponent)
RawInfo =  cg.esphome_ns.class_('BalboaGLRawSensor', text_sensor.TextSensor, cg.PollingComponent)
LCDInfo =  cg.esphome_ns.class_('BalboaGLLCDSensor', text_sensor.TextSensor, cg.PollingComponent)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_STATE): text_sensor.text_sensor_schema(StateInfo,
        ).extend(cv.polling_component_schema("1s")),
        
        cv.Optional(CONF_RAW): text_sensor.text_sensor_schema(RawInfo,
                                                              entity_category=ENTITY_CATEGORY_DIAGNOSTIC                    
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
    await setup_conf(config, CONF_RAW)
    await setup_conf(config, CONF_LCD)
