#include "vl53l1x_pololu.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace vl53l1x_pololu {

static const char *const TAG = "vl53l1x_pololu";

void VL53L1XPololuComponent::setup() {
  this->sensor_.set_i2c(this);
  this->sensor_.setTimeout(500);
  if (!this->sensor_.init()) {
    ESP_LOGE(TAG, "Pololu VL53L1X init() failed (model id / boot)");
    this->mark_failed();
    return;
  }
  this->sensor_.setDistanceMode(VL53L1X::Long);
  this->sensor_.setMeasurementTimingBudget(50000);  // 50 ms
  // CONTINUOUS / autonomous ranging at a 100 ms inter-measurement period
  // (> timing budget, per UM2356). This is the exact path our own driver can't
  // sustain on this module — the A/B test.
  this->sensor_.startContinuous(100);
  this->ok_ = true;
  ESP_LOGCONFIG(TAG, "Pololu VL53L1X: continuous started (50ms budget / 100ms period)");
}

void VL53L1XPololuComponent::loop() {
  if (!this->ok_)
    return;
  if (this->sensor_.dataReady()) {
    uint16_t mm = this->sensor_.read(false);  // reads results + clears interrupt
    this->last_distance_ = mm;
    this->frame_count_++;
  }
  // Once a second, report whether frames keep arriving (the whole point of the test):
  // a sustaining sensor shows frames + stream_count climbing; a halt freezes both.
  if (millis() - this->last_log_ms_ > 1000) {
    this->last_log_ms_ = millis();
    ESP_LOGD(TAG, "frames=%u stream=%u dist=%umm status=%u",
             (unsigned) this->frame_count_, this->sensor_.getStreamCount(),
             this->last_distance_, (unsigned) this->sensor_.ranging_data.range_status);
  }
}

void VL53L1XPololuComponent::update() {
  if (!this->ok_)
    return;
  if (this->distance_sensor_ != nullptr)
    this->distance_sensor_->publish_state(this->last_distance_);
  if (this->frames_sensor_ != nullptr)
    this->frames_sensor_->publish_state(this->frame_count_);
}

void VL53L1XPololuComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "VL53L1X (Pololu port), continuous ranging:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed())
    ESP_LOGE(TAG, "  init failed");
}

}  // namespace vl53l1x_pololu
}  // namespace esphome
