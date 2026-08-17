#pragma once

#include <cstdint>
#include <string>

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "../onx_app_state/onx_app_state.h"

namespace esphome::onx_mode_manager {

// Boot-time mode arbiter for Normal Mode.
class OnxModeManager : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;

  void set_app_state(onx_app_state::OnxAppState *app_state);
  void set_normal_wifi_ssid(const std::string &normal_wifi_ssid);
  void set_normal_wifi_password(const std::string &normal_wifi_password);
  void set_runtime_log_level(uint8_t level);
  void mark_boot_finished();
  bool is_provisioning_mode_enabled() const { return this->provisioning_mode_enabled_; }
  bool clear_saved_normal_wifi_prefs_();

 protected:
  void evaluate_boot_mode_();
  void evaluate_runtime_mode_();
  void apply_wifi_for_mode_(onx_app_state::OnxRuntimeMode mode);
  void disable_provisioning_mode_();
  void enable_provisioning_mode_();
  bool has_saved_normal_wifi_() const;
  void schedule_normal_wifi_connect_(const std::string &ssid, const std::string &password, bool use_explicit_credentials);
  const char *get_reset_reason_name_() const;
  void select_mode_(onx_app_state::OnxRuntimeMode mode, const std::string &reason);

  onx_app_state::OnxAppState *app_state_{nullptr};
  bool mode_decided_{false};
  bool missing_binding_logged_{false};
  bool boot_finished_{false};
  uint8_t runtime_log_level_{ESPHOME_LOG_LEVEL_INFO};
  std::string normal_wifi_ssid_;
  std::string normal_wifi_password_;
  std::string applied_wifi_ssid_;
  std::string applied_wifi_password_;
  bool provisioning_mode_enabled_{false};
  bool normal_wifi_connect_scheduled_{false};
};

}  // namespace esphome::onx_mode_manager
