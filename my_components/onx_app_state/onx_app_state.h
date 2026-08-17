#pragma once

#include <string>

#include "esphome/core/component.h"

namespace esphome::onx_app_state {

enum class OnxRuntimeMode : uint8_t {
  MODE_UNKNOWN = 0,
  MODE_NORMAL = 2,
};

enum class OnxLinkState : uint8_t {
  LINK_IDLE = 0,
  LINK_PROBING = 1,
  LINK_MATCHED = 2,
  LINK_TIMEOUT = 3,
};

// Central runtime state shared by future mode controllers, protocol handlers and UI.
// The goal is to keep cross-cutting state out of YAML lambdas so mode logic stays testable.
class OnxAppState : public Component {
 public:
  void setup() override;
  void dump_config() override;

  void set_current_mode(OnxRuntimeMode mode);
  OnxRuntimeMode get_current_mode() const;
  const char *get_current_mode_name() const;

  void set_link_state(OnxLinkState state);
  OnxLinkState get_link_state() const;
  const char *get_link_state_name() const;

  void set_mode_reason(const std::string &reason);
  const std::string &get_mode_reason() const;

  void set_last_rx_line(const std::string &line);
  const std::string &get_last_rx_line() const;

  void set_last_tx_line(const std::string &line);
  const std::string &get_last_tx_line() const;

  void mark_ui_dirty();
  void clear_ui_dirty();
  bool is_ui_dirty() const;

 protected:
  OnxRuntimeMode current_mode_{OnxRuntimeMode::MODE_UNKNOWN};
  OnxLinkState link_state_{OnxLinkState::LINK_IDLE};
  std::string mode_reason_;
  std::string last_rx_line_;
  std::string last_tx_line_;
  bool ui_dirty_{true};
};

}  // namespace esphome::onx_app_state
