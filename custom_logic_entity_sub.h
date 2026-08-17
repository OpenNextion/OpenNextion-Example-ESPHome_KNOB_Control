#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <string>
#include <utility>

#include <ArduinoJson.h>
#include "esphome.h"
#include "esphome/components/api/api_server.h"
#include "esphome/components/select/select.h"

namespace custom_logic_entity_sub {

static const char *const TAG = "CustomLogicEntitySub";
static constexpr uint32_t NORMAL_LIGHT_FEEDBACK_SETTLE_MS = 1113;

struct NormalLightFeedbackWindow {
  uint32_t deadline_ms{0};
  uint8_t target_mask{0};
  bool target_power{false};
  int target_brightness_pct{-1};
  int target_color_temperature_kelvin{-1};
  int target_rgb_tick{-1};
  bool pending_power{false};
  std::string pending_state{};
  bool pending_brightness{false};
  std::string pending_brightness_value{};
  bool pending_color_temperature{false};
  std::string pending_color_temperature_value{};
  bool pending_rgb{false};
  std::string pending_hs_color_value{};
  bool pending_supported_color_modes{false};
  std::string pending_supported_color_modes_value{};
  bool pending_min_color_temp_kelvin{false};
  std::string pending_min_color_temp_kelvin_value{};
  bool pending_max_color_temp_kelvin{false};
  std::string pending_max_color_temp_kelvin_value{};
  bool pending_min_mireds{false};
  std::string pending_min_mireds_value{};
  bool pending_max_mireds{false};
  std::string pending_max_mireds_value{};
};

static std::array<NormalLightFeedbackWindow, custom_logic::CONFIGURABLE_DEVICE_SLOT_COUNT>
    normal_light_feedback_windows{};

inline bool normal_light_feedback_window_active(size_t ui_slot) {
  if (ui_slot >= normal_light_feedback_windows.size()) {
    return false;
  }
  return static_cast<int32_t>(normal_light_feedback_windows[ui_slot].deadline_ms - millis()) > 0;
}

inline void begin_normal_light_feedback_window(size_t ui_slot, uint8_t target_mask, bool target_power,
                                               int target_value) {
  if (ui_slot >= normal_light_feedback_windows.size()) {
    return;
  }
  auto &window = normal_light_feedback_windows[ui_slot];
  window.deadline_ms = millis() + NORMAL_LIGHT_FEEDBACK_SETTLE_MS;
  window.target_mask |= target_mask;
  window.target_power = target_power;
  window.pending_power = false;
  window.pending_state.clear();
  if ((target_mask & custom_logic::NORMAL_LIGHT_FEEDBACK_BRIGHTNESS) != 0) {
    window.target_brightness_pct = std::clamp(target_value, 0, 100);
    window.pending_brightness = false;
    window.pending_brightness_value.clear();
  }
  if ((target_mask & custom_logic::NORMAL_LIGHT_FEEDBACK_COLOR_TEMPERATURE) != 0) {
    window.target_color_temperature_kelvin = target_value;
    window.pending_color_temperature = false;
    window.pending_color_temperature_value.clear();
  }
  if ((target_mask & custom_logic::NORMAL_LIGHT_FEEDBACK_RGB) != 0) {
    window.target_rgb_tick = std::clamp(target_value, 0, 35);
    window.pending_rgb = false;
    window.pending_hs_color_value.clear();
  }
}

inline void flush_normal_light_feedback_window(size_t ui_slot, const std::string &entity_id);

inline bool has_prefix(const std::string &value, const char *prefix) {
  return value.rfind(prefix, 0) == 0;
}

inline bool is_valid_entity_id(const std::string &entity_id) {
  if (entity_id.empty() || entity_id.size() > custom_logic::MAX_HA_ENTITY_ID_LENGTH ||
      entity_id.find('.') == std::string::npos) {
    return false;
  }
  for (char ch : entity_id) {
    if (std::iscntrl(static_cast<unsigned char>(ch)) || std::isspace(static_cast<unsigned char>(ch))) {
      return false;
    }
  }
  return true;
}

inline std::string entity_id_from_select(esphome::select::Select *select) {
  if (select == nullptr) {
    return "";
  }
  return custom_logic::extract_entity_id_from_device1_option(select->state);
}

inline const std::array<const char *, 8> &light_attributes() {
  static const std::array<const char *, 8> attrs{{
      "supported_color_modes",
      "min_color_temp_kelvin",
      "max_color_temp_kelvin",
      "min_mireds",
      "max_mireds",
      "brightness",
      "color_temp_kelvin",
      "hs_color",
  }};
  return attrs;
}

inline const std::array<const char *, 1> &cover_attributes() {
  static const std::array<const char *, 1> attrs{{"current_position"}};
  return attrs;
}

inline const std::array<const char *, 1> &automation_attributes() {
  static const std::array<const char *, 1> attrs{{"last_triggered"}};
  return attrs;
}

inline const std::array<const char *, 3> &weather_attributes() {
  static const std::array<const char *, 3> attrs{{"temperature", "temperature_unit", "humidity"}};
  return attrs;
}

inline std::array<std::string, 10> &automation_last_triggered_values() {
  static std::array<std::string, 10> values{};
  return values;
}

inline std::array<std::string, 10> &automation_last_triggered_entity_ids() {
  static std::array<std::string, 10> entity_ids{};
  return entity_ids;
}

inline bool parse_ha_datetime_epoch(const std::string &raw_value, time_t *epoch_out) {
  if (epoch_out == nullptr) {
    return false;
  }
  std::string value = raw_value;
  if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
    value = value.substr(1, value.size() - 2);
  }
  if (value.size() < 19 || value == "None" || value == "none" || value == "null" ||
      value == "unknown" || value == "unavailable") {
    return false;
  }

  std::string date_time = value.substr(0, 19);
  if (date_time[10] == 'T') {
    date_time[10] = ' ';
  }
  esphome::ESPTime parsed{};
  parsed.day_of_week = 1;
  parsed.day_of_year = 1;
  if (!esphome::ESPTime::strptime(date_time, parsed)) {
    return false;
  }
  parsed.recalc_timestamp_utc(false);
  if (parsed.timestamp < 0) {
    return false;
  }

  size_t suffix = 19;
  if (suffix < value.size() && value[suffix] == '.') {
    suffix++;
    while (suffix < value.size() && value[suffix] >= '0' && value[suffix] <= '9') {
      suffix++;
    }
  }
  int32_t offset_seconds = 0;
  if (suffix < value.size() && (value[suffix] == '+' || value[suffix] == '-')) {
    const int sign = value[suffix] == '+' ? 1 : -1;
    if (suffix + 5 >= value.size() || value[suffix + 3] != ':') {
      return false;
    }
    const char hour_tens = value[suffix + 1];
    const char hour_ones = value[suffix + 2];
    const char minute_tens = value[suffix + 4];
    const char minute_ones = value[suffix + 5];
    if (hour_tens < '0' || hour_tens > '9' || hour_ones < '0' || hour_ones > '9' ||
        minute_tens < '0' || minute_tens > '9' || minute_ones < '0' || minute_ones > '9') {
      return false;
    }
    const int hours = (hour_tens - '0') * 10 + (hour_ones - '0');
    const int minutes = (minute_tens - '0') * 10 + (minute_ones - '0');
    if (hours > 23 || minutes > 59) {
      return false;
    }
    offset_seconds = sign * (hours * 3600 + minutes * 60);
  }
  *epoch_out = parsed.timestamp - offset_seconds;
  return true;
}

inline int64_t civil_day_number(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned year_of_era = static_cast<unsigned>(year - era * 400);
  const unsigned adjusted_month = month > 2 ? month - 3 : month + 9;
  const unsigned day_of_year = (153 * adjusted_month + 2) / 5 + day - 1;
  const unsigned day_of_era = year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
  return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(day_of_era) - 719468;
}

inline int64_t floor_mod(int64_t value, int64_t divisor) {
  const int64_t remainder = value % divisor;
  return remainder < 0 ? remainder + divisor : remainder;
}

inline int js_round(double value) {
  return static_cast<int>(std::floor(value + 0.5));
}

inline std::string format_english_relative_time(int value, const char *unit) {
  const std::string unit_text(unit);
  if (value == 0) {
    if (unit_text == "second") return "now";
    if (unit_text == "day") return "today";
    if (unit_text == "week") return "this week";
    if (unit_text == "month") return "this month";
    if (unit_text == "year") return "this year";
  }
  if (value == -1) {
    if (unit_text == "day") return "yesterday";
    if (unit_text == "week") return "last week";
    if (unit_text == "month") return "last month";
    if (unit_text == "year") return "last year";
  }
  if (value == 1) {
    if (unit_text == "day") return "tomorrow";
    if (unit_text == "week") return "next week";
    if (unit_text == "month") return "next month";
    if (unit_text == "year") return "next year";
  }

  const int magnitude = value < 0 ? -value : value;
  const std::string quantity = std::to_string(magnitude) + " " + unit_text + (magnitude == 1 ? "" : "s");
  return value < 0 ? quantity + " ago" : "in " + quantity;
}

inline bool is_missing_ha_datetime(const std::string &raw_value) {
  std::string value = raw_value;
  if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
    value = value.substr(1, value.size() - 2);
  }
  return value.empty() || value == "None" || value == "none" || value == "null" ||
         value == "unknown" || value == "unavailable";
}

inline std::string automation_last_triggered_relative_text(size_t ui_slot, const std::string &entity_id,
                                                           time_t now_epoch) {
  if (ui_slot >= automation_last_triggered_values().size() ||
      automation_last_triggered_entity_ids()[ui_slot] != entity_id) {
    return "Never";
  }
  const std::string &raw_value = automation_last_triggered_values()[ui_slot];
  if (is_missing_ha_datetime(raw_value)) {
    return "Never";
  }
  time_t epoch = 0;
  if (!parse_ha_datetime_epoch(raw_value, &epoch)) {
    return "Invalid date";
  }
  const esphome::ESPTime from = esphome::ESPTime::from_epoch_local(epoch);
  const esphome::ESPTime to = esphome::ESPTime::from_epoch_local(now_epoch);
  if (!from.is_valid() || !to.is_valid()) {
    return "Invalid date";
  }

  const double seconds = static_cast<double>(epoch - now_epoch);
  if (std::fabs(seconds) < 59.0) {
    return format_english_relative_time(js_round(seconds), "second");
  }

  const double minutes = seconds / 60.0;
  if (std::fabs(minutes) < 59.0) {
    return format_english_relative_time(js_round(minutes), "minute");
  }

  const double hours = minutes / 60.0;
  if (std::fabs(hours) < 22.0) {
    return format_english_relative_time(js_round(hours), "hour");
  }

  const int64_t from_day = civil_day_number(from.year, from.month, from.day_of_month);
  const int64_t to_day = civil_day_number(to.year, to.month, to.day_of_month);
  const int days = static_cast<int>(from_day - to_day);
  if (days == 0) {
    return format_english_relative_time(js_round(hours), "hour");
  }
  if (std::abs(days) < 5) {
    return format_english_relative_time(days, "day");
  }

  // The UI is English, whose HA/Intl week starts on Sunday.
  const int64_t from_week = from_day - floor_mod(from_day + 4, 7);
  const int64_t to_week = to_day - floor_mod(to_day + 4, 7);
  const int weeks = static_cast<int>((from_week - to_week) / 7);
  if (weeks == 0) {
    return format_english_relative_time(days, "day");
  }
  if (std::abs(weeks) < 4) {
    return format_english_relative_time(weeks, "week");
  }

  const int years = from.year - to.year;
  const int months = years * 12 + from.month - to.month;
  if (months == 0) {
    return format_english_relative_time(weeks, "week");
  }
  if (std::abs(months) < 11 || years == 0) {
    return format_english_relative_time(months, "month");
  }
  return format_english_relative_time(years, "year");
}

class DeviceSlotStateUpdater {
 public:
  DeviceSlotStateUpdater(size_t slot, std::string entity_id) : slot_(slot), entity_id_(std::move(entity_id)) {}

  void on_state_changed(const std::string &field, esphome::StringRef raw_value) {
    const std::string value = raw_value.str();
    // ESP_LOGI(TAG, "%s entity=%s field=%s value=%s", this->slot_label_().c_str(), this->entity_id_.c_str(), field.c_str(),
    //          value.c_str());

    if (!this->storage_ready_()) {
      ESP_LOGW(TAG, "%s state ignored; normal UI storage is unavailable", this->slot_label_().c_str());
      return;
    }
    const size_t ui_slot = this->ui_slot_();
    if (this->is_light_() && ui_slot != static_cast<size_t>(-1) &&
        this->defer_nonmatching_light_feedback_(ui_slot, field, value)) {
      return;
    }
    if (field == "state") {
      const bool online = !value.empty() && value != "unknown" && value != "unavailable";
      if (ui_slot == static_cast<size_t>(-1)) {
        // ESP_LOGI(TAG, "%s state ignored; no compact UI slot for entity=%s", this->slot_label_().c_str(),
        //          this->entity_id_.c_str());
        return;
      }
      custom_logic::set_normal_ui_device_online_state(ui_slot, online);
      if (!online) {
        return;
      }
    }
    if (field == "state") {
      this->apply_power_or_cover_state_(value);
    } else if (field == "brightness") {
      this->apply_brightness_(value);
    } else if (field == "color_temp_kelvin") {
      this->apply_color_temperature_(value);
    } else if (field == "hs_color") {
      this->apply_hs_color_(value);
    } else if (field == "current_position") {
      this->apply_cover_position_(value);
    } else if (field == "last_triggered") {
      this->apply_automation_last_triggered_(value);
    } else if (field == "supported_color_modes") {
      this->apply_supported_color_modes_(value);
    } else if (field == "min_color_temp_kelvin") {
      this->apply_color_temp_range_(this->parse_int_(value, -1), -1);
    } else if (field == "max_color_temp_kelvin") {
      this->apply_color_temp_range_(-1, this->parse_int_(value, -1));
    } else if (field == "min_mireds") {
      this->apply_color_temp_range_(-1, this->kelvin_from_mired_(value));
    } else if (field == "max_mireds") {
      this->apply_color_temp_range_(this->kelvin_from_mired_(value), -1);
    }
  }

  void flush_deferred_light_feedback() {
    const size_t ui_slot = this->ui_slot_();
    if (!this->is_light_() || ui_slot == static_cast<size_t>(-1) ||
        normal_light_feedback_window_active(ui_slot)) {
      return;
    }
    auto &window = normal_light_feedback_windows[ui_slot];
    if (window.pending_power) {
      const std::string state = window.pending_state;
      window.pending_power = false;
      window.pending_state.clear();
      const bool online = !state.empty() && state != "unknown" && state != "unavailable";
      custom_logic::set_normal_ui_device_online_state(ui_slot, online);
      if (online) {
        this->apply_power_or_cover_state_(state);
      }
    }
    if (window.pending_brightness) {
      const std::string value = window.pending_brightness_value;
      window.pending_brightness = false;
      window.pending_brightness_value.clear();
      this->apply_brightness_(value);
    }
    if (window.pending_color_temperature) {
      const std::string value = window.pending_color_temperature_value;
      window.pending_color_temperature = false;
      window.pending_color_temperature_value.clear();
      this->apply_color_temperature_(value);
    }
    if (window.pending_rgb) {
      const std::string value = window.pending_hs_color_value;
      window.pending_rgb = false;
      window.pending_hs_color_value.clear();
      this->apply_hs_color_(value);
    }
    if (window.pending_supported_color_modes) {
      const std::string value = window.pending_supported_color_modes_value;
      window.pending_supported_color_modes = false;
      window.pending_supported_color_modes_value.clear();
      this->apply_supported_color_modes_(value);
    }
    if (window.pending_min_color_temp_kelvin) {
      const std::string value = window.pending_min_color_temp_kelvin_value;
      window.pending_min_color_temp_kelvin = false;
      window.pending_min_color_temp_kelvin_value.clear();
      this->apply_color_temp_range_(this->parse_int_(value, -1), -1);
    }
    if (window.pending_max_color_temp_kelvin) {
      const std::string value = window.pending_max_color_temp_kelvin_value;
      window.pending_max_color_temp_kelvin = false;
      window.pending_max_color_temp_kelvin_value.clear();
      this->apply_color_temp_range_(-1, this->parse_int_(value, -1));
    }
    if (window.pending_min_mireds) {
      const std::string value = window.pending_min_mireds_value;
      window.pending_min_mireds = false;
      window.pending_min_mireds_value.clear();
      this->apply_color_temp_range_(-1, this->kelvin_from_mired_(value));
    }
    if (window.pending_max_mireds) {
      const std::string value = window.pending_max_mireds_value;
      window.pending_max_mireds = false;
      window.pending_max_mireds_value.clear();
      this->apply_color_temp_range_(this->kelvin_from_mired_(value), -1);
    }
    window.target_mask = 0;
    window.target_brightness_pct = -1;
    window.target_color_temperature_kelvin = -1;
    window.target_rgb_tick = -1;
  }

 private:
  std::string slot_label_() const { return custom_logic::device_slot_label(this->slot_); }

  size_t ui_slot_() const {
    if (custom_logic::normal_ui_storage == nullptr) {
      return static_cast<size_t>(-1);
    }
    custom_logic::load_device_config_if_needed(this->slot_);
    if (custom_logic::cached_device_entity_id(this->slot_) != this->entity_id_) {
      return static_cast<size_t>(-1);
    }
    return custom_logic::normal_ui_slot_for_device_config_slot(this->slot_);
  }

  bool storage_ready_() const {
    return custom_logic::normal_ui_storage != nullptr && custom_logic::is_configurable_device_slot(this->slot_);
  }

  bool is_light_() const { return has_prefix(this->entity_id_, "light."); }
  bool is_cover_() const { return has_prefix(this->entity_id_, "cover."); }
  bool is_automation_() const { return has_prefix(this->entity_id_, "automation."); }

  int parse_int_(const std::string &value, int fallback) const {
    char *end = nullptr;
    const long parsed = strtol(value.c_str(), &end, 10);
    if (end == value.c_str()) {
      return fallback;
    }
    return static_cast<int>(parsed);
  }

  int kelvin_from_mired_(const std::string &value) const {
    char *end = nullptr;
    const float mired = strtof(value.c_str(), &end);
    if (end == value.c_str() || mired <= 0.0f) {
      return -1;
    }
    return static_cast<int>((1000000.0f / mired) + 0.5f);
  }

  bool defer_nonmatching_light_feedback_(size_t ui_slot, const std::string &field, const std::string &value) {
    if (!normal_light_feedback_window_active(ui_slot)) {
      return false;
    }
    auto &window = normal_light_feedback_windows[ui_slot];
    if (field == "state") {
      const bool matches_target = (window.target_power && value == "on") ||
                                  (!window.target_power && value == "off");
      if (matches_target) {
        window.pending_power = false;
        window.pending_state.clear();
        return false;
      }
      window.pending_power = true;
      window.pending_state = value;
      return true;
    }
    if (field == "brightness") {
      const int brightness_pct = custom_logic::brightness_255_to_percent(this->parse_int_(value, -1));
      if ((window.target_mask & custom_logic::NORMAL_LIGHT_FEEDBACK_BRIGHTNESS) != 0 &&
          brightness_pct == window.target_brightness_pct) {
        window.pending_brightness = false;
        window.pending_brightness_value.clear();
        return false;
      }
      window.pending_brightness = true;
      window.pending_brightness_value = value;
      return true;
    }
    if (field == "color_temp_kelvin") {
      const int kelvin = this->parse_int_(value, -1);
      if ((window.target_mask & custom_logic::NORMAL_LIGHT_FEEDBACK_COLOR_TEMPERATURE) != 0 &&
          kelvin == window.target_color_temperature_kelvin) {
        window.pending_color_temperature = false;
        window.pending_color_temperature_value.clear();
        return false;
      }
      window.pending_color_temperature = true;
      window.pending_color_temperature_value = value;
      return true;
    }
    if (field == "hs_color") {
      StaticJsonDocument<128> doc;
      int rgb_tick = -1;
      if (deserializeJson(doc, value) == DeserializationError::Ok && doc.is<JsonArray>()) {
        JsonArray hs = doc.as<JsonArray>();
        if (hs.size() >= 2) {
          int hue = static_cast<int>(hs[0].as<float>() + 0.5f);
          if (hue < 0) hue = 0;
          rgb_tick = ((hue % 360 + 5) / 10) % 36;
        }
      }
      if ((window.target_mask & custom_logic::NORMAL_LIGHT_FEEDBACK_RGB) != 0 &&
          rgb_tick == window.target_rgb_tick) {
        window.pending_rgb = false;
        window.pending_hs_color_value.clear();
        return false;
      }
      window.pending_rgb = true;
      window.pending_hs_color_value = value;
      return true;
    }
    if (field == "supported_color_modes") {
      window.pending_supported_color_modes = true;
      window.pending_supported_color_modes_value = value;
      return true;
    }
    if (field == "min_color_temp_kelvin") {
      window.pending_min_color_temp_kelvin = true;
      window.pending_min_color_temp_kelvin_value = value;
      return true;
    }
    if (field == "max_color_temp_kelvin") {
      window.pending_max_color_temp_kelvin = true;
      window.pending_max_color_temp_kelvin_value = value;
      return true;
    }
    if (field == "min_mireds") {
      window.pending_min_mireds = true;
      window.pending_min_mireds_value = value;
      return true;
    }
    if (field == "max_mireds") {
      window.pending_max_mireds = true;
      window.pending_max_mireds_value = value;
      return true;
    }
    return false;
  }

  void apply_power_or_cover_state_(const std::string &state) {
    if (state.empty() || state == "unknown" || state == "unavailable") {
      return;
    }
    if (this->is_cover_()) {
      const size_t ui_slot = this->ui_slot_();
      if (ui_slot == static_cast<size_t>(-1)) {
        return;
      }
      const uint32_t state_code = custom_logic::curtain_state_code_from_ha_state(state);
      custom_logic::normal_ui_storage->set_normal_ui_device_extra_u32(ui_slot, 2, state_code);
      // ESP_LOGI(TAG, "%s cover state stored: ui_slot=%u entity=%s state=%s state_code=%u",
      //          this->slot_label_().c_str(), static_cast<unsigned>(ui_slot + 1), this->entity_id_.c_str(),
      //          state.c_str(), static_cast<unsigned>(state_code));
      return;
    }

    const size_t ui_slot = this->ui_slot_();
    if (ui_slot == static_cast<size_t>(-1)) {
      return;
    }
    const bool is_on = state == "on";
    if (this->is_light_()) {
      esphome::onx_storage::NormalLightStatePatch patch{};
      patch.has_power = true;
      patch.is_on = is_on;
      custom_logic::normal_ui_storage->normal_device_slot(ui_slot).update_light(patch);
    } else {
      custom_logic::normal_ui_storage->set_normal_ui_device_state(ui_slot, is_on);
    }
    // ESP_LOGI(TAG, "%s power stored: ui_slot=%u entity=%s state=%s", this->slot_label_().c_str(),
    //          static_cast<unsigned>(ui_slot + 1), this->entity_id_.c_str(), is_on ? "ON" : "OFF");
  }

  void apply_brightness_(const std::string &value) {
    const int brightness_pct = custom_logic::brightness_255_to_percent(this->parse_int_(value, -1));
    if (brightness_pct < 0) {
      return;
    }
    const size_t ui_slot = this->ui_slot_();
    if (ui_slot == static_cast<size_t>(-1)) {
      return;
    }
    if (custom_logic::is_normal_light_feedback_suppressed(
            ui_slot, custom_logic::NORMAL_LIGHT_FEEDBACK_BRIGHTNESS)) {
      return;
    }
    esphome::onx_storage::NormalLightStatePatch patch{};
    patch.has_brightness_pct = true;
    patch.brightness_pct = static_cast<uint8_t>(std::clamp(brightness_pct, 0, 100));
    custom_logic::normal_ui_storage->normal_device_slot(ui_slot).update_light(patch);
    // ESP_LOGI(TAG, "%s brightness stored: ui_slot=%u entity=%s brightness_pct=%d", this->slot_label_().c_str(),
    //          static_cast<unsigned>(ui_slot + 1), this->entity_id_.c_str(), brightness_pct);
  }

  void apply_color_temperature_(const std::string &value) {
    int kelvin = this->parse_int_(value, -1);
    if (kelvin < 0) {
      return;
    }
    const size_t ui_slot = this->ui_slot_();
    if (ui_slot == static_cast<size_t>(-1)) {
      return;
    }
    if (custom_logic::is_normal_light_feedback_suppressed(
            ui_slot, custom_logic::NORMAL_LIGHT_FEEDBACK_COLOR_TEMPERATURE)) {
      return;
    }
    auto light_state = custom_logic::normal_ui_storage->normal_device_slot(ui_slot).light();
    int min_kelvin = static_cast<int>(light_state.min_color_temperature_kelvin);
    int max_kelvin = static_cast<int>(light_state.max_color_temperature_kelvin);
    if (min_kelvin > 0 && max_kelvin > 0 && min_kelvin > max_kelvin) {
      std::swap(min_kelvin, max_kelvin);
    }
    if (min_kelvin > 0 && max_kelvin > 0) {
      kelvin = std::clamp(kelvin, min_kelvin, max_kelvin);
    }
    esphome::onx_storage::NormalLightStatePatch patch{};
    patch.has_color_temperature_kelvin = true;
    patch.color_temperature_kelvin = static_cast<uint16_t>(std::clamp(kelvin, 0, 65535));
    custom_logic::normal_ui_storage->normal_device_slot(ui_slot).update_light(patch);
    // ESP_LOGI(TAG, "%s color temperature stored: ui_slot=%u entity=%s kelvin=%d", this->slot_label_().c_str(),
    //          static_cast<unsigned>(ui_slot + 1), this->entity_id_.c_str(), kelvin);
  }

  void apply_hs_color_(const std::string &value) {
    StaticJsonDocument<128> doc;
    if (deserializeJson(doc, value) != DeserializationError::Ok || !doc.is<JsonArray>()) {
      return;
    }
    JsonArray hs = doc.as<JsonArray>();
    if (hs.size() < 2) {
      return;
    }
    int hue = static_cast<int>(hs[0].as<float>() + 0.5f);
    int saturation = static_cast<int>(hs[1].as<float>() + 0.5f);
    if (hue < 0) {
      hue = 0;
    }
    if (hue >= 360) {
      hue %= 360;
    }
    saturation = std::clamp(saturation, 0, 100);
    const int rgb_tick = ((hue + 5) / 10) % 36;
    const size_t ui_slot = this->ui_slot_();
    if (ui_slot == static_cast<size_t>(-1)) {
      return;
    }
    if (custom_logic::is_normal_light_feedback_suppressed(
            ui_slot, custom_logic::NORMAL_LIGHT_FEEDBACK_RGB)) {
      return;
    }
    esphome::onx_storage::NormalLightStatePatch patch{};
    patch.has_rgb_tick = true;
    patch.rgb_tick = static_cast<uint8_t>(rgb_tick);
    custom_logic::normal_ui_storage->normal_device_slot(ui_slot).update_light(patch);
    // ESP_LOGI(TAG, "%s hs color stored: ui_slot=%u entity=%s hue=%d saturation=%d rgb_tick=%d",
    //          this->slot_label_().c_str(), static_cast<unsigned>(ui_slot + 1), this->entity_id_.c_str(), hue,
    //          saturation, rgb_tick);
  }

  void apply_cover_position_(const std::string &value) {
    const int position = std::clamp(this->parse_int_(value, -1), 0, 100);
    const size_t ui_slot = this->ui_slot_();
    if (ui_slot == static_cast<size_t>(-1)) {
      return;
    }
    custom_logic::normal_ui_storage->set_normal_ui_device_position(ui_slot, static_cast<uint8_t>(position));
    custom_logic::normal_ui_storage->set_normal_ui_device_extra_u32(ui_slot, 1, static_cast<uint32_t>(position));
    // ESP_LOGI(TAG, "%s cover position stored: ui_slot=%u entity=%s position=%d", this->slot_label_().c_str(),
    //          static_cast<unsigned>(ui_slot + 1), this->entity_id_.c_str(), position);
  }

  void apply_automation_last_triggered_(const std::string &value) {
    if (!this->is_automation_()) {
      return;
    }
    const size_t ui_slot = this->ui_slot_();
    if (ui_slot == static_cast<size_t>(-1) || ui_slot >= automation_last_triggered_values().size()) {
      return;
    }
    automation_last_triggered_entity_ids()[ui_slot] = this->entity_id_;
    automation_last_triggered_values()[ui_slot] = value;
    ESP_LOGI(TAG, "%s automation last_triggered updated: ui_slot=%u entity=%s value=%s",
             this->slot_label_().c_str(), static_cast<unsigned>(ui_slot + 1),
             this->entity_id_.c_str(), value.c_str());
  }

  void apply_supported_color_modes_(const std::string &value) {
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, value) != DeserializationError::Ok || !doc.is<JsonArray>()) {
      return;
    }
    uint32_t mask = custom_logic::infer_light_capability_mask(doc.as<JsonArrayConst>());
    const size_t ui_slot = this->ui_slot_();
    if (ui_slot == static_cast<size_t>(-1)) {
      return;
    }
    esphome::onx_storage::NormalLightStatePatch patch{};
    patch.has_capability_mask = true;
    patch.capability_mask = mask;
    custom_logic::normal_ui_storage->normal_device_slot(ui_slot).update_light(patch);
    // ESP_LOGI(TAG, "%s light capability stored: ui_slot=%u entity=%s mask=%u", this->slot_label_().c_str(),
    //          static_cast<unsigned>(ui_slot + 1), this->entity_id_.c_str(), static_cast<unsigned>(mask));
  }

  void apply_color_temp_range_(int min_kelvin, int max_kelvin) {
    if (!this->is_light_()) {
      return;
    }
    const size_t ui_slot = this->ui_slot_();
    if (ui_slot == static_cast<size_t>(-1)) {
      return;
    }
    if (min_kelvin < 0) {
      min_kelvin =
          static_cast<int>(custom_logic::normal_ui_storage->normal_device_slot(ui_slot).light().min_color_temperature_kelvin);
    }
    if (max_kelvin < 0) {
      max_kelvin =
          static_cast<int>(custom_logic::normal_ui_storage->normal_device_slot(ui_slot).light().max_color_temperature_kelvin);
    }
    uint16_t normalized_min = 0;
    uint16_t normalized_max = 0;
    custom_logic::normalize_kelvin_range_for_storage(min_kelvin, max_kelvin, &normalized_min, &normalized_max);
    esphome::onx_storage::NormalLightStatePatch patch{};
    patch.has_color_temperature_range = true;
    patch.min_color_temperature_kelvin = normalized_min;
    patch.max_color_temperature_kelvin = normalized_max;
    custom_logic::normal_ui_storage->normal_device_slot(ui_slot).update_light(patch);
    // ESP_LOGI(TAG, "%s color temp range stored: ui_slot=%u entity=%s min=%u max=%u", this->slot_label_().c_str(),
    //          static_cast<unsigned>(ui_slot + 1), this->entity_id_.c_str(), static_cast<unsigned>(normalized_min),
    //          static_cast<unsigned>(normalized_max));
  }

  size_t slot_;
  std::string entity_id_;
};

inline void flush_normal_light_feedback_window(size_t ui_slot, const std::string &entity_id) {
  if (ui_slot >= normal_light_feedback_windows.size() || normal_light_feedback_window_active(ui_slot)) {
    return;
  }
  DeviceSlotStateUpdater(ui_slot, entity_id).flush_deferred_light_feedback();
}

inline void subscribe_field(esphome::api::APIServer *api_server, size_t slot, const std::string &entity_id,
                            const std::string &field, const std::string &attribute) {
#ifdef USE_API_HOMEASSISTANT_STATES
  if (api_server == nullptr) {
    ESP_LOGW(TAG, "%s api server is null, skip HA entity subscription", custom_logic::device_slot_label(slot).c_str());
    return;
  }
  if (!is_valid_entity_id(entity_id)) {
    ESP_LOGI(TAG, "%s has no valid HA entity subscription", custom_logic::device_slot_label(slot).c_str());
    return;
  }
  api_server->subscribe_home_assistant_state(
      entity_id, esphome::optional<std::string>(attribute),
      [slot, entity_id, field](esphome::StringRef value) {
        DeviceSlotStateUpdater(slot, entity_id).on_state_changed(field, value);
      });
#else
  ESP_LOGW(TAG, "%s entity=%s field=%s skipped; api.homeassistant_states is disabled",
           custom_logic::device_slot_label(slot).c_str(), entity_id.c_str(), field.c_str());
#endif
}

inline void subscribe_entity(esphome::api::APIServer *api_server, size_t slot, const std::string &entity_id) {
  if (!is_valid_entity_id(entity_id)) {
    ESP_LOGI(TAG, "%s has no valid dynamic HA entity subscription", custom_logic::device_slot_label(slot).c_str());
    return;
  }

  subscribe_field(api_server, slot, entity_id, "state", "");

  if (has_prefix(entity_id, "light.")) {
    for (const char *attribute : light_attributes()) {
      subscribe_field(api_server, slot, entity_id, attribute, attribute);
    }
  } else if (has_prefix(entity_id, "cover.")) {
    for (const char *attribute : cover_attributes()) {
      subscribe_field(api_server, slot, entity_id, attribute, attribute);
    }
  } else if (has_prefix(entity_id, "automation.")) {
    for (const char *attribute : automation_attributes()) {
      subscribe_field(api_server, slot, entity_id, attribute, attribute);
    }
  }

  ESP_LOGI(TAG, "%s subscribed HA entity: %s", custom_logic::device_slot_label(slot).c_str(), entity_id.c_str());
}

inline void subscribe_standby_weather_field(esphome::api::APIServer *api_server, const std::string &entity_id,
                                            const std::string &field, const std::string &attribute,
                                            std::string *entity_id_out, std::string *state_out,
                                            std::string *temperature_out, std::string *temperature_unit_out,
                                            std::string *humidity_out) {
#ifdef USE_API_HOMEASSISTANT_STATES
  if (api_server == nullptr) {
    ESP_LOGW(TAG, "api server is null, skip standby weather HA subscription");
    return;
  }
  if (!is_valid_entity_id(entity_id) || !has_prefix(entity_id, "weather.")) {
    ESP_LOGI(TAG, "standby weather has no valid HA entity subscription");
    return;
  }
  api_server->subscribe_home_assistant_state(
      entity_id, esphome::optional<std::string>(attribute),
      [entity_id, field, entity_id_out, state_out, temperature_out, temperature_unit_out, humidity_out](
          esphome::StringRef raw_value) {
        const std::string value = raw_value.str();
        // ESP_LOGI(TAG, "standby_weather entity=%s field=%s value=%s", entity_id.c_str(), field.c_str(), value.c_str());
        if (entity_id_out != nullptr) {
          *entity_id_out = entity_id;
        }
        if (field == "state" && state_out != nullptr) {
          *state_out = value;
        } else if (field == "temperature" && temperature_out != nullptr) {
          *temperature_out = value.empty() ? "unknown" : value;
        } else if (field == "temperature_unit" && temperature_unit_out != nullptr) {
          *temperature_unit_out = value;
        } else if (field == "humidity" && humidity_out != nullptr) {
          *humidity_out = value.empty() ? "unknown" : value;
        }
      });
#else
  ESP_LOGW(TAG, "standby weather entity=%s field=%s skipped; api.homeassistant_states is disabled",
           entity_id.c_str(), field.c_str());
#endif
}

inline void subscribe_standby_weather_entity(esphome::api::APIServer *api_server, const std::string &entity_id,
                                             std::string *entity_id_out, std::string *state_out,
                                             std::string *temperature_out, std::string *temperature_unit_out,
                                             std::string *humidity_out) {
  if (!is_valid_entity_id(entity_id) || !has_prefix(entity_id, "weather.")) {
    ESP_LOGI(TAG, "standby weather has no valid dynamic HA entity subscription");
    return;
  }

  subscribe_standby_weather_field(api_server, entity_id, "state", "", entity_id_out, state_out, temperature_out,
                                  temperature_unit_out, humidity_out);
  for (const char *attribute : weather_attributes()) {
    subscribe_standby_weather_field(api_server, entity_id, attribute, attribute, entity_id_out, state_out,
                                    temperature_out, temperature_unit_out, humidity_out);
  }
  ESP_LOGI(TAG, "standby weather subscribed HA entity: %s", entity_id.c_str());
}

inline void subscribe_standby_weather_config_entity_from_select(esphome::select::Select *standby_weather_select,
                                                                esphome::api::APIServer *api_server,
                                                                std::string *entity_id_out,
                                                                std::string *state_out,
                                                                std::string *temperature_out,
                                                                std::string *temperature_unit_out,
                                                                std::string *humidity_out) {
  subscribe_standby_weather_entity(api_server, entity_id_from_select(standby_weather_select), entity_id_out, state_out,
                                   temperature_out, temperature_unit_out, humidity_out);
}

inline void subscribe_device_config_entities_from_selects(esphome::select::Select *device1_select,
                                                          esphome::select::Select *device2_select,
                                                          esphome::api::APIServer *api_server) {
  subscribe_entity(api_server, 0, entity_id_from_select(device1_select));
  subscribe_entity(api_server, 1, entity_id_from_select(device2_select));
}

inline void subscribe_device_config_entities_from_selects(esphome::select::Select *device1_select,
                                                          esphome::select::Select *device2_select,
                                                          esphome::select::Select *device3_select,
                                                          esphome::select::Select *device4_select,
                                                          esphome::select::Select *device5_select,
                                                          esphome::select::Select *device6_select,
                                                          esphome::select::Select *device7_select,
                                                          esphome::select::Select *device8_select,
                                                          esphome::select::Select *device9_select,
                                                          esphome::select::Select *device10_select,
                                                          esphome::api::APIServer *api_server) {
  subscribe_entity(api_server, 0, entity_id_from_select(device1_select));
  subscribe_entity(api_server, 1, entity_id_from_select(device2_select));
  subscribe_entity(api_server, 2, entity_id_from_select(device3_select));
  subscribe_entity(api_server, 3, entity_id_from_select(device4_select));
  subscribe_entity(api_server, 4, entity_id_from_select(device5_select));
  subscribe_entity(api_server, 5, entity_id_from_select(device6_select));
  subscribe_entity(api_server, 6, entity_id_from_select(device7_select));
  subscribe_entity(api_server, 7, entity_id_from_select(device8_select));
  subscribe_entity(api_server, 8, entity_id_from_select(device9_select));
  subscribe_entity(api_server, 9, entity_id_from_select(device10_select));
}

inline void subscribe_device_config_entities_from_selects(esphome::select::Select *device1_select,
                                                          esphome::select::Select *device2_select,
                                                          esphome::select::Select *standby_weather_select,
                                                          esphome::api::APIServer *api_server,
                                                          std::string *weather_entity_id_out,
                                                          std::string *weather_state_out,
                                                          std::string *weather_temperature_out,
                                                          std::string *weather_temperature_unit_out,
                                                          std::string *weather_humidity_out) {
  subscribe_device_config_entities_from_selects(device1_select, device2_select, api_server);
  subscribe_standby_weather_config_entity_from_select(standby_weather_select, api_server, weather_entity_id_out,
                                                      weather_state_out, weather_temperature_out,
                                                      weather_temperature_unit_out, weather_humidity_out);
}

inline void subscribe_device_config_entities_from_selects(esphome::select::Select *device1_select,
                                                          esphome::select::Select *device2_select,
                                                          esphome::select::Select *device3_select,
                                                          esphome::select::Select *device4_select,
                                                          esphome::select::Select *device5_select,
                                                          esphome::select::Select *device6_select,
                                                          esphome::select::Select *device7_select,
                                                          esphome::select::Select *device8_select,
                                                          esphome::select::Select *device9_select,
                                                          esphome::select::Select *device10_select,
                                                          esphome::select::Select *standby_weather_select,
                                                          esphome::api::APIServer *api_server,
                                                          std::string *weather_entity_id_out,
                                                          std::string *weather_state_out,
                                                          std::string *weather_temperature_out,
                                                          std::string *weather_temperature_unit_out,
                                                          std::string *weather_humidity_out) {
  subscribe_device_config_entities_from_selects(device1_select, device2_select, device3_select, device4_select,
                                                device5_select, device6_select, device7_select, device8_select,
                                                device9_select, device10_select, api_server);
  subscribe_standby_weather_config_entity_from_select(standby_weather_select, api_server, weather_entity_id_out,
                                                      weather_state_out, weather_temperature_out,
                                                      weather_temperature_unit_out, weather_humidity_out);
}

}  // namespace custom_logic_entity_sub

inline void subscribe_device_config_entities_from_selects(esphome::select::Select *device1_select,
                                                          esphome::select::Select *device2_select,
                                                          esphome::api::APIServer *api_server) {
  custom_logic_entity_sub::subscribe_device_config_entities_from_selects(device1_select, device2_select, api_server);
}

inline void subscribe_device_config_entities_from_selects(esphome::select::Select *device1_select,
                                                          esphome::select::Select *device2_select,
                                                          esphome::select::Select *device3_select,
                                                          esphome::select::Select *device4_select,
                                                          esphome::select::Select *device5_select,
                                                          esphome::select::Select *device6_select,
                                                          esphome::select::Select *device7_select,
                                                          esphome::select::Select *device8_select,
                                                          esphome::select::Select *device9_select,
                                                          esphome::select::Select *device10_select,
                                                          esphome::api::APIServer *api_server) {
  custom_logic_entity_sub::subscribe_device_config_entities_from_selects(device1_select, device2_select, device3_select,
                                                                         device4_select, device5_select, device6_select,
                                                                         device7_select, device8_select, device9_select,
                                                                         device10_select, api_server);
}

inline void subscribe_device_config_entities_from_selects(esphome::select::Select *device1_select,
                                                          esphome::select::Select *device2_select,
                                                          esphome::select::Select *standby_weather_select,
                                                          esphome::api::APIServer *api_server,
                                                          std::string *weather_entity_id_out,
                                                          std::string *weather_state_out,
                                                          std::string *weather_temperature_out,
                                                          std::string *weather_temperature_unit_out,
                                                          std::string *weather_humidity_out) {
  custom_logic_entity_sub::subscribe_device_config_entities_from_selects(
      device1_select, device2_select, standby_weather_select, api_server, weather_entity_id_out, weather_state_out,
      weather_temperature_out, weather_temperature_unit_out, weather_humidity_out);
}

inline void subscribe_device_config_entities_from_selects(esphome::select::Select *device1_select,
                                                          esphome::select::Select *device2_select,
                                                          esphome::select::Select *device3_select,
                                                          esphome::select::Select *device4_select,
                                                          esphome::select::Select *device5_select,
                                                          esphome::select::Select *device6_select,
                                                          esphome::select::Select *device7_select,
                                                          esphome::select::Select *device8_select,
                                                          esphome::select::Select *device9_select,
                                                          esphome::select::Select *device10_select,
                                                          esphome::select::Select *standby_weather_select,
                                                          esphome::api::APIServer *api_server,
                                                          std::string *weather_entity_id_out,
                                                          std::string *weather_state_out,
                                                          std::string *weather_temperature_out,
                                                          std::string *weather_temperature_unit_out,
                                                          std::string *weather_humidity_out) {
  custom_logic_entity_sub::subscribe_device_config_entities_from_selects(
      device1_select, device2_select, device3_select, device4_select, device5_select, device6_select, device7_select,
      device8_select, device9_select, device10_select, standby_weather_select, api_server, weather_entity_id_out,
      weather_state_out, weather_temperature_out, weather_temperature_unit_out, weather_humidity_out);
}
