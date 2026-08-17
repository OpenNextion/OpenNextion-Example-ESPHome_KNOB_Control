import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import logger, onx_app_state
from esphome.const import CONF_ID

CONF_APP_STATE_ID = "app_state_id"
CONF_NORMAL_WIFI_SSID = "normal_wifi_ssid"
CONF_NORMAL_WIFI_PASSWORD = "normal_wifi_password"
CONF_RUNTIME_LOG_LEVEL = "runtime_log_level"

onx_mode_manager_ns = cg.esphome_ns.namespace("onx_mode_manager")
OnxModeManager = onx_mode_manager_ns.class_("OnxModeManager", cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(OnxModeManager),
    cv.Required(CONF_APP_STATE_ID): cv.use_id(onx_app_state.OnxAppState),
    cv.Optional(CONF_NORMAL_WIFI_SSID, default=""): cv.string,
    cv.Optional(CONF_NORMAL_WIFI_PASSWORD, default=""): cv.string,
    cv.Required(CONF_RUNTIME_LOG_LEVEL): cv.one_of(*logger.LOG_LEVELS, upper=True),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    app_state = await cg.get_variable(config[CONF_APP_STATE_ID])
    cg.add(var.set_app_state(app_state))

    cg.add(var.set_normal_wifi_ssid(config[CONF_NORMAL_WIFI_SSID]))
    cg.add(var.set_normal_wifi_password(config[CONF_NORMAL_WIFI_PASSWORD]))
    cg.add(var.set_runtime_log_level(logger.LOG_LEVELS[config[CONF_RUNTIME_LOG_LEVEL]]))
