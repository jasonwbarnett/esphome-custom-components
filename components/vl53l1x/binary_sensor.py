import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import DEVICE_CLASS_PRESENCE

from . import VL53L1XComponent, CONF_VL53L1X_ID

DEPENDENCIES = ["vl53l1x"]
CODEOWNERS = ["@mrtoy-me", "@jasonwbarnett"]

CONF_RANGE_VALID = "range_valid"
CONF_ABOVE_THRESHOLD = "above_threshold"
CONF_BELOW_THRESHOLD = "below_threshold"
CONF_ABOVE_DISTANCE = "above_distance"
CONF_BELOW_DISTANCE = "below_distance"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_VL53L1X_ID): cv.use_id(VL53L1XComponent),
        cv.Optional(CONF_RANGE_VALID): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_ABOVE_THRESHOLD): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PRESENCE,
        ).extend(
            {
                # distance in mm; sensor is "on" when measured distance > this value
                cv.Required(CONF_ABOVE_DISTANCE): cv.positive_int,
            }
        ),
        cv.Optional(CONF_BELOW_THRESHOLD): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PRESENCE,
        ).extend(
            {
                # distance in mm; sensor is "on" when measured distance < this value
                cv.Required(CONF_BELOW_DISTANCE): cv.positive_int,
            }
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_VL53L1X_ID])

    if conf := config.get(CONF_RANGE_VALID):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(hub.set_range_valid_binary_sensor(sens))

    if conf := config.get(CONF_ABOVE_THRESHOLD):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(hub.set_above_threshold_binary_sensor(sens))
        cg.add(hub.set_above_distance(conf[CONF_ABOVE_DISTANCE]))

    if conf := config.get(CONF_BELOW_THRESHOLD):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(hub.set_below_threshold_binary_sensor(sens))
        cg.add(hub.set_below_distance(conf[CONF_BELOW_DISTANCE]))
