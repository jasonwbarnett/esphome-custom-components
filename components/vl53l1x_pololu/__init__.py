import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

CODEOWNERS = ["@jasonwbarnett"]
DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["sensor"]

CONF_VL53L1X_POLOLU_ID = "vl53l1x_pololu_id"

vl53l1x_pololu_ns = cg.esphome_ns.namespace("vl53l1x_pololu")
VL53L1XPololuComponent = vl53l1x_pololu_ns.class_(
    "VL53L1XPololuComponent", cg.PollingComponent, i2c.I2CDevice
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(VL53L1XPololuComponent),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x29))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
