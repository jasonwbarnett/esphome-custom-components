#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif

namespace esphome {
namespace vl53l1x {

enum DistanceMode {
  SHORT = 0,
  LONG,
};

enum RangingMode {
  ONE_SHOT = 0,  // host explicitly triggers each measurement (default; robust)
  CONTINUOUS,    // sensor auto-ranges using the inter-measurement period
};

class VL53L1XComponent : public PollingComponent, public i2c::I2CDevice, public sensor::Sensor {
 public:
  void set_distance_sensor(sensor::Sensor *distance_sensor) { distance_sensor_ = distance_sensor; }
  void set_range_status_sensor(sensor::Sensor *range_status_sensor) { range_status_sensor_ = range_status_sensor; }
  void set_recovery_count_sensor(sensor::Sensor *recovery_count_sensor) { recovery_count_sensor_ = recovery_count_sensor; }
  void config_distance_mode(DistanceMode distance_mode ) { distance_mode_ = distance_mode; }
  void config_ranging_mode(RangingMode ranging_mode) { ranging_mode_ = ranging_mode; }
  
#ifdef USE_BINARY_SENSOR
  void set_above_distance(long above_distance) { above_distance_ = above_distance; }
  void set_below_distance(long below_distance) { below_distance_ = below_distance; }
  SUB_BINARY_SENSOR(range_valid)
  SUB_BINARY_SENSOR(above_threshold)
  SUB_BINARY_SENSOR(below_threshold)
#endif

  void setup() override;
  void dump_config() override;
  void update() override;
  void loop() override;
  float get_setup_priority() const override;
 
 protected:
  DistanceMode distance_mode_;
  RangingMode ranging_mode_{ONE_SHOT};

  uint16_t distance_{0};

  enum RangeStatus {
    RANGE_VALID  = 0,
    SIGMA_FAIL_WARNING,
    SIGNAL_FAIL_WARNING,
    OUT_OF_BOUNDS_FAIL,
    WRAP_AROUND_FAIL,
    UNDEFINED,
  } range_status_{UNDEFINED};

  enum ErrorCode {
    NONE = 0,
    COMMUNICATION_FAILED,
    WRONG_CHIP_ID,
    DATAREADY_TIMEOUT,
    BOOT_TIMEOUT,
  } error_code_{NONE};

  // Full (re-callable) sensor initialisation. Returns false on any failure WITHOUT
  // marking the component failed, so it can be used both at setup and for recovery.
  bool init_sensor();
  // Self-healing: stop + re-initialise a stuck/unresponsive sensor in place.
  void recover();

  bool boot_state(uint8_t *state);
  bool clear_interrupt();
  bool get_interrupt_polarity(uint8_t *interrupt_polarity);
  bool start_ranging();
  bool start_oneshot_ranging();
  bool stop_ranging();
  bool check_for_dataready(bool *is_dataready);
  bool check_sensor_id();
  bool get_distance(uint16_t *distance);
  bool get_range_status();

  bool get_timing_budget(uint16_t *timing_budget_ms);
  bool set_timing_budget(uint16_t timing_budget_ms);
  bool get_distance_mode(DistanceMode *mode);
  bool set_distance_mode(DistanceMode distance_mode);
  
  bool set_intermeasurement_period(uint16_t intermeasurement_ms);
  bool get_intermeasurement_period(uint16_t *intermeasurement_ms);

  bool vl53l1x_write_bytes(uint16_t a_register, const uint8_t *data, uint8_t len);
  bool vl53l1x_write_byte(uint16_t a_register, uint8_t data);
  bool vl53l1x_write_bytes_16(uint8_t a_register, const uint16_t *data, uint8_t len);
  bool vl53l1x_write_byte_16(uint16_t a_register, uint16_t data);

  bool vl53l1x_read_byte(uint16_t a_register, uint8_t *data); 
  bool vl53l1x_read_bytes(uint16_t a_register, uint8_t *data, uint8_t len);

  bool vl53l1x_read_byte_16(uint16_t a_register, uint16_t *data);
  bool vl53l1x_read_bytes_16(uint16_t a_register, uint16_t *data, uint8_t len);
  
  uint32_t last_loop_time_{0};
  bool distance_mode_overriden_{false};
  bool running_update_{false};
  uint16_t sensor_id_{0};

  // --- self-heal / freeze-recovery state ---
  // The sensor is driven in one-shot mode (continuous mode only emits one frame
  // on this unit). Liveness is judged by frame freshness: a healthy loop reads a
  // frame every ~timing-budget, so no frame for FREEZE_TIMEOUT means it hung.
  uint16_t io_error_streak_{0};          // consecutive I2C failures (bus-lockup detect)
  uint32_t last_frame_ms_{0};            // millis() of the last successful frame read (freeze detect)
  uint32_t recovery_count_{0};           // total in-place recoveries since boot
  bool sensor_ok_{false};                // have we ever successfully initialised

  uint16_t above_distance_{0};
  uint16_t below_distance_{0};

  sensor::Sensor *distance_sensor_{nullptr};
  sensor::Sensor *range_status_sensor_{nullptr};
  sensor::Sensor *recovery_count_sensor_{nullptr};

};

}  // namespace vl53l1x
}  // namespace esphome
