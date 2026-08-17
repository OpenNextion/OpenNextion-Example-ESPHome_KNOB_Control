#include "onx_mode_manager.h"

#include <cctype>

#include "esphome/components/captive_portal/captive_portal.h"
#include "esphome/components/logger/logger.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#ifdef USE_ESP32
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs.h"
#endif

namespace esphome::onx_mode_manager {

static const char *const TAG = "onx_mode_manager";
static const uint32_t NORMAL_WIFI_RECOVERY_DELAY_MS = 3000;
// ESPHome restored global `normal_ha_added`: 1944399030 ^ 3371913747.
static const char *const NORMAL_HA_ADDED_PREF_KEY = "3139337893";

static std::string build_provisioning_ap_ssid_() {
  std::string ssid = "ONX2424G013-";
  std::string mac = get_mac_address();
  for (char &ch : mac) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  ssid += mac.size() > 4 ? mac.substr(mac.size() - 4) : mac;
  return ssid;
}

void OnxModeManager::setup() {
  this->mode_decided_ = false;
  this->missing_binding_logged_ = false;
  this->boot_finished_ = false;
  this->provisioning_mode_enabled_ = false;
  this->normal_wifi_connect_scheduled_ = false;
  if (this->app_state_ != nullptr) {
    this->app_state_->set_current_mode(onx_app_state::OnxRuntimeMode::MODE_UNKNOWN);
    this->app_state_->set_mode_reason("booting");
    this->app_state_->mark_ui_dirty();
  }
}

void OnxModeManager::loop() {
  if (!this->mode_decided_) {
    this->evaluate_boot_mode_();
  } else {
    this->evaluate_runtime_mode_();
  }
}

void OnxModeManager::dump_config() {
  ESP_LOGCONFIG(TAG, "ONX Mode Manager:");
  ESP_LOGCONFIG(TAG, "  App state bound: %s", this->app_state_ != nullptr ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Mode decided: %s", this->mode_decided_ ? "YES" : "NO");
}

float OnxModeManager::get_setup_priority() const {
  // Keep this late so the device graph is fully initialized before boot arbitration runs.
  return setup_priority::LATE;
}

void OnxModeManager::set_app_state(onx_app_state::OnxAppState *app_state) { this->app_state_ = app_state; }
void OnxModeManager::set_normal_wifi_ssid(const std::string &normal_wifi_ssid) {
  this->normal_wifi_ssid_ = normal_wifi_ssid;
}
void OnxModeManager::set_normal_wifi_password(const std::string &normal_wifi_password) {
  this->normal_wifi_password_ = normal_wifi_password;
}
void OnxModeManager::set_runtime_log_level(uint8_t level) { this->runtime_log_level_ = level; }
void OnxModeManager::mark_boot_finished() { this->boot_finished_ = true; }

void OnxModeManager::evaluate_boot_mode_() {
  if (this->app_state_ == nullptr) {
    if (!this->missing_binding_logged_) {
      ESP_LOGW(TAG, "Mode manager bindings are incomplete; waiting for dependencies");
      this->missing_binding_logged_ = true;
    }
    return;
  }

  if (!this->boot_finished_) {
    this->app_state_->set_current_mode(onx_app_state::OnxRuntimeMode::MODE_UNKNOWN);
    this->app_state_->set_mode_reason("booting");
    this->app_state_->mark_ui_dirty();
    return;
  }

  this->select_mode_(onx_app_state::OnxRuntimeMode::MODE_NORMAL, "normal-mode-only");
}

void OnxModeManager::evaluate_runtime_mode_() {
  if (this->app_state_ == nullptr) {
    return;
  }

  if (this->app_state_->get_current_mode() == onx_app_state::OnxRuntimeMode::MODE_NORMAL &&
      this->provisioning_mode_enabled_ && this->has_saved_normal_wifi_()) {
    this->disable_provisioning_mode_();
    const wifi::WiFiAP saved_sta = wifi::global_wifi_component->get_sta();
    this->schedule_normal_wifi_connect_(saved_sta.get_ssid().c_str(), saved_sta.get_password().c_str(), true);
    ESP_LOGI(TAG, "Normal Wi-Fi has been provisioned; AP provisioning is now disabled");
    return;
  }
}

void OnxModeManager::disable_provisioning_mode_() {
#ifdef USE_CAPTIVE_PORTAL
  if (captive_portal::global_captive_portal != nullptr && captive_portal::global_captive_portal->is_active()) {
    captive_portal::global_captive_portal->end();
    ESP_LOGI(TAG, "Provisioning captive portal disabled for current mode");
  }
#endif

#ifdef USE_ESP32
  esp_err_t err = esp_wifi_set_mode(WIFI_MODE_STA);
  if (err == ESP_OK) {
    ESP_LOGI(TAG, "Provisioning softAP disabled; Wi-Fi forced to STA-only mode");
  } else {
    ESP_LOGW(TAG, "Failed to force Wi-Fi into STA-only mode: %s", esp_err_to_name(err));
  }
#endif
  this->provisioning_mode_enabled_ = false;
}

void OnxModeManager::enable_provisioning_mode_() {
  wifi::WiFiAP ap{};
  if (wifi::global_wifi_component != nullptr) {
    ap = wifi::global_wifi_component->get_ap();
    const std::string ssid = build_provisioning_ap_ssid_();
    ap.set_ssid(ssid);
    wifi::global_wifi_component->set_ap(ap);
    ESP_LOGI(TAG, "Provisioning softAP SSID set to '%s'", ssid.c_str());
  }
#ifdef USE_ESP32
  esp_err_t err = esp_wifi_set_mode(WIFI_MODE_APSTA);
  if (err == ESP_OK) {
    ESP_LOGI(TAG, "Provisioning softAP enabled; Wi-Fi set to AP+STA mode");
  } else {
    ESP_LOGW(TAG, "Failed to set Wi-Fi AP+STA mode: %s", esp_err_to_name(err));
  }
  if (wifi::global_wifi_component != nullptr) {
    wifi_config_t conf;
    memset(&conf, 0, sizeof(conf));
    const std::string ap_ssid = ap.get_ssid().str();
    memcpy(reinterpret_cast<char *>(conf.ap.ssid), ap_ssid.c_str(), ap_ssid.size());
    conf.ap.channel = ap.has_channel() ? ap.get_channel() : 1;
    conf.ap.ssid_hidden = ap.get_hidden();
    conf.ap.max_connection = 5;
    conf.ap.beacon_interval = 100;

    const std::string ap_password = ap.get_password().str();
    if (ap_password.empty()) {
      conf.ap.authmode = WIFI_AUTH_OPEN;
      *conf.ap.password = 0;
    } else {
      conf.ap.authmode = WIFI_AUTH_WPA2_PSK;
      memcpy(reinterpret_cast<char *>(conf.ap.password), ap_password.c_str(), ap_password.size());
    }
    conf.ap.pairwise_cipher = WIFI_CIPHER_TYPE_CCMP;

    err = esp_wifi_set_config(WIFI_IF_AP, &conf);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "Provisioning softAP config applied");
    } else {
      ESP_LOGW(TAG, "Failed to apply provisioning softAP config: %s", esp_err_to_name(err));
    }
  }
#endif

#ifdef USE_CAPTIVE_PORTAL
  if (captive_portal::global_captive_portal != nullptr && !captive_portal::global_captive_portal->is_active()) {
    captive_portal::global_captive_portal->start();
    ESP_LOGI(TAG, "Provisioning captive portal enabled for normal mode");
  }
#endif
  this->provisioning_mode_enabled_ = true;
}

bool OnxModeManager::has_saved_normal_wifi_() const {
  if (wifi::global_wifi_component == nullptr || !wifi::global_wifi_component->has_sta()) {
    return false;
  }
  return !wifi::global_wifi_component->get_sta().get_ssid().empty();
}

bool OnxModeManager::clear_saved_normal_wifi_prefs_() {
#ifdef USE_ESP32
  nvs_handle_t handle;
  esp_err_t err = nvs_open("esphome", NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "normal_wifi_recover: failed to open NVS namespace for erase: %s", esp_err_to_name(err));
    return false;
  }

  bool ok = true;
  const char *const keys[] = {"88491487", "88491488", NORMAL_HA_ADDED_PREF_KEY};
  for (const char *key : keys) {
    err = nvs_erase_key(handle, key);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "normal_wifi_recover: erased NVS key '%s'", key);
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
      ESP_LOGI(TAG, "normal_wifi_recover: NVS key '%s' already absent", key);
    } else {
      ESP_LOGW(TAG, "normal_wifi_recover: failed to erase NVS key '%s': %s", key, esp_err_to_name(err));
      ok = false;
    }
  }

  err = nvs_commit(handle);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "normal_wifi_recover: failed to commit NVS erase: %s", esp_err_to_name(err));
    ok = false;
  }
  nvs_close(handle);
  return ok;
#else
  return false;
#endif
}

void OnxModeManager::schedule_normal_wifi_connect_(const std::string &ssid, const std::string &password,
                                                   bool use_explicit_credentials) {
  if (wifi::global_wifi_component == nullptr) {
    ESP_LOGW(TAG, "normal_wifi_recover: Wi-Fi component missing; cannot schedule normal Wi-Fi connect");
    return;
  }

  if (this->normal_wifi_connect_scheduled_) {
    ESP_LOGI(TAG, "normal_wifi_recover: connect already scheduled; skip duplicate request for SSID '%s'", ssid.c_str());
    return;
  }

  this->normal_wifi_connect_scheduled_ = true;
  ESP_LOGI(TAG,
           "normal_wifi_recover: reset_reason=%s boot_ms=%u target_ssid='%s' explicit_credentials=%s; disabling Wi-Fi "
           "adapter before delayed connect",
           this->get_reset_reason_name_(), static_cast<unsigned int>(millis()), ssid.c_str(),
           use_explicit_credentials ? "YES" : "NO");
  wifi::global_wifi_component->disable();

  this->set_timeout("normal_wifi_recover_connect", NORMAL_WIFI_RECOVERY_DELAY_MS,
                    [this, ssid, password, use_explicit_credentials]() {
                      if (wifi::global_wifi_component == nullptr) {
                        ESP_LOGW(TAG, "normal_wifi_recover: delayed connect skipped; Wi-Fi component missing");
                        this->normal_wifi_connect_scheduled_ = false;
                        return;
                      }

                      ESP_LOGI(TAG, "normal_wifi_recover: delay elapsed (%ums); enabling STA for SSID '%s'",
                               static_cast<unsigned int>(NORMAL_WIFI_RECOVERY_DELAY_MS), ssid.c_str());
                      if (use_explicit_credentials) {
                        wifi::WiFiAP target_ap{};
                        target_ap.set_ssid(ssid);
                        target_ap.set_password(password);
                        wifi::global_wifi_component->set_sta(target_ap);
                        this->applied_wifi_ssid_ = ssid;
                        this->applied_wifi_password_ = password;
                        ESP_LOGI(TAG, "normal_wifi_recover: explicit STA credentials applied for SSID '%s'",
                                 ssid.c_str());
                      }

                      wifi::global_wifi_component->enable();
                      this->normal_wifi_connect_scheduled_ = false;
                      ESP_LOGI(TAG, "normal_wifi_recover: Wi-Fi adapter enabled; ESPHome Wi-Fi state machine owns SSID '%s'",
                               ssid.c_str());
                    });
}

const char *OnxModeManager::get_reset_reason_name_() const {
#ifdef USE_ESP32
  switch (esp_reset_reason()) {
    case ESP_RST_UNKNOWN:
      return "UNKNOWN";
    case ESP_RST_POWERON:
      return "POWERON";
    case ESP_RST_EXT:
      return "EXT";
    case ESP_RST_SW:
      return "SW";
    case ESP_RST_PANIC:
      return "PANIC";
    case ESP_RST_INT_WDT:
      return "INT_WDT";
    case ESP_RST_TASK_WDT:
      return "TASK_WDT";
    case ESP_RST_WDT:
      return "WDT";
    case ESP_RST_DEEPSLEEP:
      return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:
      return "BROWNOUT";
    case ESP_RST_SDIO:
      return "SDIO";
    case ESP_RST_USB:
      return "USB";
    case ESP_RST_JTAG:
      return "JTAG";
    case ESP_RST_EFUSE:
      return "EFUSE";
    case ESP_RST_PWR_GLITCH:
      return "PWR_GLITCH";
    case ESP_RST_CPU_LOCKUP:
      return "CPU_LOCKUP";
    default:
      return "OTHER";
  }
#else
  return "UNSUPPORTED";
#endif
}

void OnxModeManager::apply_wifi_for_mode_(onx_app_state::OnxRuntimeMode mode) {
  if (wifi::global_wifi_component == nullptr) {
    ESP_LOGW(TAG, "Wi-Fi component missing; cannot apply mode-specific Wi-Fi");
    return;
  }

  std::string target_ssid;
  std::string target_password;
  if (mode == onx_app_state::OnxRuntimeMode::MODE_NORMAL) {
    target_ssid = this->normal_wifi_ssid_;
    target_password = this->normal_wifi_password_;
    if (target_ssid.empty()) {
      if (this->has_saved_normal_wifi_()) {
        this->disable_provisioning_mode_();
        const wifi::WiFiAP saved_sta = wifi::global_wifi_component->get_sta();
        this->schedule_normal_wifi_connect_(saved_sta.get_ssid().c_str(), saved_sta.get_password().c_str(), true);
        ESP_LOGI(TAG, "Normal mode selected with saved Wi-Fi; AP provisioning disabled");
        return;
      }
      this->enable_provisioning_mode_();
      ESP_LOGI(TAG, "Normal mode selected with no configured Wi-Fi; AP provisioning enabled once");
      return;
    }
    this->disable_provisioning_mode_();
  } else {
    return;
  }

  const bool credentials_changed =
      target_ssid != this->applied_wifi_ssid_ || target_password != this->applied_wifi_password_;
  if (!credentials_changed) {
    this->schedule_normal_wifi_connect_(target_ssid, target_password, true);
    ESP_LOGI(TAG, "Scheduled Wi-Fi reconnect for mode %s on SSID '%s'", this->app_state_->get_current_mode_name(),
             target_ssid.c_str());
    return;
  }

  this->schedule_normal_wifi_connect_(target_ssid, target_password, true);

  ESP_LOGI(TAG, "Scheduled mode Wi-Fi for %s: SSID '%s'", this->app_state_->get_current_mode_name(),
           target_ssid.c_str());
}

void OnxModeManager::select_mode_(onx_app_state::OnxRuntimeMode mode, const std::string &reason) {
  if (this->mode_decided_ || this->app_state_ == nullptr) {
    return;
  }

  this->app_state_->set_current_mode(mode);
  this->app_state_->set_mode_reason(reason);
  this->app_state_->mark_ui_dirty();
  this->mode_decided_ = true;
  this->apply_wifi_for_mode_(mode);

  if (logger::global_logger != nullptr) {
    logger::global_logger->set_log_level(this->runtime_log_level_);
  }

  ESP_LOGI(TAG, "Boot mode selected: %s (%s)", this->app_state_->get_current_mode_name(), reason.c_str());
}

}  // namespace esphome::onx_mode_manager
