#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

namespace esphome::onx_storage {

class OnxStorage;

struct NormalUiDeviceConfig {
  std::string title;
  std::string type;
  std::string entity_id;
};

struct NormalUiDeviceState {
  uint8_t flags{0};
  uint8_t power{0};
  uint8_t brightness{0};
  uint16_t color_temperature{0};
  uint8_t position{0};
  uint8_t reserved0{0};
  uint16_t reserved1{0};
  uint32_t extra_u32[4]{};
  uint16_t extra_u16[4]{};
};

enum NormalUiDeviceStateFlag : uint8_t {
  NORMAL_UI_DEVICE_STATE_FLAG_POWER = 1 << 0,
  NORMAL_UI_DEVICE_STATE_FLAG_BRIGHTNESS = 1 << 1,
  NORMAL_UI_DEVICE_STATE_FLAG_COLOR_TEMPERATURE = 1 << 2,
  NORMAL_UI_DEVICE_STATE_FLAG_POSITION = 1 << 3,
  NORMAL_UI_DEVICE_STATE_FLAG_EXTRA0 = 1 << 4,
  NORMAL_UI_DEVICE_STATE_FLAG_EXTRA1 = 1 << 5,
  NORMAL_UI_DEVICE_STATE_FLAG_EXTRA2 = 1 << 6,
  NORMAL_UI_DEVICE_STATE_FLAG_EXTRA3 = 1 << 7,
};

struct NormalLightState {
  bool is_on{false};
  uint8_t brightness_pct{0};
  uint8_t last_nonzero_brightness_pct{100};
  uint16_t color_temperature_kelvin{0};
  uint16_t min_color_temperature_kelvin{0};
  uint16_t max_color_temperature_kelvin{0};
  uint8_t rgb_tick{0};
  uint32_t capability_mask{1};
  uint32_t custom_capability_mask{0xF};

  bool supports_brightness() const;
  bool supports_color_temperature() const;
  bool supports_rgb() const;
  bool custom_supports_brightness() const;
  bool custom_supports_color_temperature() const;
  bool custom_supports_rgb() const;
  uint8_t color_temperature_step() const;
  uint16_t kelvin_from_color_temperature_step(uint8_t step) const;
  uint16_t hue_degrees() const;
};

struct NormalLightStatePatch {
  bool has_power{false};
  bool is_on{false};
  bool has_brightness_pct{false};
  uint8_t brightness_pct{0};
  bool has_color_temperature_kelvin{false};
  uint16_t color_temperature_kelvin{0};
  bool has_rgb_tick{false};
  uint8_t rgb_tick{0};
  bool has_capability_mask{false};
  uint32_t capability_mask{1};
  bool has_custom_capability_mask{false};
  uint32_t custom_capability_mask{0xF};
  bool has_color_temperature_range{false};
  uint16_t min_color_temperature_kelvin{0};
  uint16_t max_color_temperature_kelvin{0};
};

class NormalDeviceSlot {
 public:
  NormalDeviceSlot(OnxStorage *storage, size_t index);

  bool valid() const;
  size_t index() const;
  const std::string &title() const;
  const std::string &type() const;
  const std::string &entity_id() const;
  bool is_on() const;
  bool set_power(bool state) const;
  NormalLightState light() const;
  bool update_light(const NormalLightStatePatch &patch) const;

 protected:
  OnxStorage *storage_{nullptr};
  size_t index_{0};
};

// Storage abstraction for NVS/eFuse-backed device identity and factory metadata.
// The first version is still in-memory, but the API now mirrors the PRD fields we need.
class OnxStorage : public Component {
 public:
  void setup() override;
  void dump_config() override;

  bool is_auth_provisioned() const;
  void set_auth_provisioned(bool auth_provisioned);

  bool has_factory_wifi() const;
  const std::string &get_factory_ssid() const;
  const std::string &get_factory_password() const;
  bool set_factory_wifi(const std::string &factory_ssid, const std::string &factory_password);

  const std::string &get_auth_code() const;
  const std::string &get_auth_sha256() const;
  bool set_auth_record(const std::string &auth_code, const std::string &auth_sha256);

  bool reset_normal_mode_config();
  bool set_normal_ui_config(const std::array<NormalUiDeviceConfig, 10> &devices);
  const std::string &get_normal_ui_device_title(size_t index) const;
  const std::string &get_normal_ui_device_type(size_t index) const;
  const std::string &get_normal_ui_device_entity_id(size_t index) const;
  const NormalUiDeviceState &get_normal_ui_device_state_record(size_t index) const;
  bool set_normal_ui_device_state_record(size_t index, const NormalUiDeviceState &state);
  bool get_normal_ui_device_state(size_t index) const;
  bool set_normal_ui_device_state(size_t index, bool state);
  uint8_t get_normal_ui_device_brightness(size_t index) const;
  bool set_normal_ui_device_brightness(size_t index, uint8_t brightness);
  uint16_t get_normal_ui_device_color_temperature(size_t index) const;
  bool set_normal_ui_device_color_temperature(size_t index, uint16_t color_temperature);
  uint8_t get_normal_ui_device_position(size_t index) const;
  bool set_normal_ui_device_position(size_t index, uint8_t position);
  uint32_t get_normal_ui_device_extra_u32(size_t index, size_t slot) const;
  bool set_normal_ui_device_extra_u32(size_t index, size_t slot, uint32_t value);
  uint16_t get_normal_ui_device_extra_u16(size_t index, size_t slot) const;
  bool set_normal_ui_device_extra_u16(size_t index, size_t slot, uint16_t value);
  NormalDeviceSlot normal_device_slot(size_t index);
  NormalLightState get_normal_light_state(size_t index) const;
  bool update_normal_light_state(size_t index, const NormalLightStatePatch &patch);
  uint32_t get_standby_timeout_ms() const;
  bool set_standby_timeout_ms(uint32_t standby_timeout_ms);
  bool get_sleep_mode_enabled() const;
  uint16_t get_sleep_window_start_minute() const;
  uint16_t get_sleep_window_end_minute() const;
  uint32_t get_sleep_timeout_ms() const;
  bool set_sleep_mode_config(bool enabled, uint16_t start_minute, uint16_t end_minute, uint32_t timeout_ms);

 protected:
  void load_defaults_();
  bool save_factory_wifi_();
  bool save_auth_record_();
  bool save_normal_ui_config_();
  bool save_normal_ui_states_();
  bool save_power_policy_();

  static constexpr size_t FACTORY_SSID_MAX_LENGTH = 64;
  static constexpr size_t FACTORY_PASSWORD_MAX_LENGTH = 64;
  static constexpr size_t AUTH_CODE_MAX_LENGTH = 32;
  static constexpr size_t AUTH_SHA256_MAX_LENGTH = 64;
  static constexpr size_t NORMAL_UI_DEVICE_MAX_COUNT = 10;
  static constexpr size_t NORMAL_UI_TITLE_MAX_LENGTH = 128;
  static constexpr size_t NORMAL_UI_ICON_MAX_LENGTH = 16;
  static constexpr size_t NORMAL_UI_ENTITY_ID_MAX_LENGTH = 128;

  bool auth_provisioned_{false};
  std::string factory_ssid_;
  std::string factory_password_;
  std::string auth_code_;
  std::string auth_sha256_;
  std::array<NormalUiDeviceConfig, NORMAL_UI_DEVICE_MAX_COUNT> normal_ui_devices_{};
  std::array<NormalUiDeviceState, NORMAL_UI_DEVICE_MAX_COUNT> normal_ui_device_states_{};
  uint32_t standby_timeout_ms_{300000};
  uint32_t sleep_timeout_ms_{60000};
  uint16_t sleep_window_start_minute_{0};
  uint16_t sleep_window_end_minute_{0};
  bool sleep_mode_enabled_{false};
  ESPPreferenceObject factory_wifi_pref_;
  ESPPreferenceObject auth_record_pref_;
  ESPPreferenceObject normal_ui_pref_;
  ESPPreferenceObject normal_ui_state_pref_;
  ESPPreferenceObject normal_ui_state_legacy_pref_;
  ESPPreferenceObject power_policy_pref_;
};

}  // namespace esphome::onx_storage
