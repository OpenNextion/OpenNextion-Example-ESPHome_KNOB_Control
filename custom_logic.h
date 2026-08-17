#pragma once
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <memory>
#include <new>
#include <string>

#include <ArduinoJson.h>
#include "esphome.h"
#include "esphome/components/api/api_server.h"
#include "esphome/components/json/json_util.h"
#include "esphome/core/helpers.h"
#include "esphome/core/preferences.h"
#include "esphome/components/onx_storage/onx_storage.h"

#ifdef USE_ESP32
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_websocket_client.h"
#include "nvs.h"
#endif

namespace custom_logic {

static const char *const TAG = "CustomLogic";
static constexpr size_t MAX_HA_TOKEN_LENGTH = 255;
static constexpr size_t MAX_HA_ENTITY_COUNT = 200;
static constexpr size_t LOCAL_HMI_GPIO_SWITCH_PRESET_OPTION_COUNT = 4;
static constexpr size_t MAX_HA_ENTITY_OPTION_COUNT =
    MAX_HA_ENTITY_COUNT + 1 + LOCAL_HMI_GPIO_SWITCH_PRESET_OPTION_COUNT;
static constexpr size_t CONFIGURABLE_DEVICE_SLOT_COUNT = 10;
enum NormalDeviceDetailKind : int {
  NORMAL_DEVICE_DETAIL_KIND_NONE = 0,
  NORMAL_DEVICE_DETAIL_KIND_SWITCH = 1,
  NORMAL_DEVICE_DETAIL_KIND_LIGHT = 2,
  NORMAL_DEVICE_DETAIL_KIND_PLUG = 3,
  NORMAL_DEVICE_DETAIL_KIND_CURTAIN = 4,
  NORMAL_DEVICE_DETAIL_KIND_SETTINGS = 5,
  NORMAL_DEVICE_DETAIL_KIND_SETTINGS_WIFI = 6,
  NORMAL_DEVICE_DETAIL_KIND_DIMMING_LIGHT = 7,
  NORMAL_DEVICE_DETAIL_KIND_SETTINGS_BRIGHTNESS = 8,
  NORMAL_DEVICE_DETAIL_KIND_SETTINGS_KNOB = 9,
  NORMAL_DEVICE_DETAIL_KIND_AUTOMATION = 10,
  NORMAL_DEVICE_DETAIL_KIND_SETTINGS_USER_MANUAL = 11,
  NORMAL_DEVICE_DETAIL_KIND_SETTINGS_USER_MANUAL_QR_CODE = 12,
};
static constexpr uint8_t NORMAL_LIGHT_FEEDBACK_BRIGHTNESS = 0x01;
static constexpr uint8_t NORMAL_LIGHT_FEEDBACK_COLOR_TEMPERATURE = 0x02;
static constexpr uint8_t NORMAL_LIGHT_FEEDBACK_RGB = 0x04;
static const char *const LOCAL_HMI_GPIO_SWITCH_PRESET_OPTIONS[LOCAL_HMI_GPIO_SWITCH_PRESET_OPTION_COUNT] = {
    "[Local] HMI IO10",
    "[Local] HMI IO11",
    "[Local] HMI IO17",
    "[Local] HMI IO18",
};
static std::array<uint8_t, CONFIGURABLE_DEVICE_SLOT_COUNT> normal_light_feedback_suppression_masks{};

inline void set_normal_light_feedback_suppressed(size_t ui_slot, uint8_t attribute_mask, bool suppressed) {
  if (ui_slot >= normal_light_feedback_suppression_masks.size()) {
    return;
  }
  if (suppressed) {
    normal_light_feedback_suppression_masks[ui_slot] |= attribute_mask;
  } else {
    normal_light_feedback_suppression_masks[ui_slot] &= static_cast<uint8_t>(~attribute_mask);
  }
}

inline bool is_normal_light_feedback_suppressed(size_t ui_slot, uint8_t attribute_mask) {
  return ui_slot < normal_light_feedback_suppression_masks.size() &&
         (normal_light_feedback_suppression_masks[ui_slot] & attribute_mask) != 0;
}

static constexpr size_t HA_ENTITY_PREF_CHUNK_SIZE = 20;
static constexpr size_t HA_ENTITY_PREF_CHUNK_COUNT =
    (MAX_HA_ENTITY_COUNT + HA_ENTITY_PREF_CHUNK_SIZE - 1) / HA_ENTITY_PREF_CHUNK_SIZE;
static constexpr size_t MAX_HA_ENTITY_ID_LENGTH = 128;
static constexpr size_t MAX_HA_FRIENDLY_NAME_LENGTH = 64;
static constexpr size_t MAX_HA_AREA_COUNT = 32;
static constexpr size_t MAX_HA_AREA_ID_LENGTH = 64;
static constexpr size_t MAX_HA_AREA_NAME_LENGTH = 64;
static constexpr size_t HA_WS_CLIENT_BUFFER_SIZE = 16 * 1024;
static constexpr size_t HA_WS_RX_RESERVE_SIZE = HA_WS_CLIENT_BUFFER_SIZE * 2;
static constexpr size_t MAX_HA_WS_TEXT_MESSAGE_LENGTH = 2 * 1024 * 1024;
static constexpr int HA_WS_NETWORK_TIMEOUT_MS = 30000;
using PsramString = std::basic_string<char, std::char_traits<char>, esphome::RAMAllocator<char>>;
using HaEntityOptionArray = std::array<std::string, MAX_HA_ENTITY_OPTION_COUNT>;
template<typename T> struct RamAllocatedDeleter {
  void operator()(T *ptr) const {
    if (ptr == nullptr) {
      return;
    }
    ptr->~T();
    esphome::RAMAllocator<T> allocator;
    allocator.deallocate(ptr, 1);
  }
};
template<typename T> using RamAllocatedPtr = std::unique_ptr<T, RamAllocatedDeleter<T>>;
template<typename T> inline RamAllocatedPtr<T> make_ram_allocated() {
  esphome::RAMAllocator<T> allocator;
  T *ptr = allocator.allocate(1);
  if (ptr == nullptr) {
    return RamAllocatedPtr<T>(nullptr);
  }
  new (ptr) T();
  return RamAllocatedPtr<T>(ptr);
}
static constexpr uint32_t HA_TOKEN_MAGIC = 0x4F4E5854;  // ONXT
static constexpr uint32_t HA_ENTITIES_MAGIC = 0x4F4E5849;  // ONXI
static constexpr uint32_t HA_ENTITY_CHUNK_MAGIC = 0x4F4E5843;  // ONXC
static constexpr uint32_t HA_AREAS_MAGIC = 0x4F4E5841;  // ONXA
static constexpr uint32_t DEVICE1_CONFIG_MAGIC = 0x4F4E5831;  // ONX1
static constexpr uint32_t DEVICE2_CONFIG_MAGIC = 0x4F4E5832;  // ONX2
static constexpr uint32_t DEVICE3_CONFIG_MAGIC = 0x4F4E5833;  // ONX3
static constexpr uint32_t DEVICE4_CONFIG_MAGIC = 0x4F4E5834;  // ONX4
static constexpr uint32_t DEVICE5_CONFIG_MAGIC = 0x4F4E5835;  // ONX5
static constexpr uint32_t DEVICE6_CONFIG_MAGIC = 0x4F4E5836;  // ONX6
static constexpr uint32_t DEVICE7_CONFIG_MAGIC = 0x4F4E5837;  // ONX7
static constexpr uint32_t DEVICE8_CONFIG_MAGIC = 0x4F4E5838;  // ONX8
static constexpr uint32_t DEVICE9_CONFIG_MAGIC = 0x4F4E5839;  // ONX9
static constexpr uint32_t DEVICE10_CONFIG_MAGIC = 0x4F4E5841;  // ONXA
static constexpr uint32_t STANDBY_WEATHER_CONFIG_MAGIC = 0x4F4E5857;  // ONXW
static const uint32_t HA_TOKEN_PREF_KEY = esphome::fnv1_hash("ha-long-lived-token-v1");
static const uint32_t HA_ENTITY_CHUNK_PREF_KEY_BASE = esphome::fnv1_hash("ha-entities-v4-chunk");
static const uint32_t HA_AREAS_PREF_KEY = esphome::fnv1_hash("ha-areas-v1");
static const uint32_t DEVICE1_CONFIG_PREF_KEY = esphome::fnv1_hash("device1-config-v1");
static const uint32_t DEVICE2_CONFIG_PREF_KEY = esphome::fnv1_hash("device2-config-v1");
static const uint32_t DEVICE3_CONFIG_PREF_KEY = esphome::fnv1_hash("device3-config-v1");
static const uint32_t DEVICE4_CONFIG_PREF_KEY = esphome::fnv1_hash("device4-config-v1");
static const uint32_t DEVICE5_CONFIG_PREF_KEY = esphome::fnv1_hash("device5-config-v1");
static const uint32_t DEVICE6_CONFIG_PREF_KEY = esphome::fnv1_hash("device6-config-v1");
static const uint32_t DEVICE7_CONFIG_PREF_KEY = esphome::fnv1_hash("device7-config-v1");
static const uint32_t DEVICE8_CONFIG_PREF_KEY = esphome::fnv1_hash("device8-config-v1");
static const uint32_t DEVICE9_CONFIG_PREF_KEY = esphome::fnv1_hash("device9-config-v1");
static const uint32_t DEVICE10_CONFIG_PREF_KEY = esphome::fnv1_hash("device10-config-v1");
static const uint32_t STANDBY_WEATHER_CONFIG_PREF_KEY = esphome::fnv1_hash("standby-weather-config-v1");
static const uint32_t DEVICE1_LIGHT_RUNTIME_PREF_KEY = esphome::fnv1_hash("device1-light-runtime-v1");
static const uint32_t DEVICE2_LIGHT_RUNTIME_PREF_KEY = esphome::fnv1_hash("device2-light-runtime-v1");
static const uint32_t DEVICE3_LIGHT_RUNTIME_PREF_KEY = esphome::fnv1_hash("device3-light-runtime-v1");
static const uint32_t DEVICE4_LIGHT_RUNTIME_PREF_KEY = esphome::fnv1_hash("device4-light-runtime-v1");
static const uint32_t DEVICE5_LIGHT_RUNTIME_PREF_KEY = esphome::fnv1_hash("device5-light-runtime-v1");
static const uint32_t DEVICE6_LIGHT_RUNTIME_PREF_KEY = esphome::fnv1_hash("device6-light-runtime-v1");
static const uint32_t DEVICE7_LIGHT_RUNTIME_PREF_KEY = esphome::fnv1_hash("device7-light-runtime-v1");
static const uint32_t DEVICE8_LIGHT_RUNTIME_PREF_KEY = esphome::fnv1_hash("device8-light-runtime-v1");
static const uint32_t DEVICE9_LIGHT_RUNTIME_PREF_KEY = esphome::fnv1_hash("device9-light-runtime-v1");
static const uint32_t DEVICE10_LIGHT_RUNTIME_PREF_KEY = esphome::fnv1_hash("device10-light-runtime-v1");
static const uint32_t HA_HOST_RESTORE_PREF_KEY = 3120863703UL;
static const uint32_t STANDBY_TIMEOUT_SECONDS_RESTORE_PREF_KEY = 2269619899UL;
static const std::array<uint32_t, CONFIGURABLE_DEVICE_SLOT_COUNT> DEVICE_TYPE_RESTORE_PREF_KEYS = {
    2895307732UL, 3842786398UL, 1065727512UL, 154823182UL, 1579139780UL,
    834557022UL,  1476224880UL, 3411481934UL, 4219871412UL, 3942567008UL,
};

struct StoredHaToken {
  uint32_t magic;
  char token[MAX_HA_TOKEN_LENGTH + 1];
};

struct StoredHaEntity {
  char entity_id[MAX_HA_ENTITY_ID_LENGTH + 1];
  char friendly_name[MAX_HA_FRIENDLY_NAME_LENGTH + 1];
  char area_name[MAX_HA_AREA_NAME_LENGTH + 1];
};

struct StoredHaEntities {
  uint32_t magic;
  uint16_t count;
  StoredHaEntity entities[MAX_HA_ENTITY_COUNT];
};

struct StoredHaEntityChunk {
  uint32_t magic;
  uint16_t total_count;
  uint16_t chunk_index;
  uint16_t count;
  StoredHaEntity entities[HA_ENTITY_PREF_CHUNK_SIZE];
};

struct StoredHaArea {
  char area_id[MAX_HA_AREA_ID_LENGTH + 1];
  char name[MAX_HA_AREA_NAME_LENGTH + 1];
};

struct StoredHaAreas {
  uint32_t magic;
  uint8_t count;
  StoredHaArea areas[MAX_HA_AREA_COUNT];
};

struct StoredDevice1Config {
  uint32_t magic;
  char entity_id[MAX_HA_ENTITY_ID_LENGTH + 1];
  char custom_type[16];
  uint32_t custom_light_capability_mask;
};

struct StoredStandbyWeatherConfig {
  uint32_t magic;
  char entity_id[MAX_HA_ENTITY_ID_LENGTH + 1];
};

struct StoredDeviceLightRuntimeConfig {
  uint32_t magic;
  char entity_id[MAX_HA_ENTITY_ID_LENGTH + 1];
  uint32_t inferred_light_capability_mask;
  uint16_t color_temperature_min_kelvin;
  uint16_t color_temperature_max_kelvin;
};

enum class HaTokenStatus : uint8_t {
  NOT_CONFIGURED = 0,
  CONFIGURED = 1,
  INVALID = 2,
  VALID = 3,
};

static esphome::ESPPreferenceObject ha_token_pref;
static std::array<esphome::ESPPreferenceObject, HA_ENTITY_PREF_CHUNK_COUNT> ha_entity_chunk_prefs;
static esphome::ESPPreferenceObject ha_areas_pref;
static esphome::ESPPreferenceObject device1_config_pref;
static esphome::ESPPreferenceObject device2_config_pref;
static esphome::ESPPreferenceObject device3_config_pref;
static esphome::ESPPreferenceObject device4_config_pref;
static esphome::ESPPreferenceObject device5_config_pref;
static esphome::ESPPreferenceObject device6_config_pref;
static esphome::ESPPreferenceObject device7_config_pref;
static esphome::ESPPreferenceObject device8_config_pref;
static esphome::ESPPreferenceObject device9_config_pref;
static esphome::ESPPreferenceObject device10_config_pref;
static esphome::ESPPreferenceObject standby_weather_config_pref;
static esphome::ESPPreferenceObject device1_light_runtime_pref;
static esphome::ESPPreferenceObject device2_light_runtime_pref;
static esphome::ESPPreferenceObject device3_light_runtime_pref;
static esphome::ESPPreferenceObject device4_light_runtime_pref;
static esphome::ESPPreferenceObject device5_light_runtime_pref;
static esphome::ESPPreferenceObject device6_light_runtime_pref;
static esphome::ESPPreferenceObject device7_light_runtime_pref;
static esphome::ESPPreferenceObject device8_light_runtime_pref;
static esphome::ESPPreferenceObject device9_light_runtime_pref;
static esphome::ESPPreferenceObject device10_light_runtime_pref;
static bool ha_token_pref_ready = false;
static bool ha_entity_chunk_prefs_ready = false;
static bool ha_areas_pref_ready = false;
static bool device1_config_pref_ready = false;
static bool device2_config_pref_ready = false;
static bool device3_config_pref_ready = false;
static bool device4_config_pref_ready = false;
static bool device5_config_pref_ready = false;
static bool device6_config_pref_ready = false;
static bool device7_config_pref_ready = false;
static bool device8_config_pref_ready = false;
static bool device9_config_pref_ready = false;
static bool device10_config_pref_ready = false;
static bool standby_weather_config_pref_ready = false;
static bool device1_light_runtime_pref_ready = false;
static bool device2_light_runtime_pref_ready = false;
static bool device3_light_runtime_pref_ready = false;
static bool device4_light_runtime_pref_ready = false;
static bool device5_light_runtime_pref_ready = false;
static bool device6_light_runtime_pref_ready = false;
static bool device7_light_runtime_pref_ready = false;
static bool device8_light_runtime_pref_ready = false;
static bool device9_light_runtime_pref_ready = false;
static bool device10_light_runtime_pref_ready = false;
static bool ha_token_loaded = false;
static bool ha_entities_loaded = false;
static bool ha_areas_loaded = false;
static bool device1_config_loaded = false;
static bool device2_config_loaded = false;
static bool device3_config_loaded = false;
static bool device4_config_loaded = false;
static bool device5_config_loaded = false;
static bool device6_config_loaded = false;
static bool device7_config_loaded = false;
static bool device8_config_loaded = false;
static bool device9_config_loaded = false;
static bool device10_config_loaded = false;
static bool standby_weather_config_loaded = false;
static bool ha_entities_changed_since_last_parse_flag = false;
static std::string cached_ha_token;
static std::string cached_device1_entity_id;
static std::string cached_device1_custom_type;
static uint32_t cached_device1_custom_light_capability_mask = 1;
static std::string cached_device2_entity_id;
static std::string cached_device2_custom_type;
static uint32_t cached_device2_custom_light_capability_mask = 1;
static std::string cached_device3_entity_id;
static std::string cached_device3_custom_type;
static uint32_t cached_device3_custom_light_capability_mask = 1;
static std::string cached_device4_entity_id;
static std::string cached_device4_custom_type;
static uint32_t cached_device4_custom_light_capability_mask = 1;
static std::string cached_device5_entity_id;
static std::string cached_device5_custom_type;
static uint32_t cached_device5_custom_light_capability_mask = 1;
static std::string cached_device6_entity_id;
static std::string cached_device6_custom_type;
static uint32_t cached_device6_custom_light_capability_mask = 1;
static std::string cached_device7_entity_id;
static std::string cached_device7_custom_type;
static uint32_t cached_device7_custom_light_capability_mask = 1;
static std::string cached_device8_entity_id;
static std::string cached_device8_custom_type;
static uint32_t cached_device8_custom_light_capability_mask = 1;
static std::string cached_device9_entity_id;
static std::string cached_device9_custom_type;
static uint32_t cached_device9_custom_light_capability_mask = 1;
static std::string cached_device10_entity_id;
static std::string cached_device10_custom_type;
static uint32_t cached_device10_custom_light_capability_mask = 1;
static std::string cached_standby_weather_entity_id;
static HaTokenStatus cached_ha_token_status = HaTokenStatus::NOT_CONFIGURED;
static RamAllocatedPtr<StoredHaEntities> cached_ha_entities_storage;
static StoredHaAreas cached_ha_areas{};
static std::string cached_ha_auth_header;
static RamAllocatedPtr<HaEntityOptionArray> device1_config_option_strings_storage;
static RamAllocatedPtr<HaEntityOptionArray> staged_device1_config_option_strings_storage;
static RamAllocatedPtr<HaEntityOptionArray> standby_weather_config_option_strings_storage;
static RamAllocatedPtr<HaEntityOptionArray> staged_standby_weather_config_option_strings_storage;
static std::string cached_provisioning_ap_ssid;
static uint32_t ha_ws_config_revision = 0;
static esphome::onx_storage::OnxStorage *normal_ui_storage = nullptr;

inline HaEntityOptionArray &ha_entity_option_array(RamAllocatedPtr<HaEntityOptionArray> &storage, const char *label) {
  if (storage == nullptr) {
    storage = make_ram_allocated<HaEntityOptionArray>();
    if (storage == nullptr) {
      ESP_LOGE(TAG, "failed to allocate %s option array (%zu bytes)", label != nullptr ? label : "HA entity",
               sizeof(HaEntityOptionArray));
      static HaEntityOptionArray *fallback = nullptr;
      if (fallback == nullptr) {
        fallback = new (std::nothrow) HaEntityOptionArray();
      }
      return *fallback;
    }
  }
  return *storage;
}

inline HaEntityOptionArray &standby_weather_config_options() {
  return ha_entity_option_array(standby_weather_config_option_strings_storage, "standby weather");
}

inline HaEntityOptionArray &staged_standby_weather_config_options() {
  return ha_entity_option_array(staged_standby_weather_config_option_strings_storage, "staged standby weather");
}

inline StoredHaEntities &cached_ha_entities() {
  if (cached_ha_entities_storage == nullptr) {
    cached_ha_entities_storage = make_ram_allocated<StoredHaEntities>();
    if (cached_ha_entities_storage != nullptr) {
      cached_ha_entities_storage->magic = HA_ENTITIES_MAGIC;
    } else {
      ESP_LOGE(TAG, "failed to allocate cached HA entities in RAM allocator (%zu bytes)", sizeof(StoredHaEntities));
      static StoredHaEntities *fallback = nullptr;
      if (fallback == nullptr) {
        fallback = new (std::nothrow) StoredHaEntities();
        if (fallback != nullptr) {
          fallback->magic = HA_ENTITIES_MAGIC;
        }
      }
      return *fallback;
    }
  }
  return *cached_ha_entities_storage;
}

struct InferredNormalDeviceConfig {
  bool valid{false};
  std::string title;
  std::string type;
  std::string entity_id;
  uint32_t inferred_light_capability_mask{1};
  uint32_t light_capability_mask{1};
  std::string custom_type;
  uint32_t custom_light_capability_mask{1};
  int color_temperature_min_kelvin{2000};
  int color_temperature_max_kelvin{6000};
  std::string curtain_position_preset{"100%"};
};

static InferredNormalDeviceConfig device1_inferred_config{};
static InferredNormalDeviceConfig device2_inferred_config{};
static InferredNormalDeviceConfig device3_inferred_config{};
static InferredNormalDeviceConfig device4_inferred_config{};
static InferredNormalDeviceConfig device5_inferred_config{};
static InferredNormalDeviceConfig device6_inferred_config{};
static InferredNormalDeviceConfig device7_inferred_config{};
static InferredNormalDeviceConfig device8_inferred_config{};
static InferredNormalDeviceConfig device9_inferred_config{};
static InferredNormalDeviceConfig device10_inferred_config{};

inline uint32_t normalize_light_capability_mask(uint32_t mask);
inline bool is_configurable_device_slot(size_t slot);
inline std::string device_slot_label(size_t slot);
inline void set_cached_device1_custom_config_from_entity_default(const std::string &entity_id);
inline void set_cached_device2_custom_config_from_entity_default(const std::string &entity_id);
inline std::string normal_ui_type_from_entity_id(const std::string &entity_id);
inline bool is_supported_normal_device_entity_id(const char *entity_id);
inline bool is_local_hmi_gpio_switch_entity_id(const std::string &entity_id);
inline void load_device_config_if_needed(size_t slot);
inline void ensure_device_config_pref_ready(size_t slot);
inline bool apply_device_config_options(size_t slot, esphome::select::Select *my_select);
inline bool apply_standby_weather_config_options(esphome::select::Select *my_select);
inline bool save_device_config_entity_id(size_t slot, const std::string &entity_id);
inline bool save_standby_weather_config_entity_id(const std::string &entity_id);
inline bool save_device_custom_config(size_t slot, const std::string &custom_type, uint32_t custom_light_capability_mask);
inline bool apply_device_config_to_normal_ui_storage(size_t slot);
inline std::string ha_json_variant_to_string(JsonVariantConst value);
inline std::string ha_entity_state_text(JsonObjectConst state_obj);
inline JsonObjectConst ha_entity_attrs(JsonObjectConst state_obj);
inline std::string ha_entity_title_from_registry_cache(const std::string &entity_id);
inline std::string build_ha_http_state_url(const std::string &raw_host, const std::string &entity_id);
inline bool update_device_light_config_from_http_response(size_t slot, const std::string &entity_id,
                                                          const std::string &body);
inline JsonDocument make_psram_json_document();

#ifdef USE_ESP32
static esp_websocket_client_handle_t ha_ws_client = nullptr;
static std::string ha_ws_uri;
static std::string ha_ws_last_host;
static std::string ha_ws_last_token;
static uint32_t ha_ws_seen_config_revision = 0;
static uint32_t ha_ws_next_attempt_ms = 0;
static uint32_t ha_ws_backoff_ms = 2000;
static bool ha_ws_started = false;
static bool ha_ws_authenticated = false;
static bool ha_ws_auth_sent = false;
static bool ha_ws_auth_invalid = false;
static bool ha_ws_initial_ping_sent = false;
static bool ha_ws_disconnected_seen = false;
static bool ha_ws_stop_requested = false;
static bool ha_ws_reported_connected = false;
static bool ha_ws_entity_list_pending = false;
static bool ha_ws_entity_list_send_pending = false;
static bool ha_ws_registry_fetch_active = false;
static uint32_t ha_ws_registry_fetch_started_ms = 0;
static uint32_t ha_ws_transport_connected_ms = 0;
static uint32_t ha_ws_auth_required_ms = 0;
static uint32_t ha_ws_registry_fetch_attempt = 0;
static uint32_t ha_ws_next_message_id = 1;
static uint32_t ha_ws_area_list_request_id = 0;
static uint32_t ha_ws_entity_list_request_id = 0;
static uint32_t ha_ws_device1_subscribe_request_id = 0;
static uint32_t ha_ws_device2_subscribe_request_id = 0;
static uint32_t ha_ws_device3_subscribe_request_id = 0;
static uint32_t ha_ws_device4_subscribe_request_id = 0;
static uint32_t ha_ws_device5_subscribe_request_id = 0;
static uint32_t ha_ws_device6_subscribe_request_id = 0;
static uint32_t ha_ws_device7_subscribe_request_id = 0;
static uint32_t ha_ws_device8_subscribe_request_id = 0;
static uint32_t ha_ws_device9_subscribe_request_id = 0;
static uint32_t ha_ws_device10_subscribe_request_id = 0;
static esphome::select::Select *ha_ws_device1_entity_select = nullptr;
static esphome::select::Select *ha_ws_device2_entity_select = nullptr;
static esphome::select::Select *ha_ws_device3_entity_select = nullptr;
static esphome::select::Select *ha_ws_device4_entity_select = nullptr;
static esphome::select::Select *ha_ws_device5_entity_select = nullptr;
static esphome::select::Select *ha_ws_device6_entity_select = nullptr;
static esphome::select::Select *ha_ws_device7_entity_select = nullptr;
static esphome::select::Select *ha_ws_device8_entity_select = nullptr;
static esphome::select::Select *ha_ws_device9_entity_select = nullptr;
static esphome::select::Select *ha_ws_device10_entity_select = nullptr;
static esphome::select::Select *ha_ws_standby_weather_entity_select = nullptr;
static esphome::api::APIServer *ha_ws_entity_api_server = nullptr;
static PsramString ha_ws_rx_text_buffer;
#ifdef USE_PSRAM
static esphome::json::SpiRamAllocator ha_json_allocator;
#endif

inline void log_ha_registry_memory(const char *stage) {
  const char *label = stage != nullptr ? stage : "unknown";
  ESP_LOGI(TAG,
           "HA registry memory [%s]: internal_free=%u internal_largest=%u psram_free=%u psram_largest=%u "
           "rx_size=%u rx_capacity=%u task=%s stack_high_water_words=%u",
           label,
           static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
           static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
           static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)),
           static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)),
           static_cast<unsigned>(ha_ws_rx_text_buffer.size()),
           static_cast<unsigned>(ha_ws_rx_text_buffer.capacity()),
           pcTaskGetName(nullptr),
           static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr)));
}

static uint32_t ha_ws_device1_subscription_id = 0;
static std::string ha_ws_device1_subscribed_entity_id;
static std::string ha_ws_device1_pending_entity_id;
static uint32_t ha_ws_device2_subscription_id = 0;
static std::string ha_ws_device2_subscribed_entity_id;
static std::string ha_ws_device2_pending_entity_id;
static uint32_t ha_ws_device3_subscription_id = 0;
static std::string ha_ws_device3_subscribed_entity_id;
static std::string ha_ws_device3_pending_entity_id;
static uint32_t ha_ws_device4_subscription_id = 0;
static std::string ha_ws_device4_subscribed_entity_id;
static std::string ha_ws_device4_pending_entity_id;
static uint32_t ha_ws_device5_subscription_id = 0;
static uint32_t ha_ws_device6_subscription_id = 0;
static uint32_t ha_ws_device7_subscription_id = 0;
static uint32_t ha_ws_device8_subscription_id = 0;
static uint32_t ha_ws_device9_subscription_id = 0;
static uint32_t ha_ws_device10_subscription_id = 0;
static std::string ha_ws_device5_subscribed_entity_id;
static std::string ha_ws_device5_pending_entity_id;
static std::string ha_ws_device6_subscribed_entity_id;
static std::string ha_ws_device6_pending_entity_id;
static std::string ha_ws_device7_subscribed_entity_id;
static std::string ha_ws_device7_pending_entity_id;
static std::string ha_ws_device8_subscribed_entity_id;
static std::string ha_ws_device8_pending_entity_id;
static std::string ha_ws_device9_subscribed_entity_id;
static std::string ha_ws_device9_pending_entity_id;
static std::string ha_ws_device10_subscribed_entity_id;
static std::string ha_ws_device10_pending_entity_id;
static uint32_t ha_ws_device1_feedback_revision = 0;
static uint32_t ha_ws_device1_feedback_consumed_revision = 0;
static uint32_t ha_ws_device2_feedback_revision = 0;
static uint32_t ha_ws_device2_feedback_consumed_revision = 0;
static uint32_t ha_ws_device3_feedback_revision = 0;
static uint32_t ha_ws_device3_feedback_consumed_revision = 0;
static uint32_t ha_ws_device4_feedback_revision = 0;
static uint32_t ha_ws_device4_feedback_consumed_revision = 0;
static uint32_t ha_ws_device5_feedback_revision = 0;
static uint32_t ha_ws_device5_feedback_consumed_revision = 0;
static uint32_t ha_ws_device6_feedback_revision = 0;
static uint32_t ha_ws_device6_feedback_consumed_revision = 0;
static uint32_t ha_ws_device7_feedback_revision = 0;
static uint32_t ha_ws_device7_feedback_consumed_revision = 0;
static uint32_t ha_ws_device8_feedback_revision = 0;
static uint32_t ha_ws_device8_feedback_consumed_revision = 0;
static uint32_t ha_ws_device9_feedback_revision = 0;
static uint32_t ha_ws_device9_feedback_consumed_revision = 0;
static uint32_t ha_ws_device10_feedback_revision = 0;
static uint32_t ha_ws_device10_feedback_consumed_revision = 0;
static bool ha_ws_device1_online_known = false;
static bool ha_ws_device1_online = true;
static bool ha_ws_device1_power_known = false;
static bool ha_ws_device1_power = false;
static bool ha_ws_device2_online_known = false;
static bool ha_ws_device2_online = true;
static bool ha_ws_device2_power_known = false;
static bool ha_ws_device2_power = false;
static bool ha_ws_device3_online_known = false;
static bool ha_ws_device3_online = true;
static bool ha_ws_device3_power_known = false;
static bool ha_ws_device3_power = false;
static bool ha_ws_device4_online_known = false;
static bool ha_ws_device4_online = true;
static bool ha_ws_device4_power_known = false;
static bool ha_ws_device4_power = false;
static bool ha_ws_device5_online_known = false;
static bool ha_ws_device5_online = true;
static bool ha_ws_device5_power_known = false;
static bool ha_ws_device5_power = false;
static bool ha_ws_device6_online_known = false;
static bool ha_ws_device6_online = true;
static bool ha_ws_device6_power_known = false;
static bool ha_ws_device6_power = false;
static bool ha_ws_device7_online_known = false;
static bool ha_ws_device7_online = true;
static bool ha_ws_device7_power_known = false;
static bool ha_ws_device7_power = false;
static bool ha_ws_device8_online_known = false;
static bool ha_ws_device8_online = true;
static bool ha_ws_device8_power_known = false;
static bool ha_ws_device8_power = false;
static bool ha_ws_device9_online_known = false;
static bool ha_ws_device9_online = true;
static bool ha_ws_device9_power_known = false;
static bool ha_ws_device9_power = false;
static bool ha_ws_device10_online_known = false;
static bool ha_ws_device10_online = true;
static bool ha_ws_device10_power_known = false;
static bool ha_ws_device10_power = false;
#endif
static esphome::api::APIServer *api_refresh_disconnect_server = nullptr;
static bool api_refresh_disconnect_pending = false;

inline size_t api_connected_client_count(esphome::api::APIServer *api_server) {
  if (api_server == nullptr) {
    return 0;
  }
  size_t client_count = 0;
  for (const auto &client : api_server->active_clients()) {
    if (client != nullptr) {
      ++client_count;
    }
  }
  return client_count;
}

inline void process_pending_api_client_refresh() {
  if (!api_refresh_disconnect_pending) {
    return;
  }
  esphome::api::APIServer *api_server = api_refresh_disconnect_server;
  api_refresh_disconnect_pending = false;
  api_refresh_disconnect_server = nullptr;
  if (api_server == nullptr) {
    ESP_LOGW(TAG, "api server is null, skip pending client refresh");
    return;
  }

  const size_t client_count = api_connected_client_count(api_server);
  if (client_count == 0) {
    ESP_LOGI(TAG, "HA API client refresh skipped: no connected clients");
    return;
  }

  ESP_LOGI(TAG, "HA API client refresh disconnect begin: client_count=%u",
           static_cast<unsigned>(client_count));
  for (const auto &client : api_server->active_clients()) {
    if (client == nullptr) {
      continue;
    }
    esphome::api::DisconnectRequest req;
    if (!client->send_message(req)) {
      ESP_LOGW(TAG, "failed to send HA API client refresh disconnect request");
    }
  }
}

inline void disconnect_all_clients_for_refresh(esphome::api::APIServer *api_server) {
  if (api_server == nullptr) {
    ESP_LOGW(TAG, "api server is null, skip client refresh");
    return;
  }

  const size_t client_count = api_connected_client_count(api_server);
  if (client_count == 0) {
    ESP_LOGI(TAG, "HA API client refresh skipped: no connected clients");
    return;
  }

  api_refresh_disconnect_server = api_server;
  api_refresh_disconnect_pending = true;
  ESP_LOGI(TAG, "HA API client refresh queued: client_count=%u", static_cast<unsigned>(client_count));
}

inline void ensure_ha_token_pref_ready() {
  if (ha_token_pref_ready) {
    return;
  }
  if (esphome::global_preferences == nullptr) {
    ESP_LOGW(TAG, "preferences backend is unavailable; HA token will stay in RAM only");
    return;
  }
  ha_token_pref = esphome::global_preferences->make_preference<StoredHaToken>(HA_TOKEN_PREF_KEY, true);
  ha_token_pref_ready = true;
}

inline void ensure_ha_entity_chunk_prefs_ready() {
  if (ha_entity_chunk_prefs_ready) {
    return;
  }
  if (esphome::global_preferences == nullptr) {
    ESP_LOGW(TAG, "preferences backend is unavailable; HA entities will stay in RAM only");
    return;
  }
  for (size_t i = 0; i < HA_ENTITY_PREF_CHUNK_COUNT; i++) {
    ha_entity_chunk_prefs[i] =
        esphome::global_preferences->make_preference<StoredHaEntityChunk>(HA_ENTITY_CHUNK_PREF_KEY_BASE + i, true);
  }
  ha_entity_chunk_prefs_ready = true;
}

inline void ensure_ha_areas_pref_ready() {
  if (ha_areas_pref_ready) {
    return;
  }
  if (esphome::global_preferences == nullptr) {
    ESP_LOGW(TAG, "preferences backend is unavailable; HA areas will stay in RAM only");
    return;
  }
  ha_areas_pref = esphome::global_preferences->make_preference<StoredHaAreas>(HA_AREAS_PREF_KEY, true);
  ha_areas_pref_ready = true;
}

inline void ensure_standby_weather_config_pref_ready() {
  if (standby_weather_config_pref_ready) {
    return;
  }
  if (esphome::global_preferences == nullptr) {
    ESP_LOGW(TAG, "preferences backend is unavailable; standby weather config will stay in RAM only");
    return;
  }
  standby_weather_config_pref =
      esphome::global_preferences->make_preference<StoredStandbyWeatherConfig>(STANDBY_WEATHER_CONFIG_PREF_KEY, true);
  standby_weather_config_pref_ready = true;
}

inline void ensure_device1_config_pref_ready() {
  ensure_device_config_pref_ready(0);
}

inline void ensure_device2_config_pref_ready() {
  ensure_device_config_pref_ready(1);
}

inline bool is_configurable_device_slot(size_t slot) {
  return slot < CONFIGURABLE_DEVICE_SLOT_COUNT;
}

inline void set_normal_ui_device_online_state(size_t slot, bool online) {
  if (!is_configurable_device_slot(slot)) {
    return;
  }
  const uint32_t bit = 1u << slot;
  if (online) {
    id(normal_device_online_mask) |= bit;
  } else {
    id(normal_device_online_mask) &= ~bit;
  }
}

inline std::string device_slot_label(size_t slot) {
  return std::string("device") + std::to_string(slot + 1);
}

inline uint32_t device_config_magic(size_t slot) {
  switch (slot) {
    case 0:
      return DEVICE1_CONFIG_MAGIC;
    case 1:
      return DEVICE2_CONFIG_MAGIC;
    case 2:
      return DEVICE3_CONFIG_MAGIC;
    case 3:
      return DEVICE4_CONFIG_MAGIC;
    case 4:
      return DEVICE5_CONFIG_MAGIC;
    case 5:
      return DEVICE6_CONFIG_MAGIC;
    case 6:
      return DEVICE7_CONFIG_MAGIC;
    case 7:
      return DEVICE8_CONFIG_MAGIC;
    case 8:
      return DEVICE9_CONFIG_MAGIC;
    case 9:
      return DEVICE10_CONFIG_MAGIC;
    default:
      return DEVICE1_CONFIG_MAGIC;
  }
}

inline uint32_t device_config_pref_key(size_t slot) {
  switch (slot) {
    case 0:
      return DEVICE1_CONFIG_PREF_KEY;
    case 1:
      return DEVICE2_CONFIG_PREF_KEY;
    case 2:
      return DEVICE3_CONFIG_PREF_KEY;
    case 3:
      return DEVICE4_CONFIG_PREF_KEY;
    case 4:
      return DEVICE5_CONFIG_PREF_KEY;
    case 5:
      return DEVICE6_CONFIG_PREF_KEY;
    case 6:
      return DEVICE7_CONFIG_PREF_KEY;
    case 7:
      return DEVICE8_CONFIG_PREF_KEY;
    case 8:
      return DEVICE9_CONFIG_PREF_KEY;
    case 9:
      return DEVICE10_CONFIG_PREF_KEY;
    default:
      return DEVICE1_CONFIG_PREF_KEY;
  }
}

inline uint32_t device_light_runtime_pref_key(size_t slot) {
  switch (slot) {
    case 0:
      return DEVICE1_LIGHT_RUNTIME_PREF_KEY;
    case 1:
      return DEVICE2_LIGHT_RUNTIME_PREF_KEY;
    case 2:
      return DEVICE3_LIGHT_RUNTIME_PREF_KEY;
    case 3:
      return DEVICE4_LIGHT_RUNTIME_PREF_KEY;
    case 4:
      return DEVICE5_LIGHT_RUNTIME_PREF_KEY;
    case 5:
      return DEVICE6_LIGHT_RUNTIME_PREF_KEY;
    case 6:
      return DEVICE7_LIGHT_RUNTIME_PREF_KEY;
    case 7:
      return DEVICE8_LIGHT_RUNTIME_PREF_KEY;
    case 8:
      return DEVICE9_LIGHT_RUNTIME_PREF_KEY;
    case 9:
      return DEVICE10_LIGHT_RUNTIME_PREF_KEY;
    default:
      return DEVICE1_LIGHT_RUNTIME_PREF_KEY;
  }
}

inline esphome::ESPPreferenceObject &device_config_pref(size_t slot) {
  switch (slot) {
    case 0:
      return device1_config_pref;
    case 1:
      return device2_config_pref;
    case 2:
      return device3_config_pref;
    case 3:
      return device4_config_pref;
    case 4:
      return device5_config_pref;
    case 5:
      return device6_config_pref;
    case 6:
      return device7_config_pref;
    case 7:
      return device8_config_pref;
    case 8:
      return device9_config_pref;
    default:
      return device10_config_pref;
  }
}

inline esphome::ESPPreferenceObject &device_light_runtime_pref(size_t slot) {
  switch (slot) {
    case 0:
      return device1_light_runtime_pref;
    case 1:
      return device2_light_runtime_pref;
    case 2:
      return device3_light_runtime_pref;
    case 3:
      return device4_light_runtime_pref;
    case 4:
      return device5_light_runtime_pref;
    case 5:
      return device6_light_runtime_pref;
    case 6:
      return device7_light_runtime_pref;
    case 7:
      return device8_light_runtime_pref;
    case 8:
      return device9_light_runtime_pref;
    default:
      return device10_light_runtime_pref;
  }
}

inline bool &device_config_pref_ready(size_t slot) {
  switch (slot) {
    case 0:
      return device1_config_pref_ready;
    case 1:
      return device2_config_pref_ready;
    case 2:
      return device3_config_pref_ready;
    case 3:
      return device4_config_pref_ready;
    case 4:
      return device5_config_pref_ready;
    case 5:
      return device6_config_pref_ready;
    case 6:
      return device7_config_pref_ready;
    case 7:
      return device8_config_pref_ready;
    case 8:
      return device9_config_pref_ready;
    default:
      return device10_config_pref_ready;
  }
}

inline bool &device_light_runtime_pref_ready(size_t slot) {
  switch (slot) {
    case 0:
      return device1_light_runtime_pref_ready;
    case 1:
      return device2_light_runtime_pref_ready;
    case 2:
      return device3_light_runtime_pref_ready;
    case 3:
      return device4_light_runtime_pref_ready;
    case 4:
      return device5_light_runtime_pref_ready;
    case 5:
      return device6_light_runtime_pref_ready;
    case 6:
      return device7_light_runtime_pref_ready;
    case 7:
      return device8_light_runtime_pref_ready;
    case 8:
      return device9_light_runtime_pref_ready;
    default:
      return device10_light_runtime_pref_ready;
  }
}

inline bool &device_config_loaded(size_t slot) {
  switch (slot) {
    case 0:
      return device1_config_loaded;
    case 1:
      return device2_config_loaded;
    case 2:
      return device3_config_loaded;
    case 3:
      return device4_config_loaded;
    case 4:
      return device5_config_loaded;
    case 5:
      return device6_config_loaded;
    case 6:
      return device7_config_loaded;
    case 7:
      return device8_config_loaded;
    case 8:
      return device9_config_loaded;
    default:
      return device10_config_loaded;
  }
}

inline std::string &cached_device_entity_id(size_t slot) {
  switch (slot) {
    case 0:
      return cached_device1_entity_id;
    case 1:
      return cached_device2_entity_id;
    case 2:
      return cached_device3_entity_id;
    case 3:
      return cached_device4_entity_id;
    case 4:
      return cached_device5_entity_id;
    case 5:
      return cached_device6_entity_id;
    case 6:
      return cached_device7_entity_id;
    case 7:
      return cached_device8_entity_id;
    case 8:
      return cached_device9_entity_id;
    default:
      return cached_device10_entity_id;
  }
}

inline size_t configured_normal_device_count() {
  size_t count = 0;
  for (size_t slot = 0; slot < CONFIGURABLE_DEVICE_SLOT_COUNT; slot++) {
    load_device_config_if_needed(slot);
    const std::string &entity_id = cached_device_entity_id(slot);
    if (!entity_id.empty() && is_supported_normal_device_entity_id(entity_id.c_str())) {
      count++;
    }
  }
  return count;
}

inline std::string &cached_device_custom_type(size_t slot) {
  switch (slot) {
    case 0:
      return cached_device1_custom_type;
    case 1:
      return cached_device2_custom_type;
    case 2:
      return cached_device3_custom_type;
    case 3:
      return cached_device4_custom_type;
    case 4:
      return cached_device5_custom_type;
    case 5:
      return cached_device6_custom_type;
    case 6:
      return cached_device7_custom_type;
    case 7:
      return cached_device8_custom_type;
    case 8:
      return cached_device9_custom_type;
    default:
      return cached_device10_custom_type;
  }
}

inline uint32_t &cached_device_custom_light_capability_mask(size_t slot) {
  switch (slot) {
    case 0:
      return cached_device1_custom_light_capability_mask;
    case 1:
      return cached_device2_custom_light_capability_mask;
    case 2:
      return cached_device3_custom_light_capability_mask;
    case 3:
      return cached_device4_custom_light_capability_mask;
    case 4:
      return cached_device5_custom_light_capability_mask;
    case 5:
      return cached_device6_custom_light_capability_mask;
    case 6:
      return cached_device7_custom_light_capability_mask;
    case 7:
      return cached_device8_custom_light_capability_mask;
    case 8:
      return cached_device9_custom_light_capability_mask;
    default:
      return cached_device10_custom_light_capability_mask;
  }
}

inline InferredNormalDeviceConfig &device_inferred_config(size_t slot) {
  switch (slot) {
    case 0:
      return device1_inferred_config;
    case 1:
      return device2_inferred_config;
    case 2:
      return device3_inferred_config;
    case 3:
      return device4_inferred_config;
    case 4:
      return device5_inferred_config;
    case 5:
      return device6_inferred_config;
    case 6:
      return device7_inferred_config;
    case 7:
      return device8_inferred_config;
    case 8:
      return device9_inferred_config;
    default:
      return device10_inferred_config;
  }
}

inline HaEntityOptionArray &device_config_option_strings(size_t slot) {
  (void) slot;
  return ha_entity_option_array(device1_config_option_strings_storage, "device config");
}

inline HaEntityOptionArray &staged_device_config_option_strings(size_t slot) {
  (void) slot;
  return ha_entity_option_array(staged_device1_config_option_strings_storage, "staged device config");
}

inline void load_standby_weather_config_if_needed() {
  if (standby_weather_config_loaded) {
    return;
  }
  standby_weather_config_loaded = true;
  cached_standby_weather_entity_id.clear();
  ensure_standby_weather_config_pref_ready();
  if (!standby_weather_config_pref_ready) {
    return;
  }

  StoredStandbyWeatherConfig stored{};
  if (!standby_weather_config_pref.load(&stored) || stored.magic != STANDBY_WEATHER_CONFIG_MAGIC) {
    return;
  }
  stored.entity_id[MAX_HA_ENTITY_ID_LENGTH] = '\0';
  cached_standby_weather_entity_id = stored.entity_id;
}

inline void ensure_device_config_pref_ready(size_t slot) {
  if (!is_configurable_device_slot(slot) || device_config_pref_ready(slot)) {
    return;
  }
  if (esphome::global_preferences == nullptr) {
    ESP_LOGW(TAG, "preferences backend is unavailable; %s config will stay in RAM only",
             device_slot_label(slot).c_str());
    return;
  }
  device_config_pref(slot) =
      esphome::global_preferences->make_preference<StoredDevice1Config>(device_config_pref_key(slot), true);
  device_config_pref_ready(slot) = true;
}

inline void ensure_device3_config_pref_ready() {
  ensure_device_config_pref_ready(2);
}

inline void ensure_device4_config_pref_ready() {
  ensure_device_config_pref_ready(3);
}

inline void ensure_device5_config_pref_ready() {
  ensure_device_config_pref_ready(4);
}

inline void load_ha_areas_if_needed() {
  if (ha_areas_loaded) {
    return;
  }
  ha_areas_loaded = true;
  cached_ha_areas = {};
  cached_ha_areas.magic = HA_AREAS_MAGIC;
  ensure_ha_areas_pref_ready();
  if (!ha_areas_pref_ready) {
    return;
  }

  StoredHaAreas stored{};
  if (!ha_areas_pref.load(&stored) || stored.magic != HA_AREAS_MAGIC) {
    return;
  }
  if (stored.count > MAX_HA_AREA_COUNT) {
    stored.count = MAX_HA_AREA_COUNT;
  }
  for (size_t i = 0; i < stored.count; i++) {
    stored.areas[i].area_id[MAX_HA_AREA_ID_LENGTH] = '\0';
    stored.areas[i].name[MAX_HA_AREA_NAME_LENGTH] = '\0';
  }
  cached_ha_areas = stored;
}

inline void load_ha_entities_if_needed() {
  if (ha_entities_loaded) {
    return;
  }
  ha_entities_loaded = true;
  cached_ha_entities() = {};
  cached_ha_entities().magic = HA_ENTITIES_MAGIC;
  ensure_ha_entity_chunk_prefs_ready();
  if (!ha_entity_chunk_prefs_ready) {
    return;
  }

  auto chunk = make_ram_allocated<StoredHaEntityChunk>();
  if (chunk == nullptr) {
    ESP_LOGE(TAG, "failed to allocate HA entity chunk buffer (%zu bytes)", sizeof(StoredHaEntityChunk));
    return;
  }
  bool loaded_from_chunks = false;
  if (ha_entity_chunk_prefs[0].load(chunk.get()) && chunk->magic == HA_ENTITY_CHUNK_MAGIC &&
      chunk->chunk_index == 0 && chunk->total_count <= MAX_HA_ENTITY_COUNT) {
    StoredHaEntities &cached = cached_ha_entities();
    cached.count = 0;
    const uint16_t expected_total = chunk->total_count;
    const size_t expected_chunk_count =
        expected_total == 0 ? 1 : (expected_total + HA_ENTITY_PREF_CHUNK_SIZE - 1) / HA_ENTITY_PREF_CHUNK_SIZE;
    bool chunks_complete = true;
    for (size_t chunk_index = 0; chunk_index < expected_chunk_count; chunk_index++) {
      if (chunk_index != 0 &&
          (!ha_entity_chunk_prefs[chunk_index].load(chunk.get()) || chunk->magic != HA_ENTITY_CHUNK_MAGIC)) {
        chunks_complete = false;
        break;
      }
      const size_t offset = chunk_index * HA_ENTITY_PREF_CHUNK_SIZE;
      const size_t expected_count =
          offset < expected_total ? std::min<size_t>(HA_ENTITY_PREF_CHUNK_SIZE, expected_total - offset) : 0;
      if (chunk->chunk_index != chunk_index || chunk->total_count != expected_total ||
          chunk->count != expected_count) {
        chunks_complete = false;
        break;
      }
      for (size_t i = 0; i < chunk->count && cached.count < MAX_HA_ENTITY_COUNT; i++) {
        StoredHaEntity &entity = chunk->entities[i];
        entity.entity_id[MAX_HA_ENTITY_ID_LENGTH] = '\0';
        entity.friendly_name[MAX_HA_FRIENDLY_NAME_LENGTH] = '\0';
        entity.area_name[MAX_HA_AREA_NAME_LENGTH] = '\0';
        cached.entities[cached.count++] = entity;
      }
    }
    loaded_from_chunks = chunks_complete && cached.count == expected_total;
    if (loaded_from_chunks) {
      return;
    }
    cached = {};
    cached.magic = HA_ENTITIES_MAGIC;
  }
}

inline void load_device1_config_if_needed() {
  load_device_config_if_needed(0);
}

inline void load_device2_config_if_needed() {
  load_device_config_if_needed(1);
}

inline void load_device3_config_if_needed() {
  load_device_config_if_needed(2);
}

inline void load_device4_config_if_needed() {
  load_device_config_if_needed(3);
}

inline void load_device5_config_if_needed() {
  load_device_config_if_needed(4);
}

template<size_t N> inline void copy_string_to_fixed_buffer(const std::string &value, char (&target)[N]) {
  memset(target, 0, sizeof(target));
  const size_t copy_length = std::min(value.size(), N - 1);
  if (copy_length > 0) {
    memcpy(target, value.data(), copy_length);
  }
}

inline std::string trim_copy(const std::string &value) {
  size_t begin = 0;
  while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
    begin++;
  }
  size_t end = value.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    end--;
  }
  return value.substr(begin, end - begin);
}

inline bool is_local_hmi_gpio_switch_preset_option(const std::string &option) {
  const std::string normalized_option = trim_copy(option);
  for (const char *preset_option : LOCAL_HMI_GPIO_SWITCH_PRESET_OPTIONS) {
    if (normalized_option == preset_option) {
      return true;
    }
  }
  return normalized_option == "[Preset] Local IO10" ||
         normalized_option == "[Preset] Local IO11" ||
         normalized_option == "[Preset] Local IO17" ||
         normalized_option == "[Preset] Local IO18";
}

inline int local_hmi_gpio_switch_io_from_entity_id(const std::string &entity_id) {
  const std::string normalized_entity_id = trim_copy(entity_id);
  if (normalized_entity_id == "[Local] HMI IO10" || normalized_entity_id == "[Preset] Local IO10") {
    return 10;
  }
  if (normalized_entity_id == "[Local] HMI IO11" || normalized_entity_id == "[Preset] Local IO11") {
    return 11;
  }
  if (normalized_entity_id == "[Local] HMI IO17" || normalized_entity_id == "[Preset] Local IO17") {
    return 17;
  }
  if (normalized_entity_id == "[Local] HMI IO18" || normalized_entity_id == "[Preset] Local IO18") {
    return 18;
  }
  return -1;
}

inline std::string local_hmi_gpio_switch_display_name_from_entity_id(const std::string &entity_id) {
  const int io = local_hmi_gpio_switch_io_from_entity_id(entity_id);
  if (io < 0) {
    return "";
  }
  return "HMI IO" + std::to_string(io);
}

inline std::string extract_entity_id_from_device1_option(const std::string &option) {
  std::string entity_id = trim_copy(option);
  if (entity_id == "none") {
    return "";
  }
  const size_t pos = entity_id.find_last_of('/');
  if (pos != std::string::npos) {
    entity_id = trim_copy(entity_id.substr(pos + 1));
  }
  return entity_id;
}

inline std::string get_provisioning_ap_ssid_string() {
  cached_provisioning_ap_ssid = "ONX2424G013-";
  std::string mac = esphome::get_mac_address();
  for (char &ch : mac) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  cached_provisioning_ap_ssid += mac.size() > 4 ? mac.substr(mac.size() - 4) : mac;
  return cached_provisioning_ap_ssid;
}

inline std::string normalize_ha_token(const std::string &value) {
  std::string token = trim_copy(value);
  static const char *const bearer_prefix = "Bearer ";
  if (token.rfind(bearer_prefix, 0) == 0) {
    token = trim_copy(token.substr(strlen(bearer_prefix)));
  }
  return token;
}

inline const char *ha_token_status_to_text(HaTokenStatus status) {
  switch (status) {
    case HaTokenStatus::NOT_CONFIGURED:
      return "Not configured";
    case HaTokenStatus::CONFIGURED:
      return "Configured";
    case HaTokenStatus::INVALID:
      return "Invalid";
    case HaTokenStatus::VALID:
      return "Valid";
  }
  return "Not configured";
}

inline bool is_valid_ha_host_char(char ch) {
  return std::isalnum(static_cast<unsigned char>(ch)) || ch == '.' || ch == '-' || ch == '_' || ch == ':';
}

inline void set_ha_token_status(HaTokenStatus status) {
  cached_ha_token_status = status;
}

inline void load_ha_token_if_needed() {
  if (ha_token_loaded) {
    return;
  }
  ESP_LOGI(TAG, "HA token load begin");
  ha_token_loaded = true;
  cached_ha_token.clear();
  cached_ha_token_status = HaTokenStatus::NOT_CONFIGURED;
  ensure_ha_token_pref_ready();
  if (!ha_token_pref_ready) {
    ESP_LOGW(TAG, "HA token load failed: preference handle is not ready");
    return;
  }
  StoredHaToken stored{};
  const bool loaded = ha_token_pref.load(&stored);
  if (!loaded) {
    ESP_LOGI(TAG, "HA token load complete: no stored value");
    return;
  }
  if (stored.magic != HA_TOKEN_MAGIC) {
    ESP_LOGW(TAG, "HA token load rejected: magic mismatch");
    return;
  }
  stored.token[MAX_HA_TOKEN_LENGTH] = '\0';
  cached_ha_token = stored.token;
  if (!cached_ha_token.empty()) {
    cached_ha_token_status = HaTokenStatus::CONFIGURED;
  }
  ESP_LOGI(TAG, "HA token load complete: configured=%s token_length=%u",
           cached_ha_token.empty() ? "NO" : "YES", static_cast<unsigned>(cached_ha_token.size()));
}

inline bool save_ha_token(const std::string &raw_token) {
  std::string token = normalize_ha_token(raw_token);
  const bool truncated = token.size() > MAX_HA_TOKEN_LENGTH;
  StoredHaToken stored{};
  stored.magic = HA_TOKEN_MAGIC;
  copy_string_to_fixed_buffer(token, stored.token);
  cached_ha_token = stored.token;
  ha_token_loaded = true;
  cached_ha_token_status = cached_ha_token.empty() ? HaTokenStatus::NOT_CONFIGURED : HaTokenStatus::CONFIGURED;
  ha_ws_config_revision++;

  ensure_ha_token_pref_ready();
  if (!ha_token_pref_ready) {
    ESP_LOGW(TAG,
             "HA token save failed: preference handle is not ready raw_length=%u normalized_length=%u "
             "cached_length=%u truncated=%s revision=%u",
             static_cast<unsigned>(raw_token.size()), static_cast<unsigned>(token.size()),
             static_cast<unsigned>(cached_ha_token.size()), truncated ? "YES" : "NO",
             static_cast<unsigned>(ha_ws_config_revision));
    return false;
  }
  const bool saved = ha_token_pref.save(&stored);
  bool synced = false;
  if (saved && esphome::global_preferences != nullptr) {
    synced = esphome::global_preferences->sync();
  }
  ESP_LOGI(TAG,
           "HA token save complete: configured=%s raw_length=%u normalized_length=%u cached_length=%u "
           "truncated=%s preference_save=%s preference_sync=%s revision=%u",
           cached_ha_token.empty() ? "NO" : "YES", static_cast<unsigned>(raw_token.size()),
           static_cast<unsigned>(token.size()), static_cast<unsigned>(cached_ha_token.size()),
           truncated ? "YES" : "NO", saved ? "OK" : "FAILED", synced ? "OK" : "SKIPPED_OR_FAILED",
           static_cast<unsigned>(ha_ws_config_revision));
  return saved;
}

inline bool has_saved_ha_token() {
  load_ha_token_if_needed();
  return !cached_ha_token.empty();
}

inline const char *get_ha_authorization_header() {
  load_ha_token_if_needed();
  cached_ha_auth_header.clear();
  if (!cached_ha_token.empty()) {
    cached_ha_auth_header = "Bearer ";
    cached_ha_auth_header += cached_ha_token;
  }
  return cached_ha_auth_header.c_str();
}

inline const char *get_ha_token_status_text() {
  load_ha_token_if_needed();
  return ha_token_status_to_text(cached_ha_token_status);
}

inline std::string build_ha_websocket_url(const std::string &raw_host) {
  std::string host = trim_copy(raw_host);
  if (host.empty()) {
    host = "homeassistant.local";
  }
  if (host.rfind("ws://", 0) == 0) {
    host = host.substr(strlen("ws://"));
  } else if (host.rfind("http://", 0) == 0) {
    host = host.substr(strlen("http://"));
  } else if (host.rfind("wss://", 0) == 0 || host.rfind("https://", 0) == 0) {
    ESP_LOGW(TAG, "secure HA websocket is not supported: %s", host.c_str());
    return "";
  }

  const size_t slash_pos = host.find('/');
  if (slash_pos != std::string::npos) {
    host = host.substr(0, slash_pos);
  }
  host = trim_copy(host);
  if (host.empty() || host.size() > 128) {
    ESP_LOGW(TAG, "invalid HA websocket host length");
    return "";
  }
  for (char ch : host) {
    if (!is_valid_ha_host_char(ch)) {
      ESP_LOGW(TAG, "invalid HA websocket host character: %c", ch);
      return "";
    }
  }
  if (host.find(':') == std::string::npos) {
    host += ":8123";
  }
  return "ws://" + host + "/api/websocket";
}

inline std::string build_ha_http_state_url(const std::string &raw_host, const std::string &entity_id) {
  std::string host = trim_copy(raw_host);
  if (host.empty()) {
    host = "homeassistant.local";
  }
  if (host.rfind("http://", 0) == 0) {
    host = host.substr(strlen("http://"));
  } else if (host.rfind("ws://", 0) == 0) {
    host = host.substr(strlen("ws://"));
  } else if (host.rfind("https://", 0) == 0 || host.rfind("wss://", 0) == 0) {
    ESP_LOGW(TAG, "secure HA HTTP fetch is not supported: %s", host.c_str());
    return "";
  }

  const size_t slash_pos = host.find('/');
  if (slash_pos != std::string::npos) {
    host = host.substr(0, slash_pos);
  }
  host = trim_copy(host);
  if (host.empty() || host.size() > 128) {
    ESP_LOGW(TAG, "invalid HA HTTP host length");
    return "";
  }
  for (char ch : host) {
    if (!is_valid_ha_host_char(ch)) {
      ESP_LOGW(TAG, "invalid HA HTTP host character: %c", ch);
      return "";
    }
  }
  if (host.find(':') == std::string::npos) {
    host += ":8123";
  }
  return "http://" + host + "/api/states/" + entity_id;
}

inline bool apply_device1_config_options(esphome::select::Select *my_select);
inline bool apply_device2_config_options(esphome::select::Select *my_select);
inline bool parse_and_store_ha_areas(JsonArrayConst array);
inline bool parse_and_store_ha_entity_registry_entities(JsonArrayConst array);
inline bool save_device1_config_entity_id(const std::string &entity_id);
inline bool save_device2_config_entity_id(const std::string &entity_id);
inline bool is_supported_ha_entity_id(const char *entity_id);

#ifdef USE_ESP32
inline uint32_t ha_ws_next_request_id() {
  uint32_t request_id = ha_ws_next_message_id++;
  if (ha_ws_next_message_id == 0) {
    ha_ws_next_message_id = 1;
  }
  return request_id;
}

inline bool ha_ws_send_area_list_request() {
  if (ha_ws_client == nullptr || !ha_ws_authenticated) {
    return false;
  }
  const uint32_t request_id = ha_ws_next_request_id();
  char request[96];
  snprintf(request, sizeof(request), "{\"id\":%u,\"type\":\"config/area_registry/list\"}",
           static_cast<unsigned>(request_id));
  const int sent = esp_websocket_client_send_text(ha_ws_client, request, strlen(request), pdMS_TO_TICKS(1000));
  if (sent < 0 || static_cast<size_t>(sent) != strlen(request)) {
    ESP_LOGW(TAG, "failed to request HA area list over websocket: result_bytes=%d expected_bytes=%u",
             sent, static_cast<unsigned>(strlen(request)));
    return false;
  }
  ha_ws_area_list_request_id = request_id;
  ha_ws_entity_list_pending = false;
  ESP_LOGI(TAG, "requested HA area list over websocket: request_id=%u bytes=%u",
           static_cast<unsigned>(request_id), static_cast<unsigned>(strlen(request)));
  return true;
}

inline bool ha_ws_send_entity_list_request() {
  if (ha_ws_client == nullptr || !ha_ws_authenticated) {
    return false;
  }
  const uint32_t request_id = ha_ws_next_request_id();
  char request[96];
  snprintf(request, sizeof(request), "{\"id\":%u,\"type\":\"config/entity_registry/list_for_display\"}",
           static_cast<unsigned>(request_id));
  const int sent = esp_websocket_client_send_text(ha_ws_client, request, strlen(request), pdMS_TO_TICKS(1000));
  if (sent < 0 || static_cast<size_t>(sent) != strlen(request)) {
    ESP_LOGW(TAG, "failed to request HA entity list over websocket: result_bytes=%d expected_bytes=%u",
             sent, static_cast<unsigned>(strlen(request)));
    return false;
  }
  ha_ws_entity_list_request_id = request_id;
  ha_ws_entity_list_pending = false;
  ESP_LOGI(TAG, "requested HA entity list over websocket: request_id=%u bytes=%u",
           static_cast<unsigned>(request_id), static_cast<unsigned>(strlen(request)));
  return true;
}

inline std::string json_escape_string(const std::string &value) {
  std::string escaped;
  escaped.reserve(value.size() + 8);
  for (char ch : value) {
    switch (ch) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\b':
        escaped += "\\b";
        break;
      case '\f':
        escaped += "\\f";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped += ch;
        break;
    }
  }
  return escaped;
}

inline bool ha_ws_send_unsubscribe_request(uint32_t subscription_id) {
  if (ha_ws_client == nullptr || !ha_ws_authenticated || subscription_id == 0) {
    return false;
  }
  const uint32_t request_id = ha_ws_next_request_id();
  char request[96];
  snprintf(request, sizeof(request), "{\"id\":%u,\"type\":\"unsubscribe_events\",\"subscription\":%u}",
           static_cast<unsigned>(request_id), static_cast<unsigned>(subscription_id));
  const int sent = esp_websocket_client_send_text(ha_ws_client, request, strlen(request), pdMS_TO_TICKS(1000));
  if (sent < 0) {
    ESP_LOGW(TAG, "failed to unsubscribe device1 entity over websocket");
    return false;
  }
  ESP_LOGI(TAG, "unsubscribed device1 entity: subscription=%u", static_cast<unsigned>(subscription_id));
  return true;
}

inline uint32_t &ha_ws_device_subscribe_request_id(size_t slot) {
  switch (slot) {
    case 0:
      return ha_ws_device1_subscribe_request_id;
    case 1:
      return ha_ws_device2_subscribe_request_id;
    case 2:
      return ha_ws_device3_subscribe_request_id;
    case 3:
      return ha_ws_device4_subscribe_request_id;
    case 4:
      return ha_ws_device5_subscribe_request_id;
    case 5:
      return ha_ws_device6_subscribe_request_id;
    case 6:
      return ha_ws_device7_subscribe_request_id;
    case 7:
      return ha_ws_device8_subscribe_request_id;
    case 8:
      return ha_ws_device9_subscribe_request_id;
    default:
      return ha_ws_device10_subscribe_request_id;
  }
}

inline uint32_t &ha_ws_device_subscription_id(size_t slot) {
  switch (slot) {
    case 0:
      return ha_ws_device1_subscription_id;
    case 1:
      return ha_ws_device2_subscription_id;
    case 2:
      return ha_ws_device3_subscription_id;
    case 3:
      return ha_ws_device4_subscription_id;
    case 4:
      return ha_ws_device5_subscription_id;
    case 5:
      return ha_ws_device6_subscription_id;
    case 6:
      return ha_ws_device7_subscription_id;
    case 7:
      return ha_ws_device8_subscription_id;
    case 8:
      return ha_ws_device9_subscription_id;
    default:
      return ha_ws_device10_subscription_id;
  }
}

inline std::string &ha_ws_device_subscribed_entity_id(size_t slot) {
  switch (slot) {
    case 0:
      return ha_ws_device1_subscribed_entity_id;
    case 1:
      return ha_ws_device2_subscribed_entity_id;
    case 2:
      return ha_ws_device3_subscribed_entity_id;
    case 3:
      return ha_ws_device4_subscribed_entity_id;
    case 4:
      return ha_ws_device5_subscribed_entity_id;
    case 5:
      return ha_ws_device6_subscribed_entity_id;
    case 6:
      return ha_ws_device7_subscribed_entity_id;
    case 7:
      return ha_ws_device8_subscribed_entity_id;
    case 8:
      return ha_ws_device9_subscribed_entity_id;
    default:
      return ha_ws_device10_subscribed_entity_id;
  }
}

inline std::string &ha_ws_device_pending_entity_id(size_t slot) {
  switch (slot) {
    case 0:
      return ha_ws_device1_pending_entity_id;
    case 1:
      return ha_ws_device2_pending_entity_id;
    case 2:
      return ha_ws_device3_pending_entity_id;
    case 3:
      return ha_ws_device4_pending_entity_id;
    case 4:
      return ha_ws_device5_pending_entity_id;
    case 5:
      return ha_ws_device6_pending_entity_id;
    case 6:
      return ha_ws_device7_pending_entity_id;
    case 7:
      return ha_ws_device8_pending_entity_id;
    case 8:
      return ha_ws_device9_pending_entity_id;
    default:
      return ha_ws_device10_pending_entity_id;
  }
}

inline uint32_t &ha_ws_device_feedback_revision(size_t slot) {
  switch (slot) {
    case 0:
      return ha_ws_device1_feedback_revision;
    case 1:
      return ha_ws_device2_feedback_revision;
    case 2:
      return ha_ws_device3_feedback_revision;
    case 3:
      return ha_ws_device4_feedback_revision;
    case 4:
      return ha_ws_device5_feedback_revision;
    case 5:
      return ha_ws_device6_feedback_revision;
    case 6:
      return ha_ws_device7_feedback_revision;
    case 7:
      return ha_ws_device8_feedback_revision;
    case 8:
      return ha_ws_device9_feedback_revision;
    default:
      return ha_ws_device10_feedback_revision;
  }
}

inline uint32_t &ha_ws_device_feedback_consumed_revision(size_t slot) {
  switch (slot) {
    case 0:
      return ha_ws_device1_feedback_consumed_revision;
    case 1:
      return ha_ws_device2_feedback_consumed_revision;
    case 2:
      return ha_ws_device3_feedback_consumed_revision;
    case 3:
      return ha_ws_device4_feedback_consumed_revision;
    case 4:
      return ha_ws_device5_feedback_consumed_revision;
    case 5:
      return ha_ws_device6_feedback_consumed_revision;
    case 6:
      return ha_ws_device7_feedback_consumed_revision;
    case 7:
      return ha_ws_device8_feedback_consumed_revision;
    case 8:
      return ha_ws_device9_feedback_consumed_revision;
    default:
      return ha_ws_device10_feedback_consumed_revision;
  }
}

inline bool &ha_ws_device_online_known(size_t slot) {
  switch (slot) {
    case 0:
      return ha_ws_device1_online_known;
    case 1:
      return ha_ws_device2_online_known;
    case 2:
      return ha_ws_device3_online_known;
    case 3:
      return ha_ws_device4_online_known;
    case 4:
      return ha_ws_device5_online_known;
    case 5:
      return ha_ws_device6_online_known;
    case 6:
      return ha_ws_device7_online_known;
    case 7:
      return ha_ws_device8_online_known;
    case 8:
      return ha_ws_device9_online_known;
    default:
      return ha_ws_device10_online_known;
  }
}

inline bool &ha_ws_device_online(size_t slot) {
  switch (slot) {
    case 0:
      return ha_ws_device1_online;
    case 1:
      return ha_ws_device2_online;
    case 2:
      return ha_ws_device3_online;
    case 3:
      return ha_ws_device4_online;
    case 4:
      return ha_ws_device5_online;
    case 5:
      return ha_ws_device6_online;
    case 6:
      return ha_ws_device7_online;
    case 7:
      return ha_ws_device8_online;
    case 8:
      return ha_ws_device9_online;
    default:
      return ha_ws_device10_online;
  }
}

inline bool &ha_ws_device_power_known(size_t slot) {
  switch (slot) {
    case 0:
      return ha_ws_device1_power_known;
    case 1:
      return ha_ws_device2_power_known;
    case 2:
      return ha_ws_device3_power_known;
    case 3:
      return ha_ws_device4_power_known;
    case 4:
      return ha_ws_device5_power_known;
    case 5:
      return ha_ws_device6_power_known;
    case 6:
      return ha_ws_device7_power_known;
    case 7:
      return ha_ws_device8_power_known;
    case 8:
      return ha_ws_device9_power_known;
    default:
      return ha_ws_device10_power_known;
  }
}

inline bool &ha_ws_device_power(size_t slot) {
  switch (slot) {
    case 0:
      return ha_ws_device1_power;
    case 1:
      return ha_ws_device2_power;
    case 2:
      return ha_ws_device3_power;
    case 3:
      return ha_ws_device4_power;
    case 4:
      return ha_ws_device5_power;
    case 5:
      return ha_ws_device6_power;
    case 6:
      return ha_ws_device7_power;
    case 7:
      return ha_ws_device8_power;
    case 8:
      return ha_ws_device9_power;
    default:
      return ha_ws_device10_power;
  }
}

inline esphome::select::Select *&ha_ws_device_entity_select(size_t slot) {
  switch (slot) {
    case 0:
      return ha_ws_device1_entity_select;
    case 1:
      return ha_ws_device2_entity_select;
    case 2:
      return ha_ws_device3_entity_select;
    case 3:
      return ha_ws_device4_entity_select;
    case 4:
      return ha_ws_device5_entity_select;
    case 5:
      return ha_ws_device6_entity_select;
    case 6:
      return ha_ws_device7_entity_select;
    case 7:
      return ha_ws_device8_entity_select;
    case 8:
      return ha_ws_device9_entity_select;
    default:
      return ha_ws_device10_entity_select;
  }
}

inline uint32_t ha_ws_current_backoff_ms() {
  uint32_t current = ha_ws_backoff_ms;
  if (ha_ws_backoff_ms < 5000) {
    ha_ws_backoff_ms = 5000;
  } else if (ha_ws_backoff_ms < 10000) {
    ha_ws_backoff_ms = 10000;
  } else {
    ha_ws_backoff_ms = 30000;
  }
  return current;
}

inline void ha_ws_schedule_retry(uint32_t now_ms) {
  ha_ws_next_attempt_ms = now_ms + ha_ws_current_backoff_ms();
}

inline void ha_ws_set_connected(bool connected, bool *connected_out) {
  ha_ws_reported_connected = connected;
  if (connected_out != nullptr) {
    *connected_out = connected;
  }
}

inline void ha_ws_destroy_client() {
  if (ha_ws_client != nullptr) {
    if (ha_ws_started) {
      esp_websocket_client_stop(ha_ws_client);
    }
    esp_websocket_client_destroy(ha_ws_client);
  }
  ha_ws_client = nullptr;
  ha_ws_started = false;
  ha_ws_authenticated = false;
  ha_ws_auth_sent = false;
  ha_ws_initial_ping_sent = false;
  ha_ws_disconnected_seen = false;
  ha_ws_stop_requested = false;
}

inline JsonDocument make_psram_json_document() {
#ifdef USE_PSRAM
  return JsonDocument(&ha_json_allocator);
#else
  return JsonDocument();
#endif
}

inline void ha_ws_release_rx_text_buffer() {
  PsramString empty;
  ha_ws_rx_text_buffer.swap(empty);
}

inline void ha_ws_finish_registry_fetch(const char *reason) {
  ESP_LOGI(TAG, "HA websocket registry fetch finished: %s", reason != nullptr ? reason : "unknown");
  log_ha_registry_memory("registry fetch finishing");
  ha_ws_registry_fetch_active = false;
  ha_ws_registry_fetch_started_ms = 0;
  ha_ws_entity_list_pending = false;
  ha_ws_entity_list_send_pending = false;
  ha_ws_area_list_request_id = 0;
  ha_ws_entity_list_request_id = 0;
  ha_ws_release_rx_text_buffer();
  ha_ws_stop_requested = true;
  ha_ws_set_connected(false, nullptr);
}

inline int ha_attr_int_or_default(JsonObjectConst attrs, const char *key, int fallback) {
  JsonVariantConst value = attrs[key];
  if (value.isNull()) {
    return fallback;
  }
  if (value.is<int>()) {
    return value.as<int>();
  }
  if (value.is<float>() || value.is<double>()) {
    return static_cast<int>(value.as<float>());
  }
  return fallback;
}

inline int ha_kelvin_from_mired(JsonObjectConst attrs, const char *key) {
  JsonVariantConst value = attrs[key];
  if (value.isNull()) {
    return -1;
  }
  const float mired = value.as<float>();
  if (mired <= 0.0f) {
    return -1;
  }
  return static_cast<int>((1000000.0f / mired) + 0.5f);
}

inline int ha_min_color_temp_kelvin(JsonObjectConst attrs) {
  const int direct = ha_attr_int_or_default(attrs, "min_color_temp_kelvin", -1);
  if (direct >= 0) {
    return direct;
  }
  return ha_kelvin_from_mired(attrs, "max_mireds");
}

inline int ha_max_color_temp_kelvin(JsonObjectConst attrs) {
  const int direct = ha_attr_int_or_default(attrs, "max_color_temp_kelvin", -1);
  if (direct >= 0) {
    return direct;
  }
  return ha_kelvin_from_mired(attrs, "min_mireds");
}

inline JsonArrayConst ha_supported_color_modes(JsonObjectConst attrs) {
  return attrs["supported_color_modes"].as<JsonArrayConst>();
}

inline std::string ha_supported_color_modes_to_string(JsonArrayConst modes) {
  if (modes.isNull()) {
    return "<null>";
  }
  std::string out = "[";
  bool first = true;
  for (JsonVariantConst mode_variant : modes) {
    if (!first) {
      out += ", ";
    }
    first = false;
    const char *mode = mode_variant.as<const char *>();
    out += mode != nullptr ? std::string("\"") + mode + "\"" : std::string("<null>");
  }
  out += "]";
  return out;
}

inline bool ha_light_supported_color_mode_is(JsonArrayConst modes, const char *expected) {
  if (modes.isNull() || expected == nullptr) {
    return false;
  }
  for (JsonVariantConst mode_variant : modes) {
    const char *mode = mode_variant.as<const char *>();
    if (mode != nullptr && strcmp(mode, expected) == 0) {
      return true;
    }
  }
  return false;
}

inline bool ha_light_supports_turn_on_off(JsonArrayConst modes) {
  return !modes.isNull() && modes.size() > 0;
}

inline bool ha_light_supports_brightness(JsonArrayConst modes) {
  if (modes.isNull()) {
    return false;
  }
  for (JsonVariantConst mode_variant : modes) {
    const char *mode = mode_variant.as<const char *>();
    if (mode != nullptr && strcmp(mode, "onoff") != 0) {
      return true;
    }
  }
  return false;
}

inline bool ha_light_supports_color_temp(JsonArrayConst modes) {
  return ha_light_supported_color_mode_is(modes, "color_temp");
}

inline bool ha_light_supports_color(JsonArrayConst modes) {
  return ha_light_supported_color_mode_is(modes, "hs") || ha_light_supported_color_mode_is(modes, "xy") ||
         ha_light_supported_color_mode_is(modes, "rgb") || ha_light_supported_color_mode_is(modes, "rgbw") ||
         ha_light_supported_color_mode_is(modes, "rgbww");
}

inline uint32_t infer_light_capability_mask(JsonArrayConst modes) {
  const bool supports_turn_on_off = ha_light_supports_turn_on_off(modes);
  const bool supports_brightness = ha_light_supports_brightness(modes);
  const bool supports_color_temp = ha_light_supports_color_temp(modes);
  const bool supports_color = ha_light_supports_color(modes);

  uint32_t mask = 0;
  if (supports_turn_on_off) {
    mask |= 0x1;
  }
  if (supports_brightness) {
    mask |= 0x2;
  }
  if (supports_color_temp) {
    mask |= 0x4;
  }
  if (supports_color) {
    mask |= 0x8;
  }
  const uint32_t normalized_mask = normalize_light_capability_mask(mask);
  // ESP_LOGI(TAG,
  //          "light capability infer: supported_color_modes=%s turn_on_off=%s brightness=%s color_temp=%s color=%s mask=0x%X",
  //          ha_supported_color_modes_to_string(modes).c_str(), supports_turn_on_off ? "YES" : "NO",
  //          supports_brightness ? "YES" : "NO", supports_color_temp ? "YES" : "NO", supports_color ? "YES" : "NO",
  //          static_cast<unsigned>(normalized_mask));
  return normalized_mask;
}

inline uint32_t infer_light_capability_mask(JsonObjectConst attrs) {
  return infer_light_capability_mask(ha_supported_color_modes(attrs));
}

inline std::string ha_domain_from_entity_id(const std::string &entity_id) {
  if (is_local_hmi_gpio_switch_preset_option(entity_id)) {
    return "switch";
  }
  const size_t dot = entity_id.find('.');
  if (dot == std::string::npos) {
    return "";
  }
  return entity_id.substr(0, dot);
}

inline std::string normal_ui_type_from_entity_id(const std::string &entity_id) {
  const std::string domain = ha_domain_from_entity_id(entity_id);
  if (domain == "cover") {
    return "cover";
  }
  if (domain == "light") {
    return "light";
  }
  if (domain == "switch") {
    return "switch";
  }
  if (domain == "automation") {
    return "automation";
  }
  return domain.empty() ? "switch" : domain;
}

inline std::string custom_type_from_device1_type_option(const std::string &option) {
  if (option == "Plug") {
    return "plug";
  }
  if (option == "Cover" || option == "Cover (Preset Off)" || option == "Cover (Preset On)") {
    return "cover";
  }
  if (option == "Light (On/Off)" || option == "Dimmable Light" ||
      option == "Color Temperature Light" || option == "RGB Light") {
    return "light";
  }
  if (option == "Automation") {
    return "automation";
  }
  return "switch";
}

inline uint32_t custom_light_capability_mask_from_device1_type_option(const std::string &option) {
  if (option == "Cover (Preset Off)") {
    return 0;
  }
  if (option == "Cover (Preset On)" || option == "Cover") {
    return 100;
  }
  if (option == "Dimmable Light") {
    return 3;
  }
  if (option == "Color Temperature Light") {
    return 7;
  }
  if (option == "RGB Light") {
    return 15;
  }
  return 1;
}

inline uint32_t light_type_option_capability_mask(const std::string &option) {
  return custom_light_capability_mask_from_device1_type_option(option);
}

inline std::string curtain_position_preset_from_custom_config(const std::string &custom_type, uint32_t custom_value) {
  if (custom_type == "cover") {
    return custom_value == 0 ? "0%" : "100%";
  }
  return "100%";
}

inline uint32_t get_device_inferred_light_capability_mask(size_t slot) {
  if (!is_configurable_device_slot(slot)) {
    return 1;
  }
  load_device_config_if_needed(slot);
  const InferredNormalDeviceConfig &config = device_inferred_config(slot);
  if (!config.valid || ha_domain_from_entity_id(config.entity_id) != "light") {
    return 1;
  }
  return normalize_light_capability_mask(config.inferred_light_capability_mask);
}

inline uint32_t normalize_light_capability_mask(uint32_t mask) {
  mask &= 0xF;
  if (mask == 0 || (mask & 0x1) == 0) {
    return 1;
  }
  return mask;
}

inline uint32_t normalize_device_custom_value(const std::string &custom_type, uint32_t value) {
  return custom_type == "cover" ? (value == 0 ? 0u : 100u) : normalize_light_capability_mask(value);
}

inline size_t build_light_type_options(uint32_t inferred_mask, const char **options, size_t max_options) {
  if (options == nullptr || max_options == 0) {
    return 0;
  }
  inferred_mask = normalize_light_capability_mask(inferred_mask);
  size_t count = 0;
  options[count++] = "Light (On/Off)";
  if ((inferred_mask & light_type_option_capability_mask("Dimmable Light")) ==
          light_type_option_capability_mask("Dimmable Light") &&
      count < max_options) {
    options[count++] = "Dimmable Light";
  }
  if ((inferred_mask & light_type_option_capability_mask("Color Temperature Light")) ==
          light_type_option_capability_mask("Color Temperature Light") &&
      count < max_options) {
    options[count++] = "Color Temperature Light";
  }
  if ((inferred_mask & light_type_option_capability_mask("RGB Light")) ==
          light_type_option_capability_mask("RGB Light") &&
      count < max_options) {
    options[count++] = "RGB Light";
  }
  return count;
}

inline bool option_list_contains(const char *const *options, size_t count, const std::string &option) {
  if (options == nullptr) {
    return false;
  }
  for (size_t i = 0; i < count; i++) {
    if (options[i] != nullptr && option == options[i]) {
      return true;
    }
  }
  return false;
}

inline std::string light_type_option_from_custom_mask(uint32_t custom_mask) {
  custom_mask = normalize_light_capability_mask(custom_mask);
  if (custom_mask == light_type_option_capability_mask("RGB Light")) {
    return "RGB Light";
  }
  if (custom_mask == light_type_option_capability_mask("Color Temperature Light")) {
    return "Color Temperature Light";
  }
  if (custom_mask == light_type_option_capability_mask("Dimmable Light")) {
    return "Dimmable Light";
  }
  return "Light (On/Off)";
}

inline std::string preferred_device_type_option(size_t slot, const char *const *options, size_t count) {
  load_device_config_if_needed(slot);
  const std::string &custom_type = cached_device_custom_type(slot);
  const std::string entity_id = cached_device_entity_id(slot);
  const std::string domain = ha_domain_from_entity_id(entity_id);
  std::string preferred;
  if (domain == "switch") {
    preferred = custom_type == "plug" ? "Plug" : "Switch";
  } else if (domain == "cover") {
    preferred = cached_device_custom_light_capability_mask(slot) == 0 ? "Cover (Preset Off)" : "Cover (Preset On)";
  } else if (domain == "light") {
    preferred = light_type_option_from_custom_mask(cached_device_custom_light_capability_mask(slot));
  } else if (domain == "automation") {
    preferred = "Automation";
  } else {
    preferred = "Switch";
  }
  if (option_list_contains(options, count, preferred)) {
    return preferred;
  }
  if (count == 0) {
    return "Switch";
  }
  return options[count - 1];
}

inline bool apply_device_type_options(size_t slot, esphome::select::Select *my_select) {
  if (!is_configurable_device_slot(slot) || my_select == nullptr) {
    return false;
  }
  load_device_config_if_needed(slot);

  const std::string entity_id = cached_device_entity_id(slot);
  const std::string domain = ha_domain_from_entity_id(entity_id);
  const char *options[4]{};
  size_t option_count = 0;
  if (domain == "switch") {
    options[option_count++] = "Switch";
    options[option_count++] = "Plug";
  } else if (domain == "cover") {
    options[option_count++] = "Cover (Preset Off)";
    options[option_count++] = "Cover (Preset On)";
  } else if (domain == "light") {
    option_count = build_light_type_options(get_device_inferred_light_capability_mask(slot), options, 4);
  } else if (domain == "automation") {
    options[option_count++] = "Automation";
  } else {
    options[option_count++] = "none";
  }

  esphome::FixedVector<const char *> new_options;
  new_options.init(option_count);
  for (size_t i = 0; i < option_count; i++) {
    new_options.push_back(options[i]);
  }
  my_select->traits.set_options(new_options);

  const std::string selected_option = preferred_device_type_option(slot, options, option_count);
  const auto selected_index = my_select->index_of(selected_option);
  my_select->publish_state(selected_index.has_value() ? selected_index.value() : static_cast<size_t>(0));
  save_device_custom_config(slot, custom_type_from_device1_type_option(selected_option),
                            custom_light_capability_mask_from_device1_type_option(selected_option));
  ESP_LOGI(TAG, "updated %s_type with %zu option(s), selected=%s", device_slot_label(slot).c_str(), option_count,
           selected_option.c_str());
  return true;
}

inline uint32_t effective_custom_light_capability_mask(uint32_t inferred_mask, uint32_t custom_mask) {
  inferred_mask = normalize_light_capability_mask(inferred_mask);
  custom_mask = normalize_light_capability_mask(custom_mask);
  return normalize_light_capability_mask((inferred_mask & custom_mask) | 0x1);
}

inline void set_cached_device_custom_config(size_t slot, const std::string &custom_type,
                                            uint32_t custom_light_capability_mask) {
  if (!is_configurable_device_slot(slot)) {
    return;
  }
  cached_device_custom_type(slot) = custom_type.empty() ? "switch" : custom_type;
  cached_device_custom_light_capability_mask(slot) =
      normalize_device_custom_value(cached_device_custom_type(slot), custom_light_capability_mask);
}

inline void set_cached_device1_custom_config(const std::string &custom_type, uint32_t custom_light_capability_mask) {
  set_cached_device_custom_config(0, custom_type, custom_light_capability_mask);
}

inline void set_cached_device2_custom_config(const std::string &custom_type, uint32_t custom_light_capability_mask) {
  set_cached_device_custom_config(1, custom_type, custom_light_capability_mask);
}

inline void set_cached_device_custom_config_from_entity_default(size_t slot, const std::string &entity_id) {
  const std::string type = normal_ui_type_from_entity_id(entity_id);
  set_cached_device_custom_config(slot, type, type == "cover" ? 100 : 1);
}

inline void set_cached_device1_custom_config_from_entity_default(const std::string &entity_id) {
  set_cached_device_custom_config_from_entity_default(0, entity_id);
}

inline void set_cached_device2_custom_config_from_entity_default(const std::string &entity_id) {
  set_cached_device_custom_config_from_entity_default(1, entity_id);
}

inline void normalize_kelvin_range_for_storage(int min_kelvin, int max_kelvin, uint16_t *out_min, uint16_t *out_max) {
  if (min_kelvin < 0 || max_kelvin < 0) {
    *out_min = 0;
    *out_max = 0;
    return;
  }
  if (min_kelvin > max_kelvin) {
    const int tmp = min_kelvin;
    min_kelvin = max_kelvin;
    max_kelvin = tmp;
  }
  *out_min = static_cast<uint16_t>(min_kelvin);
  *out_max = static_cast<uint16_t>(max_kelvin);
}

inline InferredNormalDeviceConfig default_device_config_from_entity_id(size_t slot, const std::string &entity_id) {
  InferredNormalDeviceConfig config{};
  config.valid = !entity_id.empty();
  config.entity_id = entity_id;
  const std::string inferred_type = normal_ui_type_from_entity_id(entity_id);
  if (!is_configurable_device_slot(slot)) {
    return config;
  }
  if (cached_device_custom_type(slot).empty()) {
    set_cached_device_custom_config_from_entity_default(slot, entity_id);
  }
  config.type = cached_device_custom_type(slot).empty() ? inferred_type : cached_device_custom_type(slot);
  const std::string local_display_name = local_hmi_gpio_switch_display_name_from_entity_id(entity_id);
  config.title = local_display_name.empty() ? entity_id : local_display_name;
  config.curtain_position_preset =
      curtain_position_preset_from_custom_config(config.type, cached_device_custom_light_capability_mask(slot));
  config.custom_type = config.type;
  config.custom_light_capability_mask = cached_device_custom_light_capability_mask(slot);
  config.inferred_light_capability_mask = 1;
  config.light_capability_mask =
      effective_custom_light_capability_mask(config.inferred_light_capability_mask, config.custom_light_capability_mask);
  config.color_temperature_min_kelvin = inferred_type == "light" ? 2000 : -1;
  config.color_temperature_max_kelvin = inferred_type == "light" ? 6000 : -1;
  return config;
}

inline InferredNormalDeviceConfig default_device1_config_from_entity_id(const std::string &entity_id) {
  return default_device_config_from_entity_id(0, entity_id);
}

inline InferredNormalDeviceConfig default_device2_config_from_entity_id(const std::string &entity_id) {
  return default_device_config_from_entity_id(1, entity_id);
}

inline InferredNormalDeviceConfig default_device3_config_from_entity_id(const std::string &entity_id) {
  return default_device_config_from_entity_id(2, entity_id);
}

inline InferredNormalDeviceConfig default_device4_config_from_entity_id(const std::string &entity_id) {
  return default_device_config_from_entity_id(3, entity_id);
}

inline InferredNormalDeviceConfig default_device5_config_from_entity_id(const std::string &entity_id) {
  return default_device_config_from_entity_id(4, entity_id);
}

inline InferredNormalDeviceConfig default_device6_config_from_entity_id(const std::string &entity_id) {
  return default_device_config_from_entity_id(5, entity_id);
}

inline InferredNormalDeviceConfig default_device7_config_from_entity_id(const std::string &entity_id) {
  return default_device_config_from_entity_id(6, entity_id);
}

inline InferredNormalDeviceConfig default_device8_config_from_entity_id(const std::string &entity_id) {
  return default_device_config_from_entity_id(7, entity_id);
}

inline InferredNormalDeviceConfig default_device9_config_from_entity_id(const std::string &entity_id) {
  return default_device_config_from_entity_id(8, entity_id);
}

inline InferredNormalDeviceConfig default_device10_config_from_entity_id(const std::string &entity_id) {
  return default_device_config_from_entity_id(9, entity_id);
}

inline bool persist_device_config_to_normal_ui_storage(size_t slot, const InferredNormalDeviceConfig &config,
                                                       const char *source) {
  if (!is_configurable_device_slot(slot) || normal_ui_storage == nullptr) {
    return false;
  }

  std::array<esphome::onx_storage::NormalUiDeviceConfig, 10> devices{};
  std::array<InferredNormalDeviceConfig, CONFIGURABLE_DEVICE_SLOT_COUNT> compact_configs{};
  std::array<size_t, CONFIGURABLE_DEVICE_SLOT_COUNT> ui_slots{};
  ui_slots.fill(static_cast<size_t>(-1));
  size_t device_count = 0;
  for (size_t source_slot = 0; source_slot < CONFIGURABLE_DEVICE_SLOT_COUNT; source_slot++) {
    load_device_config_if_needed(source_slot);

    InferredNormalDeviceConfig source_config{};
    if (source_slot == slot) {
      source_config = config;
    } else if (device_inferred_config(source_slot).valid) {
      source_config = device_inferred_config(source_slot);
      source_config.type = cached_device_custom_type(source_slot).empty() ? source_config.type
                                                                          : cached_device_custom_type(source_slot);
      source_config.custom_type = cached_device_custom_type(source_slot);
      source_config.custom_light_capability_mask = cached_device_custom_light_capability_mask(source_slot);
      source_config.light_capability_mask = effective_custom_light_capability_mask(
          source_config.inferred_light_capability_mask, source_config.custom_light_capability_mask);
    } else if (!cached_device_entity_id(source_slot).empty()) {
      source_config = default_device_config_from_entity_id(source_slot, cached_device_entity_id(source_slot));
    }

    source_config.curtain_position_preset = curtain_position_preset_from_custom_config(
        source_config.type, source_config.custom_light_capability_mask);
    if (!source_config.valid || source_config.entity_id.empty() || device_count >= devices.size()) {
      continue;
    }
    devices[device_count] = {source_config.title, source_config.type, source_config.entity_id};
    compact_configs[device_count] = source_config;
    ui_slots[source_slot] = device_count;
    device_count++;
  }

  if (!normal_ui_storage->set_normal_ui_config(devices)) {
    ESP_LOGW(TAG, "failed to persist %s inferred normal UI config: source=%s entity_id=%s",
             device_slot_label(slot).c_str(), source, config.entity_id.c_str());
    return false;
  }

  for (size_t source_slot = 0; source_slot < CONFIGURABLE_DEVICE_SLOT_COUNT; source_slot++) {
    const size_t ui_slot = ui_slots[source_slot];
    if (ui_slot == static_cast<size_t>(-1)) {
      continue;
    }
    const InferredNormalDeviceConfig &source_config = compact_configs[ui_slot];
    normal_ui_storage->set_normal_ui_device_extra_u32(
        ui_slot, 3, source_config.curtain_position_preset == "100%" ? 100u : 0u);
    uint16_t normalized_min = 0;
    uint16_t normalized_max = 0;
    normalize_kelvin_range_for_storage(source_config.color_temperature_min_kelvin,
                                       source_config.color_temperature_max_kelvin,
                                       &normalized_min, &normalized_max);
    if (source_config.type == "light") {
      esphome::onx_storage::NormalLightStatePatch light_patch{};
      light_patch.has_capability_mask = true;
      light_patch.capability_mask = source_config.inferred_light_capability_mask;
      light_patch.has_custom_capability_mask = true;
      light_patch.custom_capability_mask = source_config.custom_light_capability_mask;
      light_patch.has_color_temperature_range = true;
      light_patch.min_color_temperature_kelvin = normalized_min;
      light_patch.max_color_temperature_kelvin = normalized_max;
      normal_ui_storage->normal_device_slot(ui_slot).update_light(light_patch);
    }
  }

  uint16_t normalized_min = 0;
  uint16_t normalized_max = 0;
  normalize_kelvin_range_for_storage(config.color_temperature_min_kelvin, config.color_temperature_max_kelvin,
                                     &normalized_min, &normalized_max);
  const size_t ui_slot = ui_slots[slot];
  // ESP_LOGI(TAG,
  //          "persisted %s normal UI config: source=%s ui_slot=%d device_count=%u title=%s type=%s entity_id=%s "
  //          "light_capability_mask=%u custom_type=%s custom_light_capability_mask=%u "
  //          "color_temperature_min_kelvin=%d color_temperature_max_kelvin=%d "
  //          "normalized_color_temperature_min_kelvin=%u normalized_color_temperature_max_kelvin=%u "
  //          "curtain_position_preset=%s",
  //          device_slot_label(slot).c_str(), source,
  //          ui_slot == static_cast<size_t>(-1) ? -1 : static_cast<int>(ui_slot + 1),
  //          static_cast<unsigned>(device_count), config.title.c_str(), config.type.c_str(), config.entity_id.c_str(),
  //          static_cast<unsigned>(config.light_capability_mask), config.custom_type.c_str(),
  //          static_cast<unsigned>(config.custom_light_capability_mask), config.color_temperature_min_kelvin,
  //          config.color_temperature_max_kelvin, static_cast<unsigned>(normalized_min),
  //          static_cast<unsigned>(normalized_max), config.curtain_position_preset.c_str());
  return true;
}

inline size_t normal_ui_slot_for_device_config_slot(size_t slot) {
  if (!is_configurable_device_slot(slot) || normal_ui_storage == nullptr) {
    return static_cast<size_t>(-1);
  }
  load_device_config_if_needed(slot);
  const std::string entity_id = cached_device_entity_id(slot);
  if (entity_id.empty()) {
    return static_cast<size_t>(-1);
  }
  size_t compact_slot = 0;
  for (size_t source_slot = 0; source_slot < slot; source_slot++) {
    load_device_config_if_needed(source_slot);
    if (!cached_device_entity_id(source_slot).empty()) {
      compact_slot++;
    }
  }
  return compact_slot < CONFIGURABLE_DEVICE_SLOT_COUNT ? compact_slot : static_cast<size_t>(-1);
}

inline bool persist_device1_config_to_normal_ui_storage(const InferredNormalDeviceConfig &config, const char *source) {
  return persist_device_config_to_normal_ui_storage(0, config, source);
}

inline bool persist_device2_config_to_normal_ui_storage(const InferredNormalDeviceConfig &config, const char *source) {
  return persist_device_config_to_normal_ui_storage(1, config, source);
}

inline bool persist_device3_config_to_normal_ui_storage(const InferredNormalDeviceConfig &config, const char *source) {
  return persist_device_config_to_normal_ui_storage(2, config, source);
}

inline bool persist_device4_config_to_normal_ui_storage(const InferredNormalDeviceConfig &config, const char *source) {
  return persist_device_config_to_normal_ui_storage(3, config, source);
}

inline bool persist_device5_config_to_normal_ui_storage(const InferredNormalDeviceConfig &config, const char *source) {
  return persist_device_config_to_normal_ui_storage(4, config, source);
}

inline bool apply_device_config_to_normal_ui_storage(size_t slot) {
  if (!is_configurable_device_slot(slot)) {
    return false;
  }
  InferredNormalDeviceConfig &config = device_inferred_config(slot);
  if (config.valid) {
    config.type = cached_device_custom_type(slot).empty() ? config.type : cached_device_custom_type(slot);
    config.custom_type = cached_device_custom_type(slot);
    config.custom_light_capability_mask = cached_device_custom_light_capability_mask(slot);
    config.light_capability_mask = effective_custom_light_capability_mask(
        config.inferred_light_capability_mask, cached_device_custom_light_capability_mask(slot));
    config.curtain_position_preset =
        curtain_position_preset_from_custom_config(config.type, cached_device_custom_light_capability_mask(slot));
    return persist_device_config_to_normal_ui_storage(slot, config, "runtime");
  }
  load_device_config_if_needed(slot);
  if (cached_device_entity_id(slot).empty()) {
    return false;
  }
  return persist_device_config_to_normal_ui_storage(
      slot, default_device_config_from_entity_id(slot, cached_device_entity_id(slot)), "default");
}

inline bool apply_device1_config_to_normal_ui_storage() {
  return apply_device_config_to_normal_ui_storage(0);
}

inline bool apply_device2_config_to_normal_ui_storage() {
  return apply_device_config_to_normal_ui_storage(1);
}

inline bool apply_device3_config_to_normal_ui_storage() {
  return apply_device_config_to_normal_ui_storage(2);
}

inline bool apply_device4_config_to_normal_ui_storage() {
  return apply_device_config_to_normal_ui_storage(3);
}

inline bool apply_device5_config_to_normal_ui_storage() {
  return apply_device_config_to_normal_ui_storage(4);
}

inline bool apply_device6_config_to_normal_ui_storage() {
  return apply_device_config_to_normal_ui_storage(5);
}

inline bool apply_device7_config_to_normal_ui_storage() {
  return apply_device_config_to_normal_ui_storage(6);
}

inline bool apply_device8_config_to_normal_ui_storage() {
  return apply_device_config_to_normal_ui_storage(7);
}

inline bool apply_device9_config_to_normal_ui_storage() {
  return apply_device_config_to_normal_ui_storage(8);
}

inline bool apply_device10_config_to_normal_ui_storage() {
  return apply_device_config_to_normal_ui_storage(9);
}

inline void set_normal_ui_storage(esphome::onx_storage::OnxStorage *storage) {
  normal_ui_storage = storage;
  if (normal_ui_storage == nullptr) {
    return;
  }
  for (size_t slot = 0; slot < CONFIGURABLE_DEVICE_SLOT_COUNT; slot++) {
    load_device_config_if_needed(slot);
    if (!cached_device_entity_id(slot).empty() &&
        normal_ui_storage->get_normal_ui_device_entity_id(slot) != cached_device_entity_id(slot)) {
      const InferredNormalDeviceConfig &config = device_inferred_config(slot);
      if (config.valid) {
        persist_device_config_to_normal_ui_storage(slot, config, "restored");
      } else {
        persist_device_config_to_normal_ui_storage(
            slot, default_device_config_from_entity_id(slot, cached_device_entity_id(slot)), "default");
      }
    }
  }
}

inline void ensure_device_light_runtime_pref_ready(size_t slot) {
  if (!is_configurable_device_slot(slot) || device_light_runtime_pref_ready(slot)) {
    return;
  }
  if (esphome::global_preferences == nullptr) {
    ESP_LOGW(TAG, "preferences backend is unavailable; %s light runtime config will stay in RAM only",
             device_slot_label(slot).c_str());
    return;
  }
  device_light_runtime_pref(slot) =
      esphome::global_preferences->make_preference<StoredDeviceLightRuntimeConfig>(
          device_light_runtime_pref_key(slot), true);
  device_light_runtime_pref_ready(slot) = true;
}

inline bool persist_device_light_runtime_config(size_t slot, const InferredNormalDeviceConfig &config) {
  if (!is_configurable_device_slot(slot) || !config.valid || ha_domain_from_entity_id(config.entity_id) != "light") {
    return false;
  }
  ensure_device_light_runtime_pref_ready(slot);
  if (!device_light_runtime_pref_ready(slot)) {
    return false;
  }

  StoredDeviceLightRuntimeConfig stored{};
  stored.magic = device_light_runtime_pref_key(slot);
  copy_string_to_fixed_buffer(config.entity_id, stored.entity_id);
  stored.inferred_light_capability_mask = normalize_light_capability_mask(config.inferred_light_capability_mask);
  stored.color_temperature_min_kelvin = static_cast<uint16_t>(std::clamp(config.color_temperature_min_kelvin, 0, 65535));
  stored.color_temperature_max_kelvin = static_cast<uint16_t>(std::clamp(config.color_temperature_max_kelvin, 0, 65535));

  bool saved = device_light_runtime_pref(slot).save(&stored);
  if (saved && esphome::global_preferences != nullptr) {
    esphome::global_preferences->sync();
  }
  ESP_LOGI(TAG,
           "%s light runtime config saved: entity_id=%s inferred_mask=%u color_temperature_min_kelvin=%u "
           "color_temperature_max_kelvin=%u",
           device_slot_label(slot).c_str(), stored.entity_id,
           static_cast<unsigned>(stored.inferred_light_capability_mask),
           static_cast<unsigned>(stored.color_temperature_min_kelvin),
           static_cast<unsigned>(stored.color_temperature_max_kelvin));
  return saved;
}

inline void log_device_config_inferred_from_entity_state(size_t slot, const std::string &entity_id,
                                                         JsonObjectConst state_obj) {
  JsonObjectConst attrs = ha_entity_attrs(state_obj);
  if (attrs.isNull()) {
    ESP_LOGW(TAG, "%s subscribed entity state missing attributes: entity_id=%s",
             device_slot_label(slot).c_str(), entity_id.c_str());
    return;
  }

  const std::string domain = ha_domain_from_entity_id(entity_id);
  std::string type = domain;
  std::string storage_type = domain;
  if (domain == "cover") {
    type = "cover";
    storage_type = "curtain";
  } else if (domain == "switch") {
    type = "switch";
    storage_type = "switch";
  } else if (domain == "light") {
    type = "light";
    storage_type = "light";
  }

  const std::string registry_title = ha_entity_title_from_registry_cache(entity_id);
  const std::string title = !registry_title.empty() ? registry_title : entity_id;
  uint32_t light_capability_mask = 0x1;
  int min_kelvin = -1;
  int max_kelvin = -1;
  if (domain == "light") {
    light_capability_mask = infer_light_capability_mask(attrs);
    min_kelvin = ha_min_color_temp_kelvin(attrs);
    max_kelvin = ha_max_color_temp_kelvin(attrs);
  }
  if (!is_configurable_device_slot(slot)) {
    return;
  }
  if (cached_device_custom_type(slot).empty()) {
    set_cached_device_custom_config_from_entity_default(slot, entity_id);
  }
  const std::string effective_type = cached_device_custom_type(slot).empty() ? type : cached_device_custom_type(slot);
  const uint32_t effective_light_capability_mask =
      effective_custom_light_capability_mask(light_capability_mask, cached_device_custom_light_capability_mask(slot));

  ESP_LOGI(TAG,
           "%s inferred config from websocket state: title=%s type=%s storage_type=%s entity_id=%s "
           "light_capability_mask=%u custom_type=%s custom_light_capability_mask=%u "
           "effective_type=%s effective_light_capability_mask=%u "
           "color_temperature_min_kelvin=%d color_temperature_max_kelvin=%d curtain_position_preset=%s",
           device_slot_label(slot).c_str(), title.c_str(), type.c_str(), storage_type.c_str(), entity_id.c_str(),
           static_cast<unsigned>(light_capability_mask), cached_device_custom_type(slot).c_str(),
           static_cast<unsigned>(cached_device_custom_light_capability_mask(slot)), effective_type.c_str(),
           static_cast<unsigned>(effective_light_capability_mask), min_kelvin, max_kelvin,
           curtain_position_preset_from_custom_config(effective_type, cached_device_custom_light_capability_mask(slot)).c_str());

  InferredNormalDeviceConfig &config = device_inferred_config(slot);
  config.valid = true;
  config.title = title;
  config.type = effective_type;
  config.entity_id = entity_id;
  config.inferred_light_capability_mask = light_capability_mask;
  config.light_capability_mask = effective_light_capability_mask;
  config.custom_type = cached_device_custom_type(slot);
  config.custom_light_capability_mask = cached_device_custom_light_capability_mask(slot);
  config.color_temperature_min_kelvin = min_kelvin;
  config.color_temperature_max_kelvin = max_kelvin;
  config.curtain_position_preset =
      curtain_position_preset_from_custom_config(effective_type, cached_device_custom_light_capability_mask(slot));
  persist_device_config_to_normal_ui_storage(slot, config, "websocket");
  persist_device_light_runtime_config(slot, config);
}

inline void log_device1_config_inferred_from_entity_state(const std::string &entity_id, JsonObjectConst state_obj) {
  log_device_config_inferred_from_entity_state(0, entity_id, state_obj);
}

inline void log_device2_config_inferred_from_entity_state(const std::string &entity_id, JsonObjectConst state_obj) {
  log_device_config_inferred_from_entity_state(1, entity_id, state_obj);
}

inline int brightness_255_to_percent(int brightness) {
  if (brightness < 0) {
    return -1;
  }
  if (brightness > 255) {
    brightness = 255;
  }
  return std::clamp((brightness * 100 + 127) / 255, 0, 100);
}

inline uint32_t curtain_state_code_from_ha_state(const std::string &state) {
  std::string normalized = state;
  for (char &ch : normalized) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  if (normalized == "open") {
    return 1;
  }
  if (normalized == "closed") {
    return 2;
  }
  if (normalized == "opening") {
    return 3;
  }
  if (normalized == "closing") {
    return 4;
  }
  if (normalized == "stopped") {
    return 5;
  }
  return 0;
}

inline std::string ha_json_variant_to_string(JsonVariantConst value) {
  if (value.isNull()) {
    return "";
  }
  if (value.is<const char *>()) {
    const char *text = value.as<const char *>();
    return text != nullptr ? std::string(text) : std::string();
  }
  if (value.is<std::string>()) {
    return value.as<std::string>();
  }
  std::string serialized;
  serializeJson(value, serialized);
  if (serialized.size() >= 2 && serialized.front() == '"' && serialized.back() == '"') {
    return serialized.substr(1, serialized.size() - 2);
  }
  return "";
}

inline std::string ha_entity_state_text(JsonObjectConst state_obj) {
  std::string state_json;
  serializeJson(state_obj, state_json);
  ESP_LOGI(TAG, "state_obj=%s", state_json.c_str());

  JsonVariantConst short_state = state_obj["s"];
  JsonVariantConst full_state = state_obj["state"];
  std::string short_state_json;
  std::string full_state_json;
  serializeJson(short_state, short_state_json);
  serializeJson(full_state, full_state_json);

  std::string state_text = ha_json_variant_to_string(short_state);
  if (state_text.empty()) {
    state_text = ha_json_variant_to_string(full_state);
  }

  ESP_LOGI(TAG, "device1 websocket state lookup: has_s=%s s_json=%s has_state=%s state_json=%s parsed=%s",
           short_state.isNull() ? "NO" : "YES",
           short_state_json.c_str(),
           full_state.isNull() ? "NO" : "YES",
           full_state_json.c_str(),
           state_text.empty() ? "(empty)" : state_text.c_str());
  return state_text;
}

inline JsonObjectConst ha_entity_attrs(JsonObjectConst state_obj) {
  JsonObjectConst attrs = state_obj["a"].as<JsonObjectConst>();
  if (!attrs.isNull()) {
    return attrs;
  }
  return state_obj["attributes"].as<JsonObjectConst>();
}

inline bool update_device_light_config_from_http_response(size_t slot, const std::string &entity_id,
                                                          const std::string &body) {
  if (!is_configurable_device_slot(slot) || ha_domain_from_entity_id(entity_id) != "light") {
    return false;
  }

  load_device_config_if_needed(slot);
  if (cached_device_entity_id(slot).empty() || cached_device_entity_id(slot) != entity_id) {
    ESP_LOGI(TAG, "%s light HTTP response ignored for stale entity_id=%s current=%s",
             device_slot_label(slot).c_str(), entity_id.c_str(), cached_device_entity_id(slot).c_str());
    return false;
  }

  JsonDocument doc = make_psram_json_document();
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    ESP_LOGW(TAG, "%s light HTTP state JSON parse failed: entity_id=%s err=%s",
             device_slot_label(slot).c_str(), entity_id.c_str(), error.c_str());
    return false;
  }

  JsonObjectConst root = doc.as<JsonObjectConst>();
  JsonObjectConst attrs = ha_entity_attrs(root);
  if (attrs.isNull()) {
    ESP_LOGW(TAG, "%s light HTTP state missing attributes: entity_id=%s", device_slot_label(slot).c_str(),
             entity_id.c_str());
    return false;
  }
  const JsonArrayConst supported_color_modes = ha_supported_color_modes(attrs);
  ESP_LOGI(TAG, "%s light HTTP state attrs: entity_id=%s supported_color_modes=%s",
           device_slot_label(slot).c_str(), entity_id.c_str(),
           ha_supported_color_modes_to_string(supported_color_modes).c_str());

  InferredNormalDeviceConfig &config = device_inferred_config(slot);
  if (!config.valid || config.entity_id != entity_id) {
    config = default_device_config_from_entity_id(slot, entity_id);
  }

  config.type = cached_device_custom_type(slot).empty() ? "light" : cached_device_custom_type(slot);
  config.custom_type = cached_device_custom_type(slot);
  config.custom_light_capability_mask = cached_device_custom_light_capability_mask(slot);
  config.inferred_light_capability_mask = infer_light_capability_mask(attrs);
  config.light_capability_mask =
      effective_custom_light_capability_mask(config.inferred_light_capability_mask, config.custom_light_capability_mask);
  config.color_temperature_min_kelvin = ha_min_color_temp_kelvin(attrs);
  config.color_temperature_max_kelvin = ha_max_color_temp_kelvin(attrs);

  ESP_LOGI(TAG,
           "%s light HTTP state applied: entity_id=%s inferred_mask=%u effective_mask=%u "
           "color_temperature_min_kelvin=%d color_temperature_max_kelvin=%d",
           device_slot_label(slot).c_str(), entity_id.c_str(),
           static_cast<unsigned>(config.inferred_light_capability_mask),
           static_cast<unsigned>(config.light_capability_mask), config.color_temperature_min_kelvin,
           config.color_temperature_max_kelvin);
  persist_device_config_to_normal_ui_storage(slot, config, "http_request");
  persist_device_light_runtime_config(slot, config);
  return true;
}

inline bool load_device_light_runtime_if_needed(size_t slot) {
  if (!is_configurable_device_slot(slot)) {
    return false;
  }
  ensure_device_light_runtime_pref_ready(slot);
  if (!device_light_runtime_pref_ready(slot)) {
    return false;
  }

  StoredDeviceLightRuntimeConfig stored{};
  if (!device_light_runtime_pref(slot).load(&stored) || stored.magic != device_light_runtime_pref_key(slot)) {
    return false;
  }
  stored.entity_id[MAX_HA_ENTITY_ID_LENGTH] = '\0';
  if (std::string(stored.entity_id) != cached_device_entity_id(slot)) {
    ESP_LOGI(TAG, "%s light runtime config ignored for stale entity_id=%s current=%s",
             device_slot_label(slot).c_str(), stored.entity_id, cached_device_entity_id(slot).c_str());
    return false;
  }

  InferredNormalDeviceConfig &config = device_inferred_config(slot);
  config.valid = !cached_device_entity_id(slot).empty();
  config.entity_id = cached_device_entity_id(slot);
  config.inferred_light_capability_mask = normalize_light_capability_mask(stored.inferred_light_capability_mask);
  config.color_temperature_min_kelvin = static_cast<int>(stored.color_temperature_min_kelvin);
  config.color_temperature_max_kelvin = static_cast<int>(stored.color_temperature_max_kelvin);
  config.light_capability_mask =
      effective_custom_light_capability_mask(config.inferred_light_capability_mask, cached_device_custom_light_capability_mask(slot));
  ESP_LOGI(TAG,
           "%s light runtime config restored: entity_id=%s inferred_mask=%u color_temperature_min_kelvin=%d "
           "color_temperature_max_kelvin=%d",
           device_slot_label(slot).c_str(), cached_device_entity_id(slot).c_str(),
           static_cast<unsigned>(config.inferred_light_capability_mask), config.color_temperature_min_kelvin,
           config.color_temperature_max_kelvin);
  return true;
}

inline void load_device_config_if_needed(size_t slot) {
  if (!is_configurable_device_slot(slot) || device_config_loaded(slot)) {
    return;
  }
  device_config_loaded(slot) = true;
  cached_device_entity_id(slot).clear();
  ensure_device_config_pref_ready(slot);
  if (!device_config_pref_ready(slot)) {
    return;
  }

  StoredDevice1Config stored{};
  if (!device_config_pref(slot).load(&stored) || stored.magic != device_config_magic(slot)) {
    return;
  }
  stored.entity_id[MAX_HA_ENTITY_ID_LENGTH] = '\0';
  stored.custom_type[sizeof(stored.custom_type) - 1] = '\0';
  cached_device_entity_id(slot) = stored.entity_id;
  cached_device_custom_type(slot) = stored.custom_type;
  cached_device_custom_light_capability_mask(slot) =
      normalize_device_custom_value(cached_device_custom_type(slot), stored.custom_light_capability_mask);
  if (cached_device_custom_type(slot).empty() && !cached_device_entity_id(slot).empty()) {
    const std::string type = normal_ui_type_from_entity_id(cached_device_entity_id(slot));
    cached_device_custom_type(slot) = type.empty() ? "switch" : type;
    cached_device_custom_light_capability_mask(slot) = 1;
  }
  if (!cached_device_entity_id(slot).empty()) {
    device_inferred_config(slot) = default_device_config_from_entity_id(slot, cached_device_entity_id(slot));
    load_device_light_runtime_if_needed(slot);
  } else {
    device_inferred_config(slot) = {};
  }
}

inline bool apply_device_light_feedback(size_t slot, JsonObjectConst state_obj, const std::string &entity_id) {
  if (normal_ui_storage == nullptr) {
    return false;
  }
  const size_t ui_slot = normal_ui_slot_for_device_config_slot(slot);
  if (ui_slot == static_cast<size_t>(-1)) {
    ESP_LOGI(TAG, "%s light feedback ignored because it has no compact normal UI slot: entity_id=%s",
             device_slot_label(slot).c_str(), entity_id.c_str());
    return false;
  }
  bool changed = false;
  const std::string state_text = ha_entity_state_text(state_obj);
  if (!state_text.empty()) {
    const bool is_on = state_text == "on";
#ifdef USE_ESP32
    ha_ws_device_power_known(slot) = true;
    ha_ws_device_power(slot) = is_on;
#endif
    esphome::onx_storage::NormalLightStatePatch patch{};
    patch.has_power = true;
    patch.is_on = is_on;
    const bool saved = normal_ui_storage->normal_device_slot(ui_slot).update_light(patch);
    ESP_LOGI(TAG, "%s websocket light power feedback: ui_slot=%u entity_id=%s ha_state=%s saved=%s stored=%s",
             device_slot_label(slot).c_str(), static_cast<unsigned>(ui_slot + 1),
             entity_id.c_str(), state_text.c_str(), saved ? "YES" : "NO",
             normal_ui_storage->get_normal_ui_device_state(ui_slot) ? "ON" : "OFF");
    changed = true;
  }

  JsonObjectConst attrs = ha_entity_attrs(state_obj);
  if (!attrs.isNull()) {
    esphome::onx_storage::NormalLightStatePatch patch{};
    if (attrs["brightness"].is<int>()) {
      const int brightness_pct = brightness_255_to_percent(attrs["brightness"].as<int>());
      if (brightness_pct >= 0 &&
          !is_normal_light_feedback_suppressed(ui_slot, NORMAL_LIGHT_FEEDBACK_BRIGHTNESS)) {
        patch.has_brightness_pct = true;
        patch.brightness_pct = static_cast<uint8_t>(std::clamp(brightness_pct, 0, 100));
        changed = true;
      }
    }
    if (attrs["color_temp_kelvin"].is<int>() &&
        !is_normal_light_feedback_suppressed(ui_slot, NORMAL_LIGHT_FEEDBACK_COLOR_TEMPERATURE)) {
      int kelvin = attrs["color_temp_kelvin"].as<int>();
      auto light_state = normal_ui_storage->normal_device_slot(ui_slot).light();
      int cct_min_kelvin = static_cast<int>(light_state.min_color_temperature_kelvin);
      int cct_max_kelvin = static_cast<int>(light_state.max_color_temperature_kelvin);
      if (cct_min_kelvin > 0 && cct_max_kelvin > 0 && cct_min_kelvin > cct_max_kelvin) {
        const int tmp = cct_min_kelvin;
        cct_min_kelvin = cct_max_kelvin;
        cct_max_kelvin = tmp;
      }
      if (cct_min_kelvin > 0 && cct_max_kelvin > 0) {
        kelvin = std::clamp(kelvin, cct_min_kelvin, cct_max_kelvin);
      }
      patch.has_color_temperature_kelvin = true;
      patch.color_temperature_kelvin = static_cast<uint16_t>(std::clamp(kelvin, 0, 65535));
      changed = true;
    }
    JsonArrayConst hs = attrs["hs_color"].as<JsonArrayConst>();
    if (!is_normal_light_feedback_suppressed(ui_slot, NORMAL_LIGHT_FEEDBACK_RGB) &&
        !hs.isNull() && hs.size() >= 2 && hs[0].is<float>() && hs[1].is<float>()) {
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
      patch.has_rgb_tick = true;
      patch.rgb_tick = static_cast<uint8_t>(rgb_tick);
      changed = true;
      ESP_LOGI(TAG, "%s websocket light hs feedback: entity_id=%s hue=%d saturation=%d rgb_tick=%d",
               device_slot_label(slot).c_str(), entity_id.c_str(), hue, saturation, rgb_tick);
    }
    if (patch.has_brightness_pct || patch.has_color_temperature_kelvin || patch.has_rgb_tick ||
        patch.has_capability_mask || patch.has_color_temperature_range || patch.has_power) {
      normal_ui_storage->normal_device_slot(ui_slot).update_light(patch);
    }
  }

  if (changed) {
    auto light_state = normal_ui_storage->normal_device_slot(ui_slot).light();
    ESP_LOGI(TAG, "%s websocket light feedback: ui_slot=%u entity_id=%s state=%s brightness=%u kelvin=%u rgb_tick=%u",
             device_slot_label(slot).c_str(), static_cast<unsigned>(ui_slot + 1), entity_id.c_str(),
             !state_text.empty() ? state_text.c_str() : "(unchanged)",
             static_cast<unsigned>(light_state.brightness_pct),
             static_cast<unsigned>(light_state.color_temperature_kelvin),
             static_cast<unsigned>(light_state.rgb_tick));
  }
  return changed;
}

inline bool ha_ws_payload_contains(const char *data, int len, const char *needle) {
  if (data == nullptr || len <= 0 || needle == nullptr || needle[0] == '\0') {
    return false;
  }
  const char *end = data + len;
  const size_t needle_len = strlen(needle);
  return std::search(data, end, needle, needle + needle_len) != end;
}

inline bool ha_ws_send_initial_ping() {
  if (ha_ws_client == nullptr || !esp_websocket_client_is_connected(ha_ws_client)) {
    return false;
  }
  static const uint8_t empty_payload = 0;
  const int sent = esp_websocket_client_send_with_opcode(ha_ws_client, WS_TRANSPORT_OPCODES_PING,
                                                        &empty_payload, 0, pdMS_TO_TICKS(1000));
  if (sent < 0) {
    ESP_LOGW(TAG, "failed to send HA websocket initial ping: result=%d", sent);
    return false;
  }
  ha_ws_initial_ping_sent = true;
  ESP_LOGI(TAG, "HA websocket initial ping sent");
  return true;
}

inline bool ha_ws_send_auth_frame(const char *reason) {
  if (ha_ws_client == nullptr) {
    ESP_LOGW(TAG, "HA websocket auth send skipped: client is null reason=%s",
             reason != nullptr ? reason : "unknown");
    return false;
  }
  load_ha_token_if_needed();
  if (cached_ha_token.empty()) {
    ESP_LOGW(TAG, "HA websocket auth send skipped: token is empty reason=%s",
             reason != nullptr ? reason : "unknown");
    return false;
  }
  std::string auth = "{\"type\":\"auth\",\"access_token\":\"";
  auth += cached_ha_token;
  auth += "\"}";
  const uint32_t send_started_ms = millis();
  const int sent = esp_websocket_client_send_text(ha_ws_client, auth.c_str(), auth.size(), pdMS_TO_TICKS(1000));
  ESP_LOGI(TAG,
           "HA websocket auth send complete: attempt=%u reason=%s result_bytes=%d expected_bytes=%u "
           "elapsed_ms=%u transport_connected=%s since_connected_ms=%u",
           static_cast<unsigned>(ha_ws_registry_fetch_attempt),
           reason != nullptr ? reason : "unknown", sent, static_cast<unsigned>(auth.size()),
           static_cast<unsigned>(millis() - send_started_ms),
           ha_ws_client != nullptr && esp_websocket_client_is_connected(ha_ws_client) ? "YES" : "NO",
           static_cast<unsigned>(millis() - ha_ws_transport_connected_ms));
  if (sent < 0 || static_cast<size_t>(sent) != auth.size()) {
    ESP_LOGW(TAG, "failed to send complete HA websocket auth: result_bytes=%d expected_bytes=%u", sent,
             static_cast<unsigned>(auth.size()));
    return false;
  }
  ha_ws_auth_sent = true;
  return true;
}

inline void ha_ws_handle_text_payload(const char *data, int len) {
  if (data == nullptr || len <= 0) {
    return;
  }
  if (!ha_ws_registry_fetch_active) {
    ESP_LOGI(TAG, "HA websocket payload ignored because registry fetch is not active");
    return;
  }
  if (ha_ws_payload_contains(data, len, "\"type\":\"auth_required\"") ||
      ha_ws_payload_contains(data, len, "\"type\": \"auth_required\"")) {
    ha_ws_auth_required_ms = millis();
    load_ha_token_if_needed();
    ESP_LOGI(TAG,
             "HA websocket auth_required received: attempt=%u payload_bytes=%u token_length=%u "
             "transport_connected=%s",
             static_cast<unsigned>(ha_ws_registry_fetch_attempt), static_cast<unsigned>(len),
             static_cast<unsigned>(cached_ha_token.size()),
             ha_ws_client != nullptr && esp_websocket_client_is_connected(ha_ws_client) ? "YES" : "NO");
    if (ha_ws_auth_sent) {
      ESP_LOGI(TAG, "HA websocket auth_required received after auth was already sent; skip duplicate auth");
      return;
    }
    if (!ha_ws_send_auth_frame("auth_required")) {
      ha_ws_finish_registry_fetch("auth send failed");
    }
    return;
  }
  if (ha_ws_payload_contains(data, len, "\"type\":\"auth_ok\"") ||
      ha_ws_payload_contains(data, len, "\"type\": \"auth_ok\"")) {
    ha_ws_authenticated = true;
    ha_ws_auth_invalid = false;
    ha_ws_backoff_ms = 2000;
    set_ha_token_status(HaTokenStatus::VALID);
    ESP_LOGI(TAG,
             "HA websocket authenticated: attempt=%u payload_bytes=%u since_auth_required_ms=%u",
             static_cast<unsigned>(ha_ws_registry_fetch_attempt), static_cast<unsigned>(len),
             static_cast<unsigned>(millis() - ha_ws_auth_required_ms));
    if (ha_ws_entity_list_pending) {
      if (!ha_ws_send_area_list_request()) {
        ha_ws_finish_registry_fetch("area request send failed");
      }
    }
    return;
  }
  if (ha_ws_payload_contains(data, len, "\"type\":\"auth_invalid\"") ||
      ha_ws_payload_contains(data, len, "\"type\": \"auth_invalid\"")) {
    ha_ws_authenticated = false;
    ha_ws_auth_invalid = true;
    ha_ws_stop_requested = true;
    set_ha_token_status(HaTokenStatus::INVALID);
    ESP_LOGW(TAG,
             "HA websocket authentication failed: attempt=%u payload_bytes=%u since_fetch_ms=%u "
             "since_auth_required_ms=%u",
             static_cast<unsigned>(ha_ws_registry_fetch_attempt), static_cast<unsigned>(len),
             static_cast<unsigned>(millis() - ha_ws_registry_fetch_started_ms),
             static_cast<unsigned>(millis() - ha_ws_auth_required_ms));
    ha_ws_finish_registry_fetch("auth invalid");
    return;
  }
  if ((ha_ws_area_list_request_id != 0 || ha_ws_entity_list_request_id != 0) &&
      (ha_ws_payload_contains(data, len, "\"type\":\"result\"") ||
       ha_ws_payload_contains(data, len, "\"type\": \"result\""))) {
    const uint32_t parse_started_ms = millis();
    ESP_LOGI(TAG, "HA websocket result JSON parse begin: payload_bytes=%u area_request_id=%u entity_request_id=%u",
             static_cast<unsigned>(len), static_cast<unsigned>(ha_ws_area_list_request_id),
             static_cast<unsigned>(ha_ws_entity_list_request_id));
    log_ha_registry_memory("before result JSON parse");
    JsonDocument doc = make_psram_json_document();
    DeserializationError error = deserializeJson(doc, data, static_cast<size_t>(len));
    if (error) {
      ESP_LOGE(TAG, "HA websocket result JSON parse failed: %s", error.c_str());
      ha_ws_finish_registry_fetch("result JSON parse failed");
      return;
    }
    ESP_LOGI(TAG, "HA websocket result JSON parse complete: elapsed_ms=%u overflowed=%s",
             static_cast<unsigned>(millis() - parse_started_ms), doc.overflowed() ? "YES" : "NO");
    log_ha_registry_memory("after result JSON parse");
    const uint32_t response_id = doc["id"] | 0;
    if (response_id != ha_ws_area_list_request_id && response_id != ha_ws_entity_list_request_id) {
      return;
    }
    const bool is_area_response = response_id == ha_ws_area_list_request_id;
    if (is_area_response) {
      ha_ws_area_list_request_id = 0;
    } else {
      ha_ws_entity_list_request_id = 0;
    }
    if (!(doc["success"] | false)) {
      ESP_LOGW(TAG, "HA websocket registry request failed");
      ha_ws_finish_registry_fetch("registry request failed");
      return;
    }
    JsonVariantConst result_variant = doc["result"];
    if (is_area_response) {
      JsonArrayConst areas = result_variant.as<JsonArrayConst>();
      if (areas.isNull()) {
        ESP_LOGW(TAG, "HA websocket area list response missing result array");
        ha_ws_finish_registry_fetch("area response missing result");
        return;
      }
      ESP_LOGI(TAG, "HA websocket area list response has %u item(s)", static_cast<unsigned>(areas.size()));
      if (!parse_and_store_ha_areas(areas)) {
        ha_ws_finish_registry_fetch("area storage failed");
        return;
      }
      ha_ws_entity_list_send_pending = true;
      return;
    }

    JsonArrayConst result = result_variant.as<JsonArrayConst>();
    if (result.isNull()) {
      result = result_variant["entities"].as<JsonArrayConst>();
    }
    if (result.isNull()) {
      ESP_LOGW(TAG, "HA websocket entity list response missing entities array");
      ha_ws_finish_registry_fetch("entity response missing result");
      return;
    }
    ESP_LOGI(TAG, "HA websocket entity list response has %u display item(s)", static_cast<unsigned>(result.size()));
    if (parse_and_store_ha_entity_registry_entities(result)) {
      ESP_LOGI(TAG, "HA entity select refresh sequence begin: device_slots=%u",
               static_cast<unsigned>(CONFIGURABLE_DEVICE_SLOT_COUNT));
      log_ha_registry_memory("before all select refreshes");
      bool options_changed = false;
      for (size_t slot = 0; slot < CONFIGURABLE_DEVICE_SLOT_COUNT; slot++) {
        options_changed = apply_device_config_options(slot, ha_ws_device_entity_select(slot)) || options_changed;
      }
      ESP_LOGI(TAG, "HA standby weather select refresh begin");
      options_changed = apply_standby_weather_config_options(ha_ws_standby_weather_entity_select) || options_changed;
      ESP_LOGI(TAG, "HA entity select refresh sequence complete: options_changed=%s",
               options_changed ? "YES" : "NO");
      log_ha_registry_memory("after all select refreshes");
      if (options_changed) {
        ESP_LOGI(TAG, "HA entity options changed; disconnecting API clients for option refresh");
        disconnect_all_clients_for_refresh(ha_ws_entity_api_server);
        ESP_LOGI(TAG, "HA API client disconnect request complete");
      }
    }
    ha_ws_finish_registry_fetch("registry fetch complete");
  }

}

inline void ha_ws_handle_text_event(const esp_websocket_event_data_t *data) {
  if (data == nullptr || data->data_ptr == nullptr || data->data_len <= 0) {
    return;
  }
  if (data->payload_offset == 0) {
    if (ha_ws_rx_text_buffer.capacity() > HA_WS_RX_RESERVE_SIZE) {
      ha_ws_release_rx_text_buffer();
    } else {
      ha_ws_rx_text_buffer.clear();
    }
    if (data->payload_len > 0) {
      ha_ws_rx_text_buffer.reserve(std::min<size_t>(data->payload_len, HA_WS_RX_RESERVE_SIZE));
    }
  }
  if (ha_ws_rx_text_buffer.size() + data->data_len > MAX_HA_WS_TEXT_MESSAGE_LENGTH) {
    ESP_LOGW(TAG, "HA websocket payload too large; drop message len=%d offset=%d total=%d", data->data_len,
             data->payload_offset, data->payload_len);
    ha_ws_release_rx_text_buffer();
    ha_ws_finish_registry_fetch("payload too large");
    return;
  }
  ha_ws_rx_text_buffer.append(data->data_ptr, data->data_len);
  const size_t received_end = static_cast<size_t>(data->payload_offset) + static_cast<size_t>(data->data_len);
  if (received_end < static_cast<size_t>(data->payload_len)) {
    return;
  }
  ha_ws_handle_text_payload(ha_ws_rx_text_buffer.data(), ha_ws_rx_text_buffer.size());
  ha_ws_release_rx_text_buffer();
}

inline void ha_ws_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  auto *data = static_cast<esp_websocket_event_data_t *>(event_data);
  switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
      ha_ws_transport_connected_ms = millis();
      ESP_LOGI(TAG, "HA websocket transport connected: attempt=%u since_fetch_ms=%u client_connected=%s",
               static_cast<unsigned>(ha_ws_registry_fetch_attempt),
               static_cast<unsigned>(ha_ws_transport_connected_ms - ha_ws_registry_fetch_started_ms),
               ha_ws_client != nullptr && esp_websocket_client_is_connected(ha_ws_client) ? "YES" : "NO");
      if (ha_ws_registry_fetch_active && !ha_ws_initial_ping_sent) {
        ha_ws_send_initial_ping();
      }
      break;
    case WEBSOCKET_EVENT_DISCONNECTED:
    case WEBSOCKET_EVENT_CLOSED:
      ESP_LOGW(TAG,
               "HA websocket transport disconnected: attempt=%u event_id=%d fetch_active=%s authenticated=%s "
               "since_fetch_ms=%u since_connected_ms=%u since_auth_required_ms=%u error_type=%d "
               "handshake_status=%d tls_esp_err=0x%x tls_stack_err=%d tls_flags=%d socket_errno=%d",
               static_cast<unsigned>(ha_ws_registry_fetch_attempt), static_cast<int>(event_id),
               ha_ws_registry_fetch_active ? "YES" : "NO", ha_ws_authenticated ? "YES" : "NO",
               static_cast<unsigned>(millis() - ha_ws_registry_fetch_started_ms),
               static_cast<unsigned>(millis() - ha_ws_transport_connected_ms),
               static_cast<unsigned>(millis() - ha_ws_auth_required_ms),
               data != nullptr ? static_cast<int>(data->error_handle.error_type) : -1,
               data != nullptr ? data->error_handle.esp_ws_handshake_status_code : 0,
               data != nullptr ? static_cast<unsigned>(data->error_handle.esp_tls_last_esp_err) : 0,
               data != nullptr ? data->error_handle.esp_tls_stack_err : 0,
               data != nullptr ? data->error_handle.esp_tls_cert_verify_flags : 0,
               data != nullptr ? data->error_handle.esp_transport_sock_errno : 0);
      ha_ws_authenticated = false;
      ha_ws_disconnected_seen = true;
      break;
    case WEBSOCKET_EVENT_DATA:
      if (data != nullptr && data->op_code == 0x01) {
        ha_ws_handle_text_event(data);
      }
      break;
    case WEBSOCKET_EVENT_ERROR:
      ESP_LOGW(TAG,
               "HA websocket transport error: attempt=%u fetch_active=%s authenticated=%s since_fetch_ms=%u "
               "error_type=%d handshake_status=%d tls_esp_err=0x%x tls_stack_err=%d tls_flags=%d "
               "socket_errno=%d",
               static_cast<unsigned>(ha_ws_registry_fetch_attempt),
               ha_ws_registry_fetch_active ? "YES" : "NO", ha_ws_authenticated ? "YES" : "NO",
               static_cast<unsigned>(millis() - ha_ws_registry_fetch_started_ms),
               data != nullptr ? static_cast<int>(data->error_handle.error_type) : -1,
               data != nullptr ? data->error_handle.esp_ws_handshake_status_code : 0,
               data != nullptr ? static_cast<unsigned>(data->error_handle.esp_tls_last_esp_err) : 0,
               data != nullptr ? data->error_handle.esp_tls_stack_err : 0,
               data != nullptr ? data->error_handle.esp_tls_cert_verify_flags : 0,
               data != nullptr ? data->error_handle.esp_transport_sock_errno : 0);
      ha_ws_authenticated = false;
      ha_ws_disconnected_seen = true;
      break;
    default:
      break;
  }
}

inline bool start_ha_websocket_client(const std::string &uri) {
  ha_ws_uri = uri;
  esp_websocket_client_config_t config = {};
  config.uri = ha_ws_uri.c_str();
  config.disable_auto_reconnect = true;
  config.buffer_size = HA_WS_CLIENT_BUFFER_SIZE;
  config.task_stack = 16384;
  config.network_timeout_ms = HA_WS_NETWORK_TIMEOUT_MS;
  config.ping_interval_sec = 20;
  config.pingpong_timeout_sec = 10;
  config.keep_alive_enable = true;
  ESP_LOGI(TAG,
           "HA websocket client init: attempt=%u uri=%s buffer_size=%u task_stack=%u "
           "network_timeout_ms=%u ping_interval_sec=%u pingpong_timeout_sec=%u",
           static_cast<unsigned>(ha_ws_registry_fetch_attempt), uri.c_str(),
           static_cast<unsigned>(config.buffer_size), static_cast<unsigned>(config.task_stack),
           static_cast<unsigned>(config.network_timeout_ms),
           static_cast<unsigned>(config.ping_interval_sec), static_cast<unsigned>(config.pingpong_timeout_sec));
  ha_ws_client = esp_websocket_client_init(&config);
  if (ha_ws_client == nullptr) {
    ESP_LOGW(TAG, "failed to create HA websocket client");
    return false;
  }
  esp_err_t err = esp_websocket_register_events(ha_ws_client, WEBSOCKET_EVENT_ANY, ha_ws_event_handler, nullptr);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "failed to register HA websocket events: %s", esp_err_to_name(err));
    ha_ws_destroy_client();
    return false;
  }
  ha_ws_auth_sent = false;
  err = esp_websocket_client_start(ha_ws_client);
  ESP_LOGI(TAG, "HA websocket client start returned: attempt=%u result=%s (0x%x)",
           static_cast<unsigned>(ha_ws_registry_fetch_attempt), esp_err_to_name(err),
           static_cast<unsigned>(err));
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "failed to start HA websocket client: %s", esp_err_to_name(err));
    ha_ws_destroy_client();
    return false;
  }
  ha_ws_started = true;
  ha_ws_authenticated = false;
  ha_ws_disconnected_seen = false;
  ha_ws_stop_requested = false;
  ESP_LOGI(TAG, "HA websocket connecting to %s", uri.c_str());
  return true;
}

inline bool maintain_ha_websocket(const std::string &host, bool wifi_connected, bool *connected_out) {
  process_pending_api_client_refresh();

  const bool previous_connected = ha_ws_reported_connected;
  const uint32_t now_ms = millis();

  if (!ha_ws_registry_fetch_active) {
    if (ha_ws_client != nullptr || ha_ws_stop_requested || ha_ws_disconnected_seen) {
      ha_ws_destroy_client();
    }
    ha_ws_set_connected(false, connected_out);
    return previous_connected != ha_ws_reported_connected;
  }

  if (!wifi_connected) {
    ha_ws_finish_registry_fetch("wifi disconnected");
    ha_ws_destroy_client();
    ha_ws_set_connected(false, connected_out);
    return previous_connected != ha_ws_reported_connected;
  }

  if (now_ms - ha_ws_registry_fetch_started_ms > 30000UL) {
    ESP_LOGW(TAG, "HA websocket registry fetch timed out");
    ha_ws_finish_registry_fetch("timeout");
    ha_ws_destroy_client();
    ha_ws_set_connected(false, connected_out);
    return previous_connected != ha_ws_reported_connected;
  }

  if (ha_ws_stop_requested || ha_ws_disconnected_seen) {
    ha_ws_destroy_client();
    ha_ws_set_connected(false, connected_out);
    if (ha_ws_registry_fetch_active) {
      ha_ws_finish_registry_fetch(ha_ws_auth_invalid ? "auth invalid" : "websocket disconnected");
    }
    return previous_connected != ha_ws_reported_connected;
  }

  if (ha_ws_authenticated) {
    if (ha_ws_entity_list_send_pending && ha_ws_area_list_request_id == 0 && ha_ws_entity_list_request_id == 0) {
      ha_ws_entity_list_send_pending = false;
      if (!ha_ws_send_entity_list_request()) {
        ha_ws_finish_registry_fetch("entity request send failed");
        ha_ws_destroy_client();
        ha_ws_set_connected(false, connected_out);
        return previous_connected != ha_ws_reported_connected;
      }
    }
    ha_ws_set_connected(true, connected_out);
    return previous_connected != ha_ws_reported_connected;
  }

  ha_ws_set_connected(false, connected_out);
  return previous_connected != ha_ws_reported_connected;
}

inline bool request_ha_entity_list_over_websocket(const std::string &host,
                                                  esphome::select::Select *device1_select,
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
                                                  esphome::api::APIServer *api_server) {
  ESP_LOGI(TAG, "HA websocket registry fetch requested: host=%s active=%s",
           host.c_str(), ha_ws_registry_fetch_active ? "YES" : "NO");
  log_ha_registry_memory("registry fetch request");
  if (ha_ws_registry_fetch_active) {
    ESP_LOGI(TAG, "HA websocket registry fetch is already active; ignore duplicate request");
    return true;
  }

  ha_ws_device1_entity_select = device1_select;
  ha_ws_device2_entity_select = device2_select;
  ha_ws_device3_entity_select = device3_select;
  ha_ws_device4_entity_select = device4_select;
  ha_ws_device5_entity_select = device5_select;
  ha_ws_device6_entity_select = device6_select;
  ha_ws_device7_entity_select = device7_select;
  ha_ws_device8_entity_select = device8_select;
  ha_ws_device9_entity_select = device9_select;
  ha_ws_device10_entity_select = device10_select;
  ha_ws_standby_weather_entity_select = standby_weather_select;
  ha_ws_entity_api_server = api_server;
  load_ha_token_if_needed();
  if (cached_ha_token.empty()) {
    set_ha_token_status(HaTokenStatus::NOT_CONFIGURED);
    ESP_LOGW(TAG, "HA token is not configured; skip websocket entity fetch");
    return false;
  }
  std::string uri = build_ha_websocket_url(host);
  if (uri.empty()) {
    ESP_LOGW(TAG, "HA websocket registry fetch skipped because host is invalid");
    return false;
  }

  ha_ws_destroy_client();
  ha_ws_registry_fetch_active = true;
  ha_ws_registry_fetch_started_ms = millis();
  ha_ws_transport_connected_ms = ha_ws_registry_fetch_started_ms;
  ha_ws_auth_required_ms = ha_ws_registry_fetch_started_ms;
  ha_ws_registry_fetch_attempt++;
  ha_ws_entity_list_pending = true;
  ha_ws_entity_list_send_pending = false;
  ha_ws_area_list_request_id = 0;
  ha_ws_entity_list_request_id = 0;
  ha_ws_auth_sent = false;
  ha_ws_initial_ping_sent = false;
  ha_ws_auth_invalid = false;
  ha_ws_disconnected_seen = false;
  ha_ws_stop_requested = false;
  ha_ws_backoff_ms = 2000;
  ha_ws_set_connected(false, nullptr);

  if (!start_ha_websocket_client(uri)) {
    ha_ws_finish_registry_fetch("websocket start failed");
    ha_ws_destroy_client();
    return false;
  }
  ESP_LOGI(TAG, "HA websocket registry fetch started");
  log_ha_registry_memory("registry fetch started");
  return true;
}

inline bool request_ha_entity_list_over_websocket(const std::string &host,
                                                  esphome::select::Select *device1_select,
                                                  esphome::select::Select *device2_select,
                                                  esphome::select::Select *standby_weather_select,
                                                  esphome::api::APIServer *api_server) {
  return request_ha_entity_list_over_websocket(host, device1_select, device2_select, nullptr, nullptr, nullptr,
                                               nullptr, nullptr, nullptr, nullptr, nullptr, standby_weather_select,
                                               api_server);
}

inline bool request_ha_entity_list_over_websocket(const std::string &host,
                                                  esphome::select::Select *device1_select,
                                                  esphome::select::Select *device2_select,
                                                  esphome::api::APIServer *api_server) {
  return request_ha_entity_list_over_websocket(host, device1_select, device2_select, nullptr, nullptr, nullptr, nullptr,
                                               nullptr, nullptr, nullptr, nullptr, nullptr, api_server);
}
#else
inline bool maintain_ha_websocket(const std::string &host, bool wifi_connected, bool *connected_out) {
  if (connected_out != nullptr) {
    *connected_out = false;
  }
  return false;
}

inline bool request_ha_entity_list_over_websocket(const std::string &host,
                                                  esphome::select::Select *device1_select,
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
                                                  esphome::api::APIServer *api_server) {
  ESP_LOGW(TAG, "HA websocket entity fetch is only supported on ESP32");
  return false;
}

inline bool request_ha_entity_list_over_websocket(const std::string &host,
                                                  esphome::select::Select *device1_select,
                                                  esphome::select::Select *device2_select,
                                                  esphome::select::Select *standby_weather_select,
                                                  esphome::api::APIServer *api_server) {
  return request_ha_entity_list_over_websocket(host, device1_select, device2_select, nullptr, nullptr, nullptr,
                                               nullptr, nullptr, nullptr, nullptr, nullptr, standby_weather_select,
                                               api_server);
}

inline bool request_ha_entity_list_over_websocket(const std::string &host,
                                                  esphome::select::Select *device1_select,
                                                  esphome::select::Select *device2_select,
                                                  esphome::api::APIServer *api_server) {
  return request_ha_entity_list_over_websocket(host, device1_select, device2_select, nullptr, nullptr, nullptr, nullptr,
                                               nullptr, nullptr, nullptr, nullptr, nullptr, api_server);
}
#endif

#ifndef USE_ESP32
inline void log_ha_registry_memory(const char *stage) {}
#endif

inline bool is_supported_normal_device_entity_id(const char *entity_id) {
  if (entity_id == nullptr) {
    return false;
  }
  if (is_local_hmi_gpio_switch_preset_option(entity_id)) {
    return true;
  }
  return strncmp(entity_id, "switch.", 7) == 0 || strncmp(entity_id, "light.", 6) == 0 ||
         strncmp(entity_id, "cover.", 6) == 0 || strncmp(entity_id, "automation.", 11) == 0;
}

inline bool is_local_hmi_gpio_switch_entity_id(const std::string &entity_id) {
  return local_hmi_gpio_switch_io_from_entity_id(entity_id) >= 0;
}

inline bool is_supported_standby_weather_entity_id(const char *entity_id) {
  return entity_id != nullptr && strncmp(entity_id, "weather.", 8) == 0;
}

inline bool is_supported_ha_entity_id(const char *entity_id) {
  return is_supported_normal_device_entity_id(entity_id) || is_supported_standby_weather_entity_id(entity_id);
}

inline const char *ha_entity_type_from_id(const char *entity_id) {
  if (entity_id == nullptr) {
    return "unknown";
  }
  const char *dot = strchr(entity_id, '.');
  if (dot == nullptr) {
    return "unknown";
  }
  static char type[12];
  const size_t len = std::min<size_t>(dot - entity_id, sizeof(type) - 1);
  memcpy(type, entity_id, len);
  type[len] = '\0';
  return type;
}

inline bool save_ha_entities(const StoredHaEntities &entities) {
  ESP_LOGI(TAG, "HA entity flash save begin: requested_count=%u max_count=%u chunk_size=%u chunk_count=%u",
           static_cast<unsigned>(entities.count), static_cast<unsigned>(MAX_HA_ENTITY_COUNT),
           static_cast<unsigned>(HA_ENTITY_PREF_CHUNK_SIZE), static_cast<unsigned>(HA_ENTITY_PREF_CHUNK_COUNT));
  log_ha_registry_memory("before entity flash save");
  ensure_ha_entity_chunk_prefs_ready();
  cached_ha_entities() = entities;
  ha_entities_loaded = true;
  if (!ha_entity_chunk_prefs_ready) {
    ESP_LOGE(TAG, "HA entity flash save aborted: preferences backend is unavailable");
    return false;
  }
  const uint16_t total_count = std::min<uint16_t>(entities.count, MAX_HA_ENTITY_COUNT);
  bool saved = true;
  auto chunk = make_ram_allocated<StoredHaEntityChunk>();
  if (chunk == nullptr) {
    ESP_LOGE(TAG, "failed to allocate HA entity chunk save buffer (%zu bytes)", sizeof(StoredHaEntityChunk));
    return false;
  }
  for (size_t chunk_index = 0; chunk_index < HA_ENTITY_PREF_CHUNK_COUNT; chunk_index++) {
    *chunk = {};
    chunk->magic = HA_ENTITY_CHUNK_MAGIC;
    chunk->total_count = total_count;
    chunk->chunk_index = static_cast<uint16_t>(chunk_index);
    const size_t offset = chunk_index * HA_ENTITY_PREF_CHUNK_SIZE;
    if (offset < total_count) {
      chunk->count = static_cast<uint16_t>(std::min<size_t>(HA_ENTITY_PREF_CHUNK_SIZE, total_count - offset));
      for (size_t i = 0; i < chunk->count; i++) {
        chunk->entities[i] = entities.entities[offset + i];
      }
    }
    ESP_LOGI(TAG, "HA entity flash chunk write begin: chunk=%u/%u offset=%u count=%u bytes=%u",
             static_cast<unsigned>(chunk_index + 1), static_cast<unsigned>(HA_ENTITY_PREF_CHUNK_COUNT),
             static_cast<unsigned>(offset), static_cast<unsigned>(chunk->count),
             static_cast<unsigned>(sizeof(StoredHaEntityChunk)));
    const bool chunk_saved = ha_entity_chunk_prefs[chunk_index].save(chunk.get());
    ESP_LOGI(TAG, "HA entity flash chunk write end: chunk=%u result=%s",
             static_cast<unsigned>(chunk_index + 1), chunk_saved ? "OK" : "FAILED");
    saved = chunk_saved && saved;
  }
  if (saved && esphome::global_preferences != nullptr) {
    ESP_LOGI(TAG, "HA entity flash sync begin");
    esphome::global_preferences->sync();
    ESP_LOGI(TAG, "HA entity flash sync end");
  }
  log_ha_registry_memory("after entity flash save");
  ESP_LOGI(TAG, "HA entity flash save end: total_count=%u result=%s",
           static_cast<unsigned>(total_count), saved ? "OK" : "FAILED");
  return saved;
}

inline bool save_ha_areas(const StoredHaAreas &areas) {
  ensure_ha_areas_pref_ready();
  cached_ha_areas = areas;
  ha_areas_loaded = true;
  if (!ha_areas_pref_ready) {
    return false;
  }
  const bool saved = ha_areas_pref.save(&areas);
  if (saved && esphome::global_preferences != nullptr) {
    esphome::global_preferences->sync();
  }
  return saved;
}

inline std::string ha_entity_title_from_registry_cache(const std::string &entity_id) {
  if (entity_id.empty()) {
    return "";
  }
  const std::string local_display_name = local_hmi_gpio_switch_display_name_from_entity_id(entity_id);
  if (!local_display_name.empty()) {
    return local_display_name;
  }
  load_ha_entities_if_needed();
  const StoredHaEntities &entities = cached_ha_entities();
  for (size_t i = 0; i < entities.count; i++) {
    const StoredHaEntity &entity = entities.entities[i];
    if (entity_id == entity.entity_id && entity.friendly_name[0] != '\0') {
      return entity.friendly_name;
    }
  }
  return "";
}

inline bool save_device_config_entity_id(size_t slot, const std::string &raw_entity_id) {
  std::string entity_id = trim_copy(raw_entity_id);
  if (!entity_id.empty() && !is_supported_normal_device_entity_id(entity_id.c_str())) {
    ESP_LOGW(TAG, "%s config ignored unsupported entity: %s", device_slot_label(slot).c_str(), entity_id.c_str());
    return false;
  }

  load_device_config_if_needed(slot);
  if (cached_device_entity_id(slot) == entity_id) {
    return true;
  }

  StoredDevice1Config stored{};
  stored.magic = device_config_magic(slot);
  copy_string_to_fixed_buffer(entity_id, stored.entity_id);
  cached_device_entity_id(slot) = stored.entity_id;
  if (!cached_device_entity_id(slot).empty()) {
    if (cached_device_custom_type(slot).empty()) {
      set_cached_device_custom_config_from_entity_default(slot, cached_device_entity_id(slot));
    }
    device_inferred_config(slot) = default_device_config_from_entity_id(slot, cached_device_entity_id(slot));
  } else {
    device_inferred_config(slot) = {};
  }
  copy_string_to_fixed_buffer(cached_device_custom_type(slot), stored.custom_type);
  stored.custom_light_capability_mask = cached_device_custom_light_capability_mask(slot);
  device_config_loaded(slot) = true;

  ensure_device_config_pref_ready(slot);
  bool saved = false;
  if (device_config_pref_ready(slot)) {
    saved = device_config_pref(slot).save(&stored);
    if (saved && esphome::global_preferences != nullptr) {
      esphome::global_preferences->sync();
    }
  }
  ESP_LOGI(TAG, "%s config entity %s", device_slot_label(slot).c_str(),
           cached_device_entity_id(slot).empty() ? "cleared" : cached_device_entity_id(slot).c_str());
  persist_device_config_to_normal_ui_storage(slot, device_inferred_config(slot), "default");
  return saved;
}

inline bool save_device1_config_entity_id(const std::string &raw_entity_id) {
  return save_device_config_entity_id(0, raw_entity_id);
}

inline bool save_device2_config_entity_id(const std::string &raw_entity_id) {
  return save_device_config_entity_id(1, raw_entity_id);
}

inline bool save_device3_config_entity_id(const std::string &raw_entity_id) {
  return save_device_config_entity_id(2, raw_entity_id);
}

inline bool save_device4_config_entity_id(const std::string &raw_entity_id) {
  return save_device_config_entity_id(3, raw_entity_id);
}

inline bool save_device5_config_entity_id(const std::string &raw_entity_id) {
  return save_device_config_entity_id(4, raw_entity_id);
}

inline bool save_standby_weather_config_entity_id(const std::string &raw_entity_id) {
  std::string entity_id = trim_copy(raw_entity_id);
  if (!entity_id.empty() && !is_supported_standby_weather_entity_id(entity_id.c_str())) {
    ESP_LOGW(TAG, "standby weather config ignored unsupported entity: %s", entity_id.c_str());
    return false;
  }

  load_standby_weather_config_if_needed();
  if (cached_standby_weather_entity_id == entity_id) {
    return true;
  }

  StoredStandbyWeatherConfig stored{};
  stored.magic = STANDBY_WEATHER_CONFIG_MAGIC;
  copy_string_to_fixed_buffer(entity_id, stored.entity_id);
  cached_standby_weather_entity_id = stored.entity_id;
  standby_weather_config_loaded = true;

  ensure_standby_weather_config_pref_ready();
  bool saved = false;
  if (standby_weather_config_pref_ready) {
    saved = standby_weather_config_pref.save(&stored);
    if (saved && esphome::global_preferences != nullptr) {
      esphome::global_preferences->sync();
    }
  }
  ESP_LOGI(TAG, "standby weather config entity %s",
           cached_standby_weather_entity_id.empty() ? "cleared" : cached_standby_weather_entity_id.c_str());
  return saved;
}

inline bool save_device_custom_config(size_t slot, const std::string &custom_type, uint32_t custom_light_capability_mask) {
  load_device_config_if_needed(slot);
  set_cached_device_custom_config(slot, custom_type, custom_light_capability_mask);

  StoredDevice1Config stored{};
  stored.magic = device_config_magic(slot);
  copy_string_to_fixed_buffer(cached_device_entity_id(slot), stored.entity_id);
  copy_string_to_fixed_buffer(cached_device_custom_type(slot), stored.custom_type);
  stored.custom_light_capability_mask = cached_device_custom_light_capability_mask(slot);

  ensure_device_config_pref_ready(slot);
  bool saved = false;
  if (device_config_pref_ready(slot)) {
    saved = device_config_pref(slot).save(&stored);
    if (saved && esphome::global_preferences != nullptr) {
      esphome::global_preferences->sync();
    }
  }
  // ESP_LOGI(TAG, "%s custom config saved: custom_type=%s custom_light_capability_mask=%u entity_id=%s",
  //          device_slot_label(slot).c_str(), cached_device_custom_type(slot).c_str(),
  //          static_cast<unsigned>(cached_device_custom_light_capability_mask(slot)),
  //          cached_device_entity_id(slot).c_str());
  apply_device_config_to_normal_ui_storage(slot);
  return saved;
}

inline bool reset_device_config_storage(size_t slot) {
  if (!is_configurable_device_slot(slot)) {
    return false;
  }

  cached_device_entity_id(slot).clear();
  cached_device_custom_type(slot).clear();
  cached_device_custom_light_capability_mask(slot) = 1;
  device_inferred_config(slot) = {};
  device_config_loaded(slot) = true;

  device_config_option_strings(slot).fill(std::string{});
  staged_device_config_option_strings(slot).fill(std::string{});

  StoredDevice1Config stored{};
  stored.magic = device_config_magic(slot);
  stored.custom_light_capability_mask = 1;

  ensure_device_config_pref_ready(slot);
  if (!device_config_pref_ready(slot)) {
    return false;
  }
  const bool saved = device_config_pref(slot).save(&stored);
  if (saved && esphome::global_preferences != nullptr) {
    esphome::global_preferences->sync();
  }
  return saved;
}

inline bool reset_device_light_runtime_storage(size_t slot) {
  if (!is_configurable_device_slot(slot)) {
    return false;
  }

  StoredDeviceLightRuntimeConfig stored{};
  stored.magic = device_light_runtime_pref_key(slot);
  stored.inferred_light_capability_mask = 1;

  ensure_device_light_runtime_pref_ready(slot);
  if (!device_light_runtime_pref_ready(slot)) {
    return false;
  }
  const bool saved = device_light_runtime_pref(slot).save(&stored);
  if (saved && esphome::global_preferences != nullptr) {
    esphome::global_preferences->sync();
  }
  return saved;
}

inline bool reset_standby_weather_config_storage() {
  cached_standby_weather_entity_id.clear();
  standby_weather_config_loaded = true;
  standby_weather_config_options().fill(std::string{});
  staged_standby_weather_config_options().fill(std::string{});

  StoredStandbyWeatherConfig stored{};
  stored.magic = STANDBY_WEATHER_CONFIG_MAGIC;

  ensure_standby_weather_config_pref_ready();
  if (!standby_weather_config_pref_ready) {
    return false;
  }
  const bool saved = standby_weather_config_pref.save(&stored);
  if (saved && esphome::global_preferences != nullptr) {
    esphome::global_preferences->sync();
  }
  return saved;
}

#ifdef USE_ESP32
inline bool erase_custom_logic_normal_mode_nvs_keys() {
  nvs_handle_t handle;
  esp_err_t err = nvs_open("esphome", NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "normal mode reset: failed to open NVS namespace: %s", esp_err_to_name(err));
    return false;
  }

  bool ok = true;
  auto erase_key = [&](uint32_t key, const char *label) {
    char key_str[16];
    snprintf(key_str, sizeof(key_str), "%u", static_cast<unsigned>(key));
    esp_err_t erase_err = nvs_erase_key(handle, key_str);
    if (erase_err == ESP_OK) {
      ESP_LOGI(TAG, "normal mode reset: erased %s NVS key '%s'", label, key_str);
    } else if (erase_err == ESP_ERR_NVS_NOT_FOUND) {
      ESP_LOGI(TAG, "normal mode reset: %s NVS key '%s' already absent", label, key_str);
    } else {
      ESP_LOGW(TAG, "normal mode reset: failed to erase %s NVS key '%s': %s", label, key_str,
               esp_err_to_name(erase_err));
      ok = false;
    }
  };

  erase_key(HA_TOKEN_PREF_KEY, "HA token");
  for (size_t i = 0; i < HA_ENTITY_PREF_CHUNK_COUNT; i++) {
    erase_key(HA_ENTITY_CHUNK_PREF_KEY_BASE + i, "HA entity chunk");
  }
  erase_key(HA_AREAS_PREF_KEY, "HA areas");
  erase_key(HA_HOST_RESTORE_PREF_KEY, "HA host restore");
  erase_key(STANDBY_WEATHER_CONFIG_PREF_KEY, "standby weather config");
  erase_key(STANDBY_TIMEOUT_SECONDS_RESTORE_PREF_KEY, "standby timeout restore");
  for (size_t slot = 0; slot < CONFIGURABLE_DEVICE_SLOT_COUNT; slot++) {
    erase_key(device_config_pref_key(slot), "device config");
    erase_key(device_light_runtime_pref_key(slot), "device light runtime");
    erase_key(DEVICE_TYPE_RESTORE_PREF_KEYS[slot], "device type restore");
  }

  err = nvs_commit(handle);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "normal mode reset: failed to commit NVS erase: %s", esp_err_to_name(err));
    ok = false;
  }
  nvs_close(handle);
  return ok;
}
#else
inline bool erase_custom_logic_normal_mode_nvs_keys() { return true; }
#endif

inline void clear_custom_logic_normal_mode_ram_state() {
  cached_ha_token.clear();
  cached_ha_auth_header.clear();
  cached_ha_token_status = HaTokenStatus::NOT_CONFIGURED;
  ha_token_loaded = true;

  cached_ha_entities() = {};
  cached_ha_entities().magic = HA_ENTITIES_MAGIC;
  ha_entities_loaded = true;
  ha_entities_changed_since_last_parse_flag = false;

  cached_ha_areas = {};
  cached_ha_areas.magic = HA_AREAS_MAGIC;
  ha_areas_loaded = true;

  cached_standby_weather_entity_id.clear();
  standby_weather_config_loaded = true;
  standby_weather_config_options().fill(std::string{});
  staged_standby_weather_config_options().fill(std::string{});

  for (size_t slot = 0; slot < CONFIGURABLE_DEVICE_SLOT_COUNT; ++slot) {
    cached_device_entity_id(slot).clear();
    cached_device_custom_type(slot).clear();
    cached_device_custom_light_capability_mask(slot) = 1;
    device_inferred_config(slot) = {};
    device_config_loaded(slot) = true;
    device_config_option_strings(slot).fill(std::string{});
    staged_device_config_option_strings(slot).fill(std::string{});
  }
}

inline bool reset_normal_mode_preferences() {
  bool ok = true;

#ifdef USE_ESP32
  ha_ws_finish_registry_fetch("normal mode reset");
  ha_ws_destroy_client();
#endif

  clear_custom_logic_normal_mode_ram_state();
  ok = erase_custom_logic_normal_mode_nvs_keys() && ok;

  if (normal_ui_storage != nullptr) {
    ok = normal_ui_storage->reset_normal_mode_config() && ok;
  }

  ha_ws_config_revision++;
  ESP_LOGW(TAG, "normal mode preferences reset: %s", ok ? "ok" : "partial failure");
  return ok;
}

inline bool save_device1_custom_config(const std::string &custom_type, uint32_t custom_light_capability_mask) {
  return save_device_custom_config(0, custom_type, custom_light_capability_mask);
}

inline bool save_device2_custom_config(const std::string &custom_type, uint32_t custom_light_capability_mask) {
  return save_device_custom_config(1, custom_type, custom_light_capability_mask);
}

inline bool save_device3_custom_config(const std::string &custom_type, uint32_t custom_light_capability_mask) {
  return save_device_custom_config(2, custom_type, custom_light_capability_mask);
}

inline bool save_device4_custom_config(const std::string &custom_type, uint32_t custom_light_capability_mask) {
  return save_device_custom_config(3, custom_type, custom_light_capability_mask);
}

inline bool save_device5_custom_config(const std::string &custom_type, uint32_t custom_light_capability_mask) {
  return save_device_custom_config(4, custom_type, custom_light_capability_mask);
}

inline bool save_device6_custom_config(const std::string &custom_type, uint32_t custom_light_capability_mask) {
  return save_device_custom_config(5, custom_type, custom_light_capability_mask);
}

inline bool save_device7_custom_config(const std::string &custom_type, uint32_t custom_light_capability_mask) {
  return save_device_custom_config(6, custom_type, custom_light_capability_mask);
}

inline bool save_device8_custom_config(const std::string &custom_type, uint32_t custom_light_capability_mask) {
  return save_device_custom_config(7, custom_type, custom_light_capability_mask);
}

inline bool save_device9_custom_config(const std::string &custom_type, uint32_t custom_light_capability_mask) {
  return save_device_custom_config(8, custom_type, custom_light_capability_mask);
}

inline bool save_device10_custom_config(const std::string &custom_type, uint32_t custom_light_capability_mask) {
  return save_device_custom_config(9, custom_type, custom_light_capability_mask);
}

inline bool has_same_ha_areas(const StoredHaAreas &lhs, const StoredHaAreas &rhs) {
  if (lhs.count != rhs.count) {
    return false;
  }
  for (size_t i = 0; i < lhs.count; i++) {
    if (strcmp(lhs.areas[i].area_id, rhs.areas[i].area_id) != 0 ||
        strcmp(lhs.areas[i].name, rhs.areas[i].name) != 0) {
      return false;
    }
  }
  return true;
}

inline const char *get_ha_area_name_by_id(const char *area_id) {
  if (area_id == nullptr || area_id[0] == '\0') {
    return "";
  }
  load_ha_areas_if_needed();
  for (size_t i = 0; i < cached_ha_areas.count; i++) {
    if (strcmp(cached_ha_areas.areas[i].area_id, area_id) == 0) {
      return cached_ha_areas.areas[i].name;
    }
  }
  return "";
}

inline bool has_same_ha_entities(const StoredHaEntities &lhs, const StoredHaEntities &rhs) {
  if (lhs.count != rhs.count) {
    return false;
  }

  std::array<uint16_t, MAX_HA_ENTITY_COUNT> lhs_indexes{};
  std::array<uint16_t, MAX_HA_ENTITY_COUNT> rhs_indexes{};
  for (size_t i = 0; i < lhs.count; i++) {
    lhs_indexes[i] = static_cast<uint16_t>(i);
    rhs_indexes[i] = static_cast<uint16_t>(i);
  }

  auto compare_entity = [](const StoredHaEntity &a, const StoredHaEntity &b) {
    int area_name_cmp = strcmp(a.area_name, b.area_name);
    if (area_name_cmp != 0) {
      return area_name_cmp < 0;
    }
    int entity_name_cmp = strcmp(a.friendly_name, b.friendly_name);
    if (entity_name_cmp != 0) {
      return entity_name_cmp < 0;
    }
    return strcmp(a.entity_id, b.entity_id) < 0;
  };

  std::sort(lhs_indexes.begin(), lhs_indexes.begin() + lhs.count,
            [&](uint16_t a, uint16_t b) { return compare_entity(lhs.entities[a], lhs.entities[b]); });
  std::sort(rhs_indexes.begin(), rhs_indexes.begin() + rhs.count,
            [&](uint16_t a, uint16_t b) { return compare_entity(rhs.entities[a], rhs.entities[b]); });

  for (size_t i = 0; i < lhs.count; i++) {
    const StoredHaEntity &lhs_entity = lhs.entities[lhs_indexes[i]];
    const StoredHaEntity &rhs_entity = rhs.entities[rhs_indexes[i]];
    if (strcmp(lhs_entity.entity_id, rhs_entity.entity_id) != 0) {
      return false;
    }
    if (strcmp(lhs_entity.friendly_name, rhs_entity.friendly_name) != 0) {
      return false;
    }
    if (strcmp(lhs_entity.area_name, rhs_entity.area_name) != 0) {
      return false;
    }
  }
  return true;
}

inline bool parse_and_store_ha_areas(JsonArrayConst array) {
  ESP_LOGI(TAG, "HA area processing begin: response_count=%u storage_limit=%u",
           static_cast<unsigned>(array.size()), static_cast<unsigned>(MAX_HA_AREA_COUNT));
  log_ha_registry_memory("before area processing");
  StoredHaAreas areas{};
  areas.magic = HA_AREAS_MAGIC;

  size_t scanned_count = 0;
  size_t missing_id_count = 0;
  for (JsonObjectConst elem : array) {
    scanned_count++;
    const char *area_id = elem["area_id"];
    const char *name = elem["name"];
    if (area_id == nullptr || area_id[0] == '\0') {
      missing_id_count++;
      continue;
    }
    if (areas.count >= MAX_HA_AREA_COUNT) {
      ESP_LOGW(TAG, "HA area storage limit reached: %u", areas.count);
      break;
    }
    StoredHaArea &target = areas.areas[areas.count];
    copy_string_to_fixed_buffer(std::string(area_id), target.area_id);
    copy_string_to_fixed_buffer(std::string(name != nullptr ? name : ""), target.name);
    areas.count++;
  }

  ESP_LOGI(TAG, "HA area processing parsed: scanned=%u stored=%u missing_id=%u truncated=%s",
           static_cast<unsigned>(scanned_count), static_cast<unsigned>(areas.count),
           static_cast<unsigned>(missing_id_count),
           scanned_count < array.size() ? "YES" : "NO");
  load_ha_areas_if_needed();
  const bool changed = !has_same_ha_areas(cached_ha_areas, areas);
  ESP_LOGI(TAG, "HA area cache comparison complete: previous_count=%u new_count=%u changed=%s",
           static_cast<unsigned>(cached_ha_areas.count), static_cast<unsigned>(areas.count),
           changed ? "YES" : "NO");
  if (changed) {
    const bool saved = save_ha_areas(areas);
    ESP_LOGI(TAG, "HA area flash save complete: result=%s", saved ? "OK" : "FAILED");
  }
  ESP_LOGI(TAG, "stored %u HA area(s)", areas.count);
  for (size_t i = 0; i < areas.count; i++) {
    ESP_LOGI(TAG, "HA area: area_id=%s name=%s", areas.areas[i].area_id, areas.areas[i].name);
  }
  if (!changed) {
    ESP_LOGI(TAG, "HA area list is unchanged");
  }
  log_ha_registry_memory("after area processing");
  return true;
}

inline bool store_ha_entities_from_entity_registry_array(JsonArrayConst array) {
  ESP_LOGI(TAG, "HA entity processing begin: response_count=%u storage_limit=%u stored_struct_bytes=%u",
           static_cast<unsigned>(array.size()), static_cast<unsigned>(MAX_HA_ENTITY_COUNT),
           static_cast<unsigned>(sizeof(StoredHaEntities)));
  log_ha_registry_memory("before entity processing");
  auto entities = make_ram_allocated<StoredHaEntities>();
  if (entities == nullptr) {
    ESP_LOGE(TAG, "failed to allocate HA entity storage buffer (%zu bytes)", sizeof(StoredHaEntities));
    return false;
  }
  entities->magic = HA_ENTITIES_MAGIC;

  size_t scanned_count = 0;
  size_t missing_id_count = 0;
  size_t unsupported_count = 0;
  for (JsonObjectConst elem : array) {
    scanned_count++;
    std::string entity_id = ha_json_variant_to_string(elem["entity_id"]);
    if (entity_id.empty()) {
      entity_id = ha_json_variant_to_string(elem["ei"]);
    }
    if (entity_id.empty()) {
      missing_id_count++;
    }
    if (!is_supported_ha_entity_id(entity_id.c_str())) {
      unsupported_count++;
      if (scanned_count % 250 == 0) {
        ESP_LOGI(TAG, "HA entity processing progress: scanned=%u/%u stored=%u unsupported=%u missing_id=%u",
                 static_cast<unsigned>(scanned_count), static_cast<unsigned>(array.size()),
                 static_cast<unsigned>(entities->count), static_cast<unsigned>(unsupported_count),
                 static_cast<unsigned>(missing_id_count));
        log_ha_registry_memory("entity processing progress");
      }
      continue;
    }
    if (entities->count >= MAX_HA_ENTITY_COUNT) {
      ESP_LOGW(TAG, "HA entity storage limit reached: %u", static_cast<unsigned>(entities->count));
      break;
    }

    JsonVariantConst entity_name_variant = elem["en"];
    std::string entity_name_json;
    serializeJson(entity_name_variant, entity_name_json);
    const std::string entity_name = ha_json_variant_to_string(entity_name_variant);
    const std::string area_id = ha_json_variant_to_string(elem["ai"]);
    const char *area_name = get_ha_area_name_by_id(area_id.empty() ? nullptr : area_id.c_str());

    StoredHaEntity &target = entities->entities[entities->count];
    copy_string_to_fixed_buffer(entity_id, target.entity_id);
    copy_string_to_fixed_buffer(entity_name, target.friendly_name);
    copy_string_to_fixed_buffer(std::string(area_name != nullptr ? area_name : ""), target.area_name);
    entities->count++;
    if (entities->count % 50 == 0) {
      ESP_LOGI(TAG, "HA entity processing accepted: scanned=%u/%u stored=%u last_entity=%s",
               static_cast<unsigned>(scanned_count), static_cast<unsigned>(array.size()),
               static_cast<unsigned>(entities->count), target.entity_id);
      log_ha_registry_memory("entity accepted progress");
    }
  }

  ESP_LOGI(TAG,
           "HA entity processing parsed: response=%u scanned=%u stored=%u unsupported=%u missing_id=%u truncated=%s",
           static_cast<unsigned>(array.size()), static_cast<unsigned>(scanned_count),
           static_cast<unsigned>(entities->count), static_cast<unsigned>(unsupported_count),
           static_cast<unsigned>(missing_id_count), scanned_count < array.size() ? "YES" : "NO");
  log_ha_registry_memory("before entity cache comparison");
  load_ha_entities_if_needed();
  ESP_LOGI(TAG, "HA entity cache loaded: previous_count=%u", static_cast<unsigned>(cached_ha_entities().count));
  const bool changed = !has_same_ha_entities(cached_ha_entities(), *entities);
  ESP_LOGI(TAG, "HA entity cache comparison complete: changed=%s", changed ? "YES" : "NO");
  ha_entities_changed_since_last_parse_flag = changed;
  if (changed) {
    const bool saved = save_ha_entities(*entities);
    ESP_LOGI(TAG, "HA entity persistence complete: attempted=YES result=%s", saved ? "OK" : "FAILED");
  } else {
    ESP_LOGI(TAG, "HA entity persistence skipped: cached list is unchanged");
  }
  ESP_LOGI(TAG, "stored %u HA switch/light/cover/automation/weather entity(s)",
           static_cast<unsigned>(entities->count));
  // for (size_t i = 0; i < entities.count; i++) {
  //   ESP_LOGI(TAG, "HA entity: type=%s area=%s friendly_name=%s entity_id=%s",
  //            ha_entity_type_from_id(entities.entities[i].entity_id), entities.entities[i].area_name,
  //            entities.entities[i].friendly_name, entities.entities[i].entity_id);
  // }
  if (!changed) {
    ESP_LOGI(TAG, "HA entity list is unchanged");
  }
  log_ha_registry_memory("after entity processing");
  return true;
}

inline const char *get_ha_entity_display_name(const StoredHaEntity &entity) {
  return entity.friendly_name[0] != '\0' ? entity.friendly_name : entity.entity_id;
}

inline std::string build_ha_entity_config_option(const StoredHaEntity &entity) {
  std::string option;
  if (entity.area_name[0] != '\0') {
    option += entity.area_name;
    option += " / ";
  }
  if (entity.friendly_name[0] != '\0') {
    option += entity.friendly_name;
    option += " / ";
  }
  option += entity.entity_id;
  return option;
}

inline bool compare_ha_entity_for_select(const StoredHaEntity &a, const StoredHaEntity &b) {
  int area_name_cmp = strcmp(a.area_name, b.area_name);
  if (area_name_cmp != 0) {
    return area_name_cmp < 0;
  }
  int entity_name_cmp = strcmp(a.friendly_name, b.friendly_name);
  if (entity_name_cmp != 0) {
    return entity_name_cmp < 0;
  }
  return strcmp(a.entity_id, b.entity_id) < 0;
}

inline size_t build_filtered_config_options(HaEntityOptionArray &options, bool (*predicate)(const char *),
                                            bool include_local_gpio_switch_presets = false) {
  load_ha_entities_if_needed();
  options[0] = "none";
  size_t option_count = 1;
  if (include_local_gpio_switch_presets) {
    for (const char *preset_option : LOCAL_HMI_GPIO_SWITCH_PRESET_OPTIONS) {
      options[option_count++] = preset_option;
    }
  }
  std::array<uint16_t, MAX_HA_ENTITY_COUNT> sorted_indexes{};
  size_t sorted_count = 0;
  const StoredHaEntities &entities = cached_ha_entities();
  for (size_t i = 0; i < entities.count; i++) {
    if (predicate(entities.entities[i].entity_id)) {
      if (sorted_count >= MAX_HA_ENTITY_COUNT) {
        ESP_LOGW(TAG, "HA entity select option limit reached: %u", static_cast<unsigned>(sorted_count));
        break;
      }
      sorted_indexes[sorted_count++] = static_cast<uint16_t>(i);
    }
  }

  std::sort(sorted_indexes.begin(), sorted_indexes.begin() + sorted_count,
            [&](uint16_t a, uint16_t b) {
              return compare_ha_entity_for_select(entities.entities[a], entities.entities[b]);
            });

  for (size_t i = 0; i < sorted_count; i++) {
    options[option_count++] = build_ha_entity_config_option(entities.entities[sorted_indexes[i]]);
  }
  return option_count;
}

inline size_t build_device1_config_options(HaEntityOptionArray &options) {
  return build_filtered_config_options(options, is_supported_normal_device_entity_id, true);
}

inline size_t build_standby_weather_config_options(HaEntityOptionArray &options) {
  return build_filtered_config_options(options, is_supported_standby_weather_entity_id);
}

inline bool select_options_match(const esphome::FixedVector<const char *> &current_options,
                                 const HaEntityOptionArray &new_options, size_t count) {
  if (current_options.size() != count) {
    return false;
  }
  for (size_t i = 0; i < count; i++) {
    if (current_options[i] == nullptr || strcmp(current_options[i], new_options[i].c_str()) != 0) {
      return false;
    }
  }
  return true;
}

inline bool apply_device_config_options(size_t slot, esphome::select::Select *my_select) {
  if (my_select == nullptr) {
    ESP_LOGW(TAG, "%s_config select is null, skip option refresh", device_slot_label(slot).c_str());
    return false;
  }

  // ESP_LOGI(TAG, "HA entity select refresh begin: slot=%u label=%s",
  //          static_cast<unsigned>(slot + 1), device_slot_label(slot).c_str());
  // log_ha_registry_memory("before device select options");
  auto &staged_options = staged_device_config_option_strings(slot);
  auto &stable_options = device_config_option_strings(slot);
  const size_t option_count = build_device1_config_options(staged_options);
  const auto &current_options = my_select->traits.get_options();
  // ESP_LOGI(TAG, "HA entity select options built: slot=%u current=%u new=%u",
  //          static_cast<unsigned>(slot + 1), static_cast<unsigned>(current_options.size()),
  //          static_cast<unsigned>(option_count));
  if (select_options_match(current_options, staged_options, option_count)) {
    // ESP_LOGI(TAG, "HA entity select refresh skipped: slot=%u options unchanged",
    //          static_cast<unsigned>(slot + 1));
    return false;
  }

  std::string selected_option;
  const auto active_index = my_select->active_index();
  if (active_index.has_value() && active_index.value() < current_options.size() &&
      current_options[active_index.value()] != nullptr) {
    selected_option = current_options[active_index.value()];
  }
  if (selected_option.empty() || selected_option == "none") {
    load_device_config_if_needed(slot);
    for (size_t i = 0; i < option_count; i++) {
      if (extract_entity_id_from_device1_option(staged_options[i]) == cached_device_entity_id(slot)) {
        selected_option = staged_options[i];
        break;
      }
    }
  }

  for (size_t i = 0; i < option_count; i++) {
    stable_options[i] = staged_options[i];
  }

  esphome::FixedVector<const char *> new_options;
  new_options.init(option_count);
  for (size_t i = 0; i < option_count; i++) {
    new_options.push_back(stable_options[i].c_str());
  }
  my_select->traits.set_options(new_options);
  const auto selected_index = my_select->index_of(selected_option);
  my_select->publish_state(selected_index.has_value() ? selected_index.value() : static_cast<size_t>(0));
  ESP_LOGI(TAG, "updated %s_config with %zu option(s)", device_slot_label(slot).c_str(), option_count);
  log_ha_registry_memory("after device select options");
  return true;
}

inline bool apply_device1_config_options(esphome::select::Select *my_select) {
  return apply_device_config_options(0, my_select);
}

inline bool apply_device2_config_options(esphome::select::Select *my_select) {
  return apply_device_config_options(1, my_select);
}

inline bool apply_standby_weather_config_options(esphome::select::Select *my_select) {
  if (my_select == nullptr) {
    ESP_LOGW(TAG, "standby_weather_config select is null, skip option refresh");
    return false;
  }

  ESP_LOGI(TAG, "HA standby weather select options build begin");
  log_ha_registry_memory("before weather select options");
  auto &staged_options = staged_standby_weather_config_options();
  auto &stable_options = standby_weather_config_options();
  const size_t option_count = build_standby_weather_config_options(staged_options);
  const auto &current_options = my_select->traits.get_options();
  ESP_LOGI(TAG, "HA standby weather select options built: current=%u new=%u",
           static_cast<unsigned>(current_options.size()), static_cast<unsigned>(option_count));
  if (select_options_match(current_options, staged_options, option_count)) {
    ESP_LOGI(TAG, "standby_weather_config options are already up to date");
    return false;
  }

  std::string selected_option;
  const auto active_index = my_select->active_index();
  if (active_index.has_value() && active_index.value() < current_options.size() &&
      current_options[active_index.value()] != nullptr) {
    selected_option = current_options[active_index.value()];
  }
  if (selected_option.empty() || selected_option == "none") {
    load_standby_weather_config_if_needed();
    for (size_t i = 0; i < option_count; i++) {
      if (extract_entity_id_from_device1_option(staged_options[i]) == cached_standby_weather_entity_id) {
        selected_option = staged_options[i];
        break;
      }
    }
  }

  for (size_t i = 0; i < option_count; i++) {
    stable_options[i] = staged_options[i];
  }

  esphome::FixedVector<const char *> new_options;
  new_options.init(option_count);
  for (size_t i = 0; i < option_count; i++) {
    new_options.push_back(stable_options[i].c_str());
  }
  my_select->traits.set_options(new_options);
  const auto selected_index = my_select->index_of(selected_option);
  my_select->publish_state(selected_index.has_value() ? selected_index.value() : static_cast<size_t>(0));
  ESP_LOGI(TAG, "updated standby_weather_config with %zu option(s)", option_count);
  log_ha_registry_memory("after weather select options");
  return true;
}

inline bool update_device1_config_from_option(const std::string &option) {
  return save_device_config_entity_id(0, extract_entity_id_from_device1_option(option));
}

inline bool update_device2_config_from_option(const std::string &option) {
  return save_device_config_entity_id(1, extract_entity_id_from_device1_option(option));
}

inline bool update_device3_config_from_option(const std::string &option) {
  return save_device_config_entity_id(2, extract_entity_id_from_device1_option(option));
}

inline bool update_device4_config_from_option(const std::string &option) {
  return save_device_config_entity_id(3, extract_entity_id_from_device1_option(option));
}

inline bool update_device5_config_from_option(const std::string &option) {
  return save_device_config_entity_id(4, extract_entity_id_from_device1_option(option));
}

inline bool update_device6_config_from_option(const std::string &option) {
  return save_device_config_entity_id(5, extract_entity_id_from_device1_option(option));
}

inline bool update_device7_config_from_option(const std::string &option) {
  return save_device_config_entity_id(6, extract_entity_id_from_device1_option(option));
}

inline bool update_device8_config_from_option(const std::string &option) {
  return save_device_config_entity_id(7, extract_entity_id_from_device1_option(option));
}

inline bool update_device9_config_from_option(const std::string &option) {
  return save_device_config_entity_id(8, extract_entity_id_from_device1_option(option));
}

inline bool update_device10_config_from_option(const std::string &option) {
  return save_device_config_entity_id(9, extract_entity_id_from_device1_option(option));
}

inline bool update_standby_weather_config_from_option(const std::string &option) {
  return save_standby_weather_config_entity_id(extract_entity_id_from_device1_option(option));
}

inline bool save_device_config_from_select(size_t slot, esphome::select::Select *my_select) {
  if (!is_configurable_device_slot(slot) || my_select == nullptr) {
    return false;
  }
  const std::string entity_id = extract_entity_id_from_device1_option(my_select->state);
  load_device_config_if_needed(slot);
  const std::string previous_entity_id = cached_device_entity_id(slot);
  const bool changed = previous_entity_id != entity_id;
  const bool saved = save_device_config_entity_id(slot, entity_id);
  if (saved) {
    ESP_LOGI(TAG, "%s config committed: previous=%s current=%s changed=%s",
             device_slot_label(slot).c_str(),
             previous_entity_id.empty() ? "<empty>" : previous_entity_id.c_str(),
             entity_id.empty() ? "<empty>" : entity_id.c_str(),
             changed ? "YES" : "NO");
  }
  return saved && changed;
}

inline bool save_entity_config_from_selects(esphome::select::Select *device1_select,
                                            esphome::select::Select *device2_select) {
  const bool device1_changed = save_device_config_from_select(0, device1_select);
  const bool device2_changed = save_device_config_from_select(1, device2_select);
  return device1_changed || device2_changed;
}

inline bool save_entity_config_from_selects(esphome::select::Select *device1_select,
                                            esphome::select::Select *device2_select,
                                            esphome::select::Select *device3_select,
                                            esphome::select::Select *device4_select,
                                            esphome::select::Select *device5_select) {
  const bool device1_changed = save_device_config_from_select(0, device1_select);
  const bool device2_changed = save_device_config_from_select(1, device2_select);
  const bool device3_changed = save_device_config_from_select(2, device3_select);
  const bool device4_changed = save_device_config_from_select(3, device4_select);
  const bool device5_changed = save_device_config_from_select(4, device5_select);
  return device1_changed || device2_changed || device3_changed || device4_changed || device5_changed;
}

inline bool save_entity_config_from_selects(esphome::select::Select *device1_select,
                                            esphome::select::Select *device2_select,
                                            esphome::select::Select *device3_select,
                                            esphome::select::Select *device4_select,
                                            esphome::select::Select *device5_select,
                                            esphome::select::Select *device6_select,
                                            esphome::select::Select *device7_select,
                                            esphome::select::Select *device8_select,
                                            esphome::select::Select *device9_select,
                                            esphome::select::Select *device10_select) {
  const bool device1_changed = save_device_config_from_select(0, device1_select);
  const bool device2_changed = save_device_config_from_select(1, device2_select);
  const bool device3_changed = save_device_config_from_select(2, device3_select);
  const bool device4_changed = save_device_config_from_select(3, device4_select);
  const bool device5_changed = save_device_config_from_select(4, device5_select);
  const bool device6_changed = save_device_config_from_select(5, device6_select);
  const bool device7_changed = save_device_config_from_select(6, device7_select);
  const bool device8_changed = save_device_config_from_select(7, device8_select);
  const bool device9_changed = save_device_config_from_select(8, device9_select);
  const bool device10_changed = save_device_config_from_select(9, device10_select);
  return device1_changed || device2_changed || device3_changed || device4_changed || device5_changed ||
         device6_changed || device7_changed || device8_changed || device9_changed || device10_changed;
}

inline bool save_standby_weather_config_from_select(esphome::select::Select *my_select) {
  if (my_select == nullptr) {
    return false;
  }
  const std::string entity_id = extract_entity_id_from_device1_option(my_select->state);
  load_standby_weather_config_if_needed();
  const std::string previous_entity_id = cached_standby_weather_entity_id;
  const bool changed = previous_entity_id != entity_id;
  const bool saved = save_standby_weather_config_entity_id(entity_id);
  if (saved) {
    ESP_LOGI(TAG, "standby weather config committed: previous=%s current=%s changed=%s",
             previous_entity_id.empty() ? "<empty>" : previous_entity_id.c_str(),
             entity_id.empty() ? "<empty>" : entity_id.c_str(),
             changed ? "YES" : "NO");
  }
  return saved && changed;
}

inline bool save_entity_config_from_selects(esphome::select::Select *device1_select,
                                            esphome::select::Select *device2_select,
                                            esphome::select::Select *standby_weather_select) {
  const bool device_changed = save_entity_config_from_selects(device1_select, device2_select);
  const bool weather_changed = save_standby_weather_config_from_select(standby_weather_select);
  return device_changed || weather_changed;
}

inline bool save_entity_config_from_selects(esphome::select::Select *device1_select,
                                            esphome::select::Select *device2_select,
                                            esphome::select::Select *device3_select,
                                            esphome::select::Select *device4_select,
                                            esphome::select::Select *device5_select,
                                            esphome::select::Select *device6_select,
                                            esphome::select::Select *device7_select,
                                            esphome::select::Select *device8_select,
                                            esphome::select::Select *device9_select,
                                            esphome::select::Select *device10_select,
                                            esphome::select::Select *standby_weather_select) {
  const bool device_changed =
      save_entity_config_from_selects(device1_select, device2_select, device3_select, device4_select, device5_select,
                                      device6_select, device7_select, device8_select, device9_select, device10_select);
  const bool weather_changed = save_standby_weather_config_from_select(standby_weather_select);
  return device_changed || weather_changed;
}

inline bool restore_standby_weather_config_for_select(esphome::select::Select *my_select) {
  load_standby_weather_config_if_needed();
  if (my_select == nullptr) {
    return false;
  }
  apply_standby_weather_config_options(my_select);
  if (cached_standby_weather_entity_id.empty()) {
    my_select->publish_state(static_cast<size_t>(0));
    return true;
  }
  const auto &options = my_select->traits.get_options();
  for (size_t i = 0; i < options.size(); i++) {
    if (options[i] != nullptr && extract_entity_id_from_device1_option(options[i]) == cached_standby_weather_entity_id) {
      my_select->publish_state(i);
      return true;
    }
  }
  auto &weather_options = standby_weather_config_options();
  weather_options[0] = cached_standby_weather_entity_id;
  esphome::FixedVector<const char *> fallback_options;
  fallback_options.init(1);
  fallback_options.push_back(weather_options[0].c_str());
  my_select->traits.set_options(fallback_options);
  my_select->publish_state(static_cast<size_t>(0));
  ESP_LOGI(TAG, "restored standby_weather_config fallback option: %s", cached_standby_weather_entity_id.c_str());
  return true;
}

inline bool parse_and_store_ha_entity_registry_entities(JsonArrayConst array) {
  ha_entities_changed_since_last_parse_flag = false;
  return store_ha_entities_from_entity_registry_array(array);
}

inline bool ha_entities_changed_since_last_parse() {
  return ha_entities_changed_since_last_parse_flag;
}

inline bool fixed_options_contains(const esphome::FixedVector<const char *> &options, const char *option) {
  for (auto opt : options) {
    if (strcmp(opt, option) == 0) {
      return true;
    }
  }
  return false;
}

}  // namespace custom_logic

using custom_logic::NORMAL_DEVICE_DETAIL_KIND_AUTOMATION;
using custom_logic::NORMAL_DEVICE_DETAIL_KIND_CURTAIN;
using custom_logic::NORMAL_DEVICE_DETAIL_KIND_DIMMING_LIGHT;
using custom_logic::NORMAL_DEVICE_DETAIL_KIND_LIGHT;
using custom_logic::NORMAL_DEVICE_DETAIL_KIND_NONE;
using custom_logic::NORMAL_DEVICE_DETAIL_KIND_PLUG;
using custom_logic::NORMAL_DEVICE_DETAIL_KIND_SETTINGS;
using custom_logic::NORMAL_DEVICE_DETAIL_KIND_SETTINGS_BRIGHTNESS;
using custom_logic::NORMAL_DEVICE_DETAIL_KIND_SETTINGS_KNOB;
using custom_logic::NORMAL_DEVICE_DETAIL_KIND_SETTINGS_USER_MANUAL;
using custom_logic::NORMAL_DEVICE_DETAIL_KIND_SETTINGS_USER_MANUAL_QR_CODE;
using custom_logic::NORMAL_DEVICE_DETAIL_KIND_SETTINGS_WIFI;
using custom_logic::NORMAL_DEVICE_DETAIL_KIND_SWITCH;

void restore_device1_config_for_select(esphome::select::Select *my_select) {
  custom_logic::apply_device1_config_options(my_select);
}

void restore_device1_config_for_select(esphome::select::Select *my_select, esphome::api::APIServer *api_server) {
  if (custom_logic::apply_device1_config_options(my_select)) {
    custom_logic::disconnect_all_clients_for_refresh(api_server);
  }
}

void restore_device2_config_for_select(esphome::select::Select *my_select) {
  custom_logic::apply_device2_config_options(my_select);
}

void restore_device2_config_for_select(esphome::select::Select *my_select, esphome::api::APIServer *api_server) {
  if (custom_logic::apply_device2_config_options(my_select)) {
    custom_logic::disconnect_all_clients_for_refresh(api_server);
  }
}

void restore_device3_config_for_select(esphome::select::Select *my_select) {
  custom_logic::apply_device_config_options(2, my_select);
}

void restore_device3_config_for_select(esphome::select::Select *my_select, esphome::api::APIServer *api_server) {
  if (custom_logic::apply_device_config_options(2, my_select)) {
    custom_logic::disconnect_all_clients_for_refresh(api_server);
  }
}

void restore_device4_config_for_select(esphome::select::Select *my_select) {
  custom_logic::apply_device_config_options(3, my_select);
}

void restore_device4_config_for_select(esphome::select::Select *my_select, esphome::api::APIServer *api_server) {
  if (custom_logic::apply_device_config_options(3, my_select)) {
    custom_logic::disconnect_all_clients_for_refresh(api_server);
  }
}

void restore_device5_config_for_select(esphome::select::Select *my_select) {
  custom_logic::apply_device_config_options(4, my_select);
}

void restore_device5_config_for_select(esphome::select::Select *my_select, esphome::api::APIServer *api_server) {
  if (custom_logic::apply_device_config_options(4, my_select)) {
    custom_logic::disconnect_all_clients_for_refresh(api_server);
  }
}

void restore_device6_config_for_select(esphome::select::Select *my_select) {
  custom_logic::apply_device_config_options(5, my_select);
}

void restore_device6_config_for_select(esphome::select::Select *my_select, esphome::api::APIServer *api_server) {
  if (custom_logic::apply_device_config_options(5, my_select)) {
    custom_logic::disconnect_all_clients_for_refresh(api_server);
  }
}

void restore_device7_config_for_select(esphome::select::Select *my_select) {
  custom_logic::apply_device_config_options(6, my_select);
}

void restore_device7_config_for_select(esphome::select::Select *my_select, esphome::api::APIServer *api_server) {
  if (custom_logic::apply_device_config_options(6, my_select)) {
    custom_logic::disconnect_all_clients_for_refresh(api_server);
  }
}

void restore_device8_config_for_select(esphome::select::Select *my_select) {
  custom_logic::apply_device_config_options(7, my_select);
}

void restore_device8_config_for_select(esphome::select::Select *my_select, esphome::api::APIServer *api_server) {
  if (custom_logic::apply_device_config_options(7, my_select)) {
    custom_logic::disconnect_all_clients_for_refresh(api_server);
  }
}

void restore_device9_config_for_select(esphome::select::Select *my_select) {
  custom_logic::apply_device_config_options(8, my_select);
}

void restore_device9_config_for_select(esphome::select::Select *my_select, esphome::api::APIServer *api_server) {
  if (custom_logic::apply_device_config_options(8, my_select)) {
    custom_logic::disconnect_all_clients_for_refresh(api_server);
  }
}

void restore_device10_config_for_select(esphome::select::Select *my_select) {
  custom_logic::apply_device_config_options(9, my_select);
}

void restore_device10_config_for_select(esphome::select::Select *my_select, esphome::api::APIServer *api_server) {
  if (custom_logic::apply_device_config_options(9, my_select)) {
    custom_logic::disconnect_all_clients_for_refresh(api_server);
  }
}

void restore_standby_weather_config_for_select(esphome::select::Select *my_select) {
  custom_logic::restore_standby_weather_config_for_select(my_select);
}

void restore_standby_weather_config_for_select(esphome::select::Select *my_select,
                                               esphome::api::APIServer *api_server) {
  custom_logic::restore_standby_weather_config_for_select(my_select);
}

void set_normal_ui_storage_for_custom_logic(esphome::onx_storage::OnxStorage *storage) {
  custom_logic::set_normal_ui_storage(storage);
}

void save_ha_token_value(const std::string &token) {
  custom_logic::save_ha_token(token);
}

bool has_saved_ha_token() {
  return custom_logic::has_saved_ha_token();
}

std::string extract_entity_id_from_device1_option(const std::string &option) {
  return custom_logic::extract_entity_id_from_device1_option(option);
}

bool is_supported_normal_device_entity_id(const char *entity_id) {
  return custom_logic::is_supported_normal_device_entity_id(entity_id);
}

uint32_t get_device1_inferred_light_capability_mask() {
  return custom_logic::get_device_inferred_light_capability_mask(0);
}

uint32_t get_device2_inferred_light_capability_mask() {
  return custom_logic::get_device_inferred_light_capability_mask(1);
}

std::string build_ha_http_state_url(const std::string &raw_host, const std::string &entity_id) {
  return custom_logic::build_ha_http_state_url(raw_host, entity_id);
}

const char *get_ha_token_status_text() {
  return custom_logic::get_ha_token_status_text();
}

const char *get_ha_authorization_header() {
  return custom_logic::get_ha_authorization_header();
}

bool maintain_ha_websocket(const std::string &host, bool wifi_connected, bool *connected_out) {
  return custom_logic::maintain_ha_websocket(host, wifi_connected, connected_out);
}

bool request_ha_entity_list_over_websocket(const std::string &host,
                                           esphome::select::Select *device1_select,
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
                                           esphome::api::APIServer *api_server) {
  return custom_logic::request_ha_entity_list_over_websocket(host, device1_select, device2_select, device3_select,
                                                             device4_select, device5_select, device6_select,
                                                             device7_select, device8_select, device9_select,
                                                             device10_select, standby_weather_select, api_server);
}

bool request_ha_entity_list_over_websocket(const std::string &host,
                                           esphome::select::Select *device1_select,
                                           esphome::select::Select *device2_select,
                                           esphome::select::Select *standby_weather_select,
                                           esphome::api::APIServer *api_server) {
  return custom_logic::request_ha_entity_list_over_websocket(host, device1_select, device2_select, standby_weather_select,
                                                             api_server);
}

bool request_ha_entity_list_over_websocket(const std::string &host,
                                           esphome::select::Select *device1_select,
                                           esphome::select::Select *device2_select,
                                           esphome::api::APIServer *api_server) {
  return custom_logic::request_ha_entity_list_over_websocket(host, device1_select, device2_select, api_server);
}

bool update_device1_config_from_option(const std::string &option) {
  return custom_logic::update_device1_config_from_option(option);
}

bool update_device2_config_from_option(const std::string &option) {
  return custom_logic::update_device2_config_from_option(option);
}

bool update_device3_config_from_option(const std::string &option) {
  return custom_logic::update_device3_config_from_option(option);
}

bool update_device4_config_from_option(const std::string &option) {
  return custom_logic::update_device4_config_from_option(option);
}

bool update_device5_config_from_option(const std::string &option) {
  return custom_logic::update_device5_config_from_option(option);
}

bool update_device6_config_from_option(const std::string &option) {
  return custom_logic::update_device6_config_from_option(option);
}

bool update_device7_config_from_option(const std::string &option) {
  return custom_logic::update_device7_config_from_option(option);
}

bool update_device8_config_from_option(const std::string &option) {
  return custom_logic::update_device8_config_from_option(option);
}

bool update_device9_config_from_option(const std::string &option) {
  return custom_logic::update_device9_config_from_option(option);
}

bool update_device10_config_from_option(const std::string &option) {
  return custom_logic::update_device10_config_from_option(option);
}

bool update_standby_weather_config_from_option(const std::string &option) {
  return custom_logic::update_standby_weather_config_from_option(option);
}

bool save_entity_config_from_selects(esphome::select::Select *device1_select,
                                     esphome::select::Select *device2_select) {
  return custom_logic::save_entity_config_from_selects(device1_select, device2_select);
}

bool save_entity_config_from_selects(esphome::select::Select *device1_select,
                                     esphome::select::Select *device2_select,
                                     esphome::select::Select *standby_weather_select) {
  return custom_logic::save_entity_config_from_selects(device1_select, device2_select, standby_weather_select);
}

bool save_entity_config_from_selects(esphome::select::Select *device1_select,
                                     esphome::select::Select *device2_select,
                                     esphome::select::Select *device3_select,
                                     esphome::select::Select *device4_select,
                                     esphome::select::Select *device5_select,
                                     esphome::select::Select *device6_select,
                                     esphome::select::Select *device7_select,
                                     esphome::select::Select *device8_select,
                                     esphome::select::Select *device9_select,
                                     esphome::select::Select *device10_select,
                                     esphome::select::Select *standby_weather_select) {
  return custom_logic::save_entity_config_from_selects(device1_select, device2_select, device3_select, device4_select,
                                                       device5_select, device6_select, device7_select, device8_select,
                                                       device9_select, device10_select, standby_weather_select);
}

bool apply_device1_type_options(esphome::select::Select *my_select) {
  return custom_logic::apply_device_type_options(0, my_select);
}

bool apply_device2_type_options(esphome::select::Select *my_select) {
  return custom_logic::apply_device_type_options(1, my_select);
}

bool apply_device3_type_options(esphome::select::Select *my_select) {
  return custom_logic::apply_device_type_options(2, my_select);
}

bool apply_device4_type_options(esphome::select::Select *my_select) {
  return custom_logic::apply_device_type_options(3, my_select);
}

bool apply_device5_type_options(esphome::select::Select *my_select) {
  return custom_logic::apply_device_type_options(4, my_select);
}

bool apply_device6_type_options(esphome::select::Select *my_select) {
  return custom_logic::apply_device_type_options(5, my_select);
}

bool apply_device7_type_options(esphome::select::Select *my_select) {
  return custom_logic::apply_device_type_options(6, my_select);
}

bool apply_device8_type_options(esphome::select::Select *my_select) {
  return custom_logic::apply_device_type_options(7, my_select);
}

bool apply_device9_type_options(esphome::select::Select *my_select) {
  return custom_logic::apply_device_type_options(8, my_select);
}

bool apply_device10_type_options(esphome::select::Select *my_select) {
  return custom_logic::apply_device_type_options(9, my_select);
}

bool update_device1_light_config_from_http_response(const std::string &entity_id, const std::string &body) {
  return custom_logic::update_device_light_config_from_http_response(0, entity_id, body);
}

bool update_device2_light_config_from_http_response(const std::string &entity_id, const std::string &body) {
  return custom_logic::update_device_light_config_from_http_response(1, entity_id, body);
}

bool update_device3_light_config_from_http_response(const std::string &entity_id, const std::string &body) {
  return custom_logic::update_device_light_config_from_http_response(2, entity_id, body);
}

bool update_device4_light_config_from_http_response(const std::string &entity_id, const std::string &body) {
  return custom_logic::update_device_light_config_from_http_response(3, entity_id, body);
}

bool update_device5_light_config_from_http_response(const std::string &entity_id, const std::string &body) {
  return custom_logic::update_device_light_config_from_http_response(4, entity_id, body);
}

bool update_device6_light_config_from_http_response(const std::string &entity_id, const std::string &body) {
  return custom_logic::update_device_light_config_from_http_response(5, entity_id, body);
}

bool update_device7_light_config_from_http_response(const std::string &entity_id, const std::string &body) {
  return custom_logic::update_device_light_config_from_http_response(6, entity_id, body);
}

bool update_device8_light_config_from_http_response(const std::string &entity_id, const std::string &body) {
  return custom_logic::update_device_light_config_from_http_response(7, entity_id, body);
}

bool update_device9_light_config_from_http_response(const std::string &entity_id, const std::string &body) {
  return custom_logic::update_device_light_config_from_http_response(8, entity_id, body);
}

bool update_device10_light_config_from_http_response(const std::string &entity_id, const std::string &body) {
  return custom_logic::update_device_light_config_from_http_response(9, entity_id, body);
}

bool update_device1_custom_type_from_option(const std::string &option) {
  return custom_logic::save_device1_custom_config(
      custom_logic::custom_type_from_device1_type_option(option),
      custom_logic::custom_light_capability_mask_from_device1_type_option(option));
}

bool update_device2_custom_type_from_option(const std::string &option) {
  return custom_logic::save_device2_custom_config(
      custom_logic::custom_type_from_device1_type_option(option),
      custom_logic::custom_light_capability_mask_from_device1_type_option(option));
}

bool update_device3_custom_type_from_option(const std::string &option) {
  return custom_logic::save_device3_custom_config(
      custom_logic::custom_type_from_device1_type_option(option),
      custom_logic::custom_light_capability_mask_from_device1_type_option(option));
}

bool update_device4_custom_type_from_option(const std::string &option) {
  return custom_logic::save_device4_custom_config(
      custom_logic::custom_type_from_device1_type_option(option),
      custom_logic::custom_light_capability_mask_from_device1_type_option(option));
}

bool update_device5_custom_type_from_option(const std::string &option) {
  return custom_logic::save_device5_custom_config(
      custom_logic::custom_type_from_device1_type_option(option),
      custom_logic::custom_light_capability_mask_from_device1_type_option(option));
}

bool update_device6_custom_type_from_option(const std::string &option) {
  return custom_logic::save_device6_custom_config(
      custom_logic::custom_type_from_device1_type_option(option),
      custom_logic::custom_light_capability_mask_from_device1_type_option(option));
}

bool update_device7_custom_type_from_option(const std::string &option) {
  return custom_logic::save_device7_custom_config(
      custom_logic::custom_type_from_device1_type_option(option),
      custom_logic::custom_light_capability_mask_from_device1_type_option(option));
}

bool update_device8_custom_type_from_option(const std::string &option) {
  return custom_logic::save_device8_custom_config(
      custom_logic::custom_type_from_device1_type_option(option),
      custom_logic::custom_light_capability_mask_from_device1_type_option(option));
}

bool update_device9_custom_type_from_option(const std::string &option) {
  return custom_logic::save_device9_custom_config(
      custom_logic::custom_type_from_device1_type_option(option),
      custom_logic::custom_light_capability_mask_from_device1_type_option(option));
}

bool update_device10_custom_type_from_option(const std::string &option) {
  return custom_logic::save_device10_custom_config(
      custom_logic::custom_type_from_device1_type_option(option),
      custom_logic::custom_light_capability_mask_from_device1_type_option(option));
}

bool apply_device1_config_to_normal_ui_storage() {
  return custom_logic::apply_device1_config_to_normal_ui_storage();
}

bool apply_device2_config_to_normal_ui_storage() {
  return custom_logic::apply_device2_config_to_normal_ui_storage();
}

bool apply_device3_config_to_normal_ui_storage() {
  return custom_logic::apply_device3_config_to_normal_ui_storage();
}

bool apply_device4_config_to_normal_ui_storage() {
  return custom_logic::apply_device4_config_to_normal_ui_storage();
}

bool apply_device5_config_to_normal_ui_storage() {
  return custom_logic::apply_device5_config_to_normal_ui_storage();
}

bool apply_device6_config_to_normal_ui_storage() {
  return custom_logic::apply_device6_config_to_normal_ui_storage();
}

bool apply_device7_config_to_normal_ui_storage() {
  return custom_logic::apply_device7_config_to_normal_ui_storage();
}

bool apply_device8_config_to_normal_ui_storage() {
  return custom_logic::apply_device8_config_to_normal_ui_storage();
}

bool apply_device9_config_to_normal_ui_storage() {
  return custom_logic::apply_device9_config_to_normal_ui_storage();
}

bool apply_device10_config_to_normal_ui_storage() {
  return custom_logic::apply_device10_config_to_normal_ui_storage();
}

std::string get_provisioning_ap_ssid() {
  return custom_logic::get_provisioning_ap_ssid_string();
}

bool ha_entities_changed_since_last_parse() {
  return custom_logic::ha_entities_changed_since_last_parse();
}

std::string ha_entity_title_from_registry_cache(const std::string &entity_id) {
  return custom_logic::ha_entity_title_from_registry_cache(entity_id);
}

size_t configured_normal_device_count() {
  return custom_logic::configured_normal_device_count();
}

bool is_local_hmi_gpio_switch_entity_id(const std::string &entity_id) {
  return custom_logic::is_local_hmi_gpio_switch_entity_id(entity_id);
}

int local_hmi_gpio_switch_io_from_entity_id(const std::string &entity_id) {
  return custom_logic::local_hmi_gpio_switch_io_from_entity_id(entity_id);
}

bool reset_normal_mode_preferences() {
  return custom_logic::reset_normal_mode_preferences();
}
