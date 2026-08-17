import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

onx_storage_ns = cg.esphome_ns.namespace("onx_storage")
OnxStorage = onx_storage_ns.class_("OnxStorage", cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(OnxStorage),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
