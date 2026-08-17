#include "onx_app_state.h"

#include "esphome/core/log.h"

namespace esphome::onx_app_state {

static const char *const TAG = "onx_app_state";

void OnxAppState::setup() {
  // Start with an undecided mode and an idle link state until boot arbitration runs.
  this->current_mode_ = OnxRuntimeMode::MODE_UNKNOWN;
  this->link_state_ = OnxLinkState::LINK_IDLE;
  this->mode_reason_.clear();
  this->last_rx_line_.clear();
  this->last_tx_line_.clear();
  this->ui_dirty_ = true;
}

void OnxAppState::dump_config() {
  ESP_LOGCONFIG(TAG, "ONX App State:");
  ESP_LOGCONFIG(TAG, "  Current mode: %s", this->get_current_mode_name());
  ESP_LOGCONFIG(TAG, "  Link state: %s", this->get_link_state_name());
  ESP_LOGCONFIG(TAG, "  Mode reason: %s", this->mode_reason_.empty() ? "<empty>" : this->mode_reason_.c_str());
  ESP_LOGCONFIG(TAG, "  Last RX: %s", this->last_rx_line_.empty() ? "<empty>" : this->last_rx_line_.c_str());
  ESP_LOGCONFIG(TAG, "  Last TX: %s", this->last_tx_line_.empty() ? "<empty>" : this->last_tx_line_.c_str());
  ESP_LOGCONFIG(TAG, "  UI dirty: %s", this->ui_dirty_ ? "YES" : "NO");
}

void OnxAppState::set_current_mode(OnxRuntimeMode mode) {
  if (this->current_mode_ == mode) {
    return;
  }
  this->current_mode_ = mode;
  this->ui_dirty_ = true;
  ESP_LOGI(TAG, "Runtime mode updated to %s", this->get_current_mode_name());
}

OnxRuntimeMode OnxAppState::get_current_mode() const { return this->current_mode_; }

const char *OnxAppState::get_current_mode_name() const {
  switch (this->current_mode_) {
    case OnxRuntimeMode::MODE_NORMAL:
      return "normal";
    case OnxRuntimeMode::MODE_UNKNOWN:
    default:
      return "unknown";
  }
}

void OnxAppState::set_link_state(OnxLinkState state) {
  if (this->link_state_ == state) {
    return;
  }
  this->link_state_ = state;
  this->ui_dirty_ = true;
  ESP_LOGI(TAG, "Link state updated to %s", this->get_link_state_name());
}

OnxLinkState OnxAppState::get_link_state() const { return this->link_state_; }

const char *OnxAppState::get_link_state_name() const {
  switch (this->link_state_) {
    case OnxLinkState::LINK_PROBING:
      return "probing";
    case OnxLinkState::LINK_MATCHED:
      return "matched";
    case OnxLinkState::LINK_TIMEOUT:
      return "timeout";
    case OnxLinkState::LINK_IDLE:
    default:
      return "idle";
  }
}

void OnxAppState::set_mode_reason(const std::string &reason) {
  if (this->mode_reason_ == reason) {
    return;
  }
  this->mode_reason_ = reason;
  this->ui_dirty_ = true;
}

const std::string &OnxAppState::get_mode_reason() const { return this->mode_reason_; }

void OnxAppState::set_last_rx_line(const std::string &line) {
  if (this->last_rx_line_ == line) {
    return;
  }
  this->last_rx_line_ = line;
  this->ui_dirty_ = true;
}

const std::string &OnxAppState::get_last_rx_line() const { return this->last_rx_line_; }

void OnxAppState::set_last_tx_line(const std::string &line) {
  if (this->last_tx_line_ == line) {
    return;
  }
  this->last_tx_line_ = line;
  this->ui_dirty_ = true;
}

const std::string &OnxAppState::get_last_tx_line() const { return this->last_tx_line_; }

void OnxAppState::mark_ui_dirty() { this->ui_dirty_ = true; }
void OnxAppState::clear_ui_dirty() { this->ui_dirty_ = false; }
bool OnxAppState::is_ui_dirty() const { return this->ui_dirty_; }

}  // namespace esphome::onx_app_state
