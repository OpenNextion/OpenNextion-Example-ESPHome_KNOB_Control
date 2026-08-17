#include "onx_ui.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/components/wifi/wifi_component.h"
#include <algorithm>
#include <cstdio>
#include <span>

namespace esphome::onx_ui {

static const char *const TAG = "onx_ui";

static bool parse_time_to_minutes_(const std::string &value, uint16_t *minutes) {
  if (minutes == nullptr) {
    return false;
  }
  if (value.size() < 5) {
    return false;
  }

  const auto parse_two_digits = [](char tens, char ones) -> int {
    if (tens < '0' || tens > '9' || ones < '0' || ones > '9') {
      return -1;
    }
    return (tens - '0') * 10 + (ones - '0');
  };

  const int hour = parse_two_digits(value[0], value[1]);
  const int minute = parse_two_digits(value[3], value[4]);
  if (hour < 0 || minute < 0) {
    return false;
  }

  const int clamped_hour = std::clamp(hour, 0, 23);
  const int clamped_minute = std::clamp(minute, 0, 59);
  *minutes = static_cast<uint16_t>(clamped_hour * 60 + clamped_minute);
  return true;
}

void OnxUi::setup() {
  this->last_user_activity_ms_ = millis();
  if (this->storage_ != nullptr) {
    this->standby_timeout_ms_ = this->storage_->get_standby_timeout_ms();
    this->sleep_mode_enabled_ = this->storage_->get_sleep_mode_enabled();
    this->sleep_window_start_minute_ = this->storage_->get_sleep_window_start_minute();
    this->sleep_window_end_minute_ = this->storage_->get_sleep_window_end_minute();
    this->sleep_timeout_ms_ = this->storage_->get_sleep_timeout_ms();
  }
}

void OnxUi::loop() {
  const bool normal_mode = this->is_normal_mode_active();
  if (normal_mode && !this->last_mode_normal_) {
    this->last_user_activity_ms_ = millis();
  }
  this->last_mode_normal_ = normal_mode;
}

void OnxUi::dump_config() {
  ESP_LOGCONFIG(TAG, "ONX UI:");
  ESP_LOGCONFIG(TAG, "  App state bound: %s", this->app_state_ != nullptr ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Storage bound: %s", this->storage_ != nullptr ? "YES" : "NO");
}

void OnxUi::set_app_state(onx_app_state::OnxAppState *app_state) { this->app_state_ = app_state; }

void OnxUi::set_storage(onx_storage::OnxStorage *storage) { this->storage_ = storage; }

bool OnxUi::consume_ui_dirty() {
  if (this->is_normal_mode_active()) {
    return false;
  }
  if (this->app_state_ == nullptr || !this->app_state_->is_ui_dirty()) {
    return false;
  }
  this->app_state_->clear_ui_dirty();
  return true;
}

bool OnxUi::is_boot_page_active() const {
  return this->app_state_ == nullptr ||
         this->app_state_->get_current_mode() == onx_app_state::OnxRuntimeMode::MODE_UNKNOWN;
}

std::string OnxUi::get_status_mode_line() const {
  if (this->app_state_ == nullptr) {
    return "booting";
  }
  if (this->app_state_->get_current_mode() == onx_app_state::OnxRuntimeMode::MODE_UNKNOWN) {
    return "booting";
  }
  return std::string(this->app_state_->get_current_mode_name());
}

std::string OnxUi::get_status_link_line() const {
  if (this->app_state_ == nullptr) {
    return "idle";
  }
  return std::string(this->app_state_->get_link_state_name());
}

std::string OnxUi::get_status_reason_line() const {
  if (this->app_state_ == nullptr) {
    return "-";
  }

  auto reason = this->app_state_->get_mode_reason();
  if (reason.empty()) {
    return "-";
  }
  if (reason.size() > 24) {
    return reason.substr(0, 21) + "...";
  }
  return reason;
}

std::string OnxUi::get_status_rx_line() const {
  if (this->app_state_ == nullptr) {
    return "waiting for mode UI";
  }

  const auto rx = this->app_state_->get_last_rx_line();
  if (rx.empty()) {
    return "waiting for mode UI";
  }
  return rx;
}

bool OnxUi::is_normal_mode_active() const {
  if (this->app_state_ == nullptr) {
    return false;
  }
  return this->app_state_->get_current_mode() == onx_app_state::OnxRuntimeMode::MODE_NORMAL;
}

bool OnxUi::is_standby_active() const {
  if (!this->is_normal_mode_active()) {
    return false;
  }
  const uint32_t idle_ms = millis() - this->last_user_activity_ms_;
  return idle_ms >= this->standby_timeout_ms_;
}

void OnxUi::notify_user_activity() { this->last_user_activity_ms_ = millis(); }

void OnxUi::set_standby_timeout_ms(uint32_t standby_timeout_ms) {
  this->standby_timeout_ms_ = standby_timeout_ms;
  if (this->storage_ != nullptr && !this->storage_->set_standby_timeout_ms(standby_timeout_ms)) {
    ESP_LOGW(TAG, "Failed to persist standby timeout");
  }
}

void OnxUi::set_sleep_mode_config(bool enabled,
                                 const std::string &start_time,
                                 const std::string &end_time,
                                 uint32_t timeout_ms) {
  this->sleep_mode_enabled_ = enabled;
  this->sleep_timeout_ms_ = timeout_ms < 1 ? 1 : timeout_ms;

  uint16_t start_minutes = 0;
  uint16_t end_minutes = 0;
  if (!parse_time_to_minutes_(start_time, &start_minutes)) {
    ESP_LOGW(TAG, "Invalid sleep start time: %s", start_time.c_str());
  }
  if (!parse_time_to_minutes_(end_time, &end_minutes)) {
    ESP_LOGW(TAG, "Invalid sleep end time: %s", end_time.c_str());
  }

  this->sleep_window_start_minute_ = start_minutes;
  this->sleep_window_end_minute_ = end_minutes;
  if (this->storage_ != nullptr &&
      !this->storage_->set_sleep_mode_config(this->sleep_mode_enabled_, this->sleep_window_start_minute_,
                                             this->sleep_window_end_minute_, this->sleep_timeout_ms_)) {
    ESP_LOGW(TAG, "Failed to persist sleep mode config");
  }
}

bool OnxUi::is_sleep_mode_window_active(uint8_t hour, uint8_t minute) const {
  if (!this->is_normal_mode_active() || !this->sleep_mode_enabled_) {
    return false;
  }

  const uint16_t now_minute = static_cast<uint16_t>(hour) * 60 + static_cast<uint16_t>(minute);
  return [&]() {
    const uint16_t start = this->sleep_window_start_minute_;
    const uint16_t end = this->sleep_window_end_minute_;
    if (start == end) {
      return true;
    }
    if (start < end) {
      return now_minute >= start && now_minute < end;
    }
    return now_minute >= start || now_minute < end;
  }();
}

bool OnxUi::should_enter_sleep_mode(uint8_t hour, uint8_t minute) const {
  if (!this->is_sleep_mode_window_active(hour, minute)) {
    return false;
  }

  const uint32_t idle_ms = millis() - this->last_user_activity_ms_;
  return idle_ms >= this->sleep_timeout_ms_;
}

bool OnxUi::get_sleep_mode_enabled() const { return this->sleep_mode_enabled_; }

std::string OnxUi::get_sleep_window_start_time() const {
  return format_minutes_as_time_(this->sleep_window_start_minute_);
}

std::string OnxUi::get_sleep_window_end_time() const {
  return format_minutes_as_time_(this->sleep_window_end_minute_);
}

uint32_t OnxUi::get_sleep_timeout_ms() const { return this->sleep_timeout_ms_; }

std::string OnxUi::get_normal_device_title(size_t index) const {
  if (this->storage_ == nullptr) {
    return "";
  }
  return this->storage_->get_normal_ui_device_title(index);
}

std::string OnxUi::get_normal_device_type(size_t index) const {
  if (this->storage_ == nullptr) {
    return "";
  }
  return this->storage_->get_normal_ui_device_type(index);
}

std::string OnxUi::get_normal_device_entity_id(size_t index) const {
  if (this->storage_ == nullptr) {
    return "";
  }
  return this->storage_->get_normal_ui_device_entity_id(index);
}

std::string OnxUi::get_normal_ssid_line() const {
  if (wifi::global_wifi_component != nullptr) {
    if (wifi::global_wifi_component->is_connected()) {
      char buffer[33];
      return wifi::global_wifi_component->wifi_ssid_to(std::span<char, 33>(buffer));
    }
  }
  return "Disconnected";
}

std::string OnxUi::get_normal_rssi_line() const {
  if (wifi::global_wifi_component != nullptr) {
    if (wifi::global_wifi_component->is_connected()) {
      return std::to_string(wifi::global_wifi_component->wifi_rssi()) + " dBm";
    }
  }
  return "- dBm";
}

std::string OnxUi::get_normal_ip_line() const {
  if (wifi::global_wifi_component != nullptr) {
    if (wifi::global_wifi_component->is_connected()) {
      auto ips = wifi::global_wifi_component->get_ip_addresses();
      if (!ips.empty()) {
        return ips[0].str();
      }
    }
  }
  return "0.0.0.0";
}

std::string OnxUi::format_mac_with_colons_(const std::string &raw_mac) {
  if (raw_mac.size() != 12) {
    return raw_mac;
  }
  std::string formatted;
  formatted.reserve(17);
  for (size_t i = 0; i < raw_mac.size(); i += 2) {
    if (!formatted.empty()) {
      formatted.push_back(':');
    }
    formatted.push_back(raw_mac[i]);
    formatted.push_back(raw_mac[i + 1]);
  }
  return formatted;
}

std::string OnxUi::format_minutes_as_time_(uint16_t minutes) {
  minutes = std::min<uint16_t>(minutes, 1439);
  char buffer[9];
  snprintf(buffer, sizeof(buffer), "%02u:%02u:00",
           static_cast<unsigned>(minutes / 60),
           static_cast<unsigned>(minutes % 60));
  return buffer;
}

}  // namespace esphome::onx_ui
