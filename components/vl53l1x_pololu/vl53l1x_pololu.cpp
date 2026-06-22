#include "vl53l1x_pololu.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace vl53l1x_pololu {

static const char *const TAG = "vl53l1x_pololu";

// No fresh ranging frame for this long => sensor hung => re-initialise in place.
// Continuous ranging delivers a frame every inter-measurement period, so this is
// generous; a live sensor never trips it.
static const uint32_t FREEZE_TIMEOUT_MS = 5000;

bool VL53L1XPololuComponent::start_ranging_() {
  if (!this->sensor_.setDistanceMode((VL53L1X::DistanceMode) this->distance_mode_)) {
    ESP_LOGE(TAG, "setDistanceMode failed");
    return false;
  }
  if (!this->sensor_.setMeasurementTimingBudget(this->timing_budget_us_)) {
    ESP_LOGE(TAG, "setMeasurementTimingBudget(%u us) failed", (unsigned) this->timing_budget_us_);
    return false;
  }
  this->sensor_.startContinuous(this->inter_measurement_ms_);
  return true;
}

void VL53L1XPololuComponent::setup() {
  this->sensor_.set_i2c(this);
  this->sensor_.setTimeout(500);
  if (!this->sensor_.init()) {
    ESP_LOGE(TAG, "Pololu VL53L1X init() failed (model id / boot)");
    this->mark_failed();
    return;
  }
  if (!this->start_ranging_()) {
    this->mark_failed();
    return;
  }
  this->last_frame_ms_ = millis();
  this->ok_ = true;
}

void VL53L1XPololuComponent::loop() {
  if (!this->ok_)
    return;

  if (this->sensor_.dataReady()) {
    this->last_distance_ = this->sensor_.read(false);  // reads results + clears interrupt
    this->last_status_ = this->sensor_.ranging_data.range_status;
    this->frame_count_++;
    this->last_frame_ms_ = millis();
    return;
  }

  // Freeze recovery: no fresh frame for FREEZE_TIMEOUT_MS => re-init in place.
  if (millis() - this->last_frame_ms_ > FREEZE_TIMEOUT_MS) {
    ESP_LOGW(TAG, "No ranging frame for %ums — re-initialising sensor",
             (unsigned) (millis() - this->last_frame_ms_));
    this->sensor_.stopContinuous();  // best effort
    if (this->sensor_.init() && this->start_ranging_()) {
      this->recovery_count_++;
      if (this->recovery_count_sensor_ != nullptr)
        this->recovery_count_sensor_->publish_state(this->recovery_count_);
      ESP_LOGW(TAG, "Sensor recovered (total recoveries: %u)", (unsigned) this->recovery_count_);
    } else {
      ESP_LOGE(TAG, "Re-init failed; will retry");
    }
    this->last_frame_ms_ = millis();
  }
}

void VL53L1XPololuComponent::update() {
  if (!this->ok_)
    return;
  if (this->distance_sensor_ != nullptr)
    this->distance_sensor_->publish_state(this->last_distance_);
  if (this->frames_sensor_ != nullptr)
    this->frames_sensor_->publish_state(this->frame_count_);
  if (this->recovery_count_sensor_ != nullptr)
    this->recovery_count_sensor_->publish_state(this->recovery_count_);
}

void VL53L1XPololuComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "VL53L1X (Pololu port), continuous ranging:");
  ESP_LOGCONFIG(TAG, "  Distance mode: %u (0=short,1=medium,2=long)", this->distance_mode_);
  ESP_LOGCONFIG(TAG, "  Timing budget: %u us", (unsigned) this->timing_budget_us_);
  ESP_LOGCONFIG(TAG, "  Inter-measurement: %u ms", (unsigned) this->inter_measurement_ms_);
  LOG_I2C_DEVICE(this);
  if (this->is_failed())
    ESP_LOGE(TAG, "  init failed");
}

}  // namespace vl53l1x_pololu
}  // namespace esphome
