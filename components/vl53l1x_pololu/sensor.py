import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_DISTANCE,
    DEVICE_CLASS_DISTANCE,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import VL53L1XPololuComponent, CONF_VL53L1X_POLOLU_ID

DEPENDENCIES = ["vl53l1x_pololu"]

CONF_FRAMES = "frames"
CONF_RECOVERY_COUNT = "recovery_count"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_VL53L1X_POLOLU_ID): cv.use_id(VL53L1XPololuComponent),
        cv.Required(CONF_DISTANCE): sensor.sensor_schema(
            unit_of_measurement="mm",
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_DISTANCE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        # Total ranging frames consumed since boot. Climbs steadily while continuous
        # ranging sustains; a stall would freeze it. Diagnostic.
        cv.Optional(CONF_FRAMES): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        # Total in-place freeze recoveries since boot. Flat = healthy.
        cv.Optional(CONF_RECOVERY_COUNT): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_VL53L1X_POLOLU_ID])
    sens = await sensor.new_sensor(config[CONF_DISTANCE])
    cg.add(hub.set_distance_sensor(sens))
    if frames_config := config.get(CONF_FRAMES):
        cg.add(hub.set_frames_sensor(await sensor.new_sensor(frames_config)))
    if recovery_config := config.get(CONF_RECOVERY_COUNT):
        cg.add(hub.set_recovery_count_sensor(await sensor.new_sensor(recovery_config)))
