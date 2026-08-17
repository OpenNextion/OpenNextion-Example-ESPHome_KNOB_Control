#pragma once

#include <cstdint>
#include <string>

#include "esphome/core/component.h"
#include "../onx_app_state/onx_app_state.h"
#include "../onx_storage/onx_storage.h"

namespace esphome::onx_ui {

class OnxUi : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_app_state(onx_app_state::OnxAppState *app_state);
  void set_storage(onx_storage::OnxStorage *storage);

  bool consume_ui_dirty();
  bool is_boot_page_active() const;

  std::string get_status_mode_line() const;
  std::string get_status_link_line() const;
  std::string get_status_reason_line() const;
  std::string get_status_rx_line() const;

  bool is_normal_mode_active() const;
  bool is_standby_active() const;
  void notify_user_activity();
  void set_standby_timeout_ms(uint32_t standby_timeout_ms);
  void set_sleep_mode_config(bool enabled, const std::string &start_time, const std::string &end_time, uint32_t timeout_ms);
  bool get_sleep_mode_enabled() const;
  std::string get_sleep_window_start_time() const;
  std::string get_sleep_window_end_time() const;
  uint32_t get_sleep_timeout_ms() const;
  bool is_sleep_mode_window_active(uint8_t hour, uint8_t minute) const;
  bool should_enter_sleep_mode(uint8_t hour, uint8_t minute) const;
  std::string get_normal_device_title(size_t index) const;
  std::string get_normal_device_type(size_t index) const;
  std::string get_normal_device_entity_id(size_t index) const;
  std::string get_normal_ssid_line() const;
  std::string get_normal_rssi_line() const;
  std::string get_normal_ip_line() const;

 protected:
  static std::string format_mac_with_colons_(const std::string &raw_mac);
  static std::string format_minutes_as_time_(uint16_t minutes);

  onx_app_state::OnxAppState *app_state_{nullptr};
  onx_storage::OnxStorage *storage_{nullptr};
  uint32_t standby_timeout_ms_{300000};
  uint32_t last_user_activity_ms_{0};
  uint32_t sleep_timeout_ms_{60000};
  uint16_t sleep_window_start_minute_{0};
  uint16_t sleep_window_end_minute_{0};
  bool sleep_mode_enabled_{false};
  bool last_mode_normal_{false};
};

}  // namespace esphome::onx_ui
