import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import onx_app_state, onx_storage
from esphome.const import CONF_ID

CONF_APP_STATE_ID = "app_state_id"
CONF_STORAGE_ID = "storage_id"

onx_ui_ns = cg.esphome_ns.namespace("onx_ui")
OnxUi = onx_ui_ns.class_("OnxUi", cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(OnxUi),
    cv.Required(CONF_APP_STATE_ID): cv.use_id(onx_app_state.OnxAppState),
    cv.Required(CONF_STORAGE_ID): cv.use_id(onx_storage.OnxStorage),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    app_state = await cg.get_variable(config[CONF_APP_STATE_ID])
    cg.add(var.set_app_state(app_state))

    storage = await cg.get_variable(config[CONF_STORAGE_ID])
    cg.add(var.set_storage(storage))
