#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"
#include "vl53l1x_core.h"

namespace esphome {
namespace vl53l1x_pololu {

// Thin ESPHome wrapper around the vendored Pololu VL53L1X driver. Exists to A/B
// the Pololu continuous-ranging sequence against our own driver on the same
// hardware. Drives the sensor in CONTINUOUS (autonomous) mode.
class VL53L1XPololuComponent : public PollingComponent, public i2c::I2CDevice {
 public:
  void set_distance_sensor(sensor::Sensor *s) { distance_sensor_ = s; }
  void set_frames_sensor(sensor::Sensor *s) { frames_sensor_ = s; }

  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  VL53L1X sensor_;
  sensor::Sensor *distance_sensor_{nullptr};
  sensor::Sensor *frames_sensor_{nullptr};
  uint16_t last_distance_{0};
  uint32_t frame_count_{0};
  uint32_t last_log_ms_{0};
  bool ok_{false};
};

}  // namespace vl53l1x_pololu
}  // namespace esphome
