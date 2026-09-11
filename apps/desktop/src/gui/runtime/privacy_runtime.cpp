/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-11
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#include <shared_mutex>

#include "rd_log.h"
#include "runtime/gui_runtime.h"

namespace crossdesk {

bool GuiRuntime::IsAuthorizedController(const std::string& remote_id) {
  // Connections appear here only after the existing password/signalling/RTC
  // admission path. Outgoing/viewer sessions live in remote_sessions_ instead.
  std::shared_lock lock(connection_status_mutex_);
  const auto found = connection_status_.find(remote_id);
  return found != connection_status_.end() &&
         found->second == ConnectionStatus::Connected;
}

void GuiRuntime::QueuePrivacyCommand(const std::string& remote_id,
                                     const PrivacyCommand& command) {
  if (!IsAuthorizedController(remote_id)) return;
  std::lock_guard lock(privacy_commands_mutex_);
  if (privacy_commands_.size() < 32)
    privacy_commands_.emplace_back(remote_id, command);
}

void GuiRuntime::HandlePrivacy() {
  std::deque<std::pair<std::string, PrivacyCommand>> commands;
  {
    std::lock_guard lock(privacy_commands_mutex_);
    commands.swap(privacy_commands_);
  }
  for (const auto& [remote_id, command] : commands) {
    if (!IsAuthorizedController(remote_id)) continue;
    if (command.flag == PrivacyCommandFlag::enable) {
      privacy_.Enable(command.block_local_input);
    } else if (command.flag == PrivacyCommandFlag::disable) {
      privacy_.Disable();
    }
    last_privacy_status_tick_ = 0;
  }
  const auto status = privacy_.Snapshot();
  if (status.remote_paused && !privacy_was_paused_) {
    keyboard_.ReleaseAllRemotePressedKeys("privacy_paused");
  }
#ifdef _WIN32
  if (status.remote_paused && IsWindowsPrivacyDesktopAvailable())
    devices_.ReleaseRemoteMouseButtons();
#endif
  if (privacy_was_paused_ && !status.remote_paused &&
      status.state == PrivacyState::off && start_screen_capturer_) {
    // A capture failure may have stopped the underlying capturer. Reuse the
    // normal start/retry path when the user explicitly leaves privacy mode.
    screen_capturer_is_started_ = devices_.StartScreenCapturer() == 0;
  }
  privacy_was_paused_ = status.remote_paused;
  const uint64_t now = SDL_GetTicks();
  if (!peer_ || (last_privacy_status_tick_ != 0 &&
                 status.revision == last_privacy_revision_ &&
                 now - last_privacy_status_tick_ < 1000))
    return;
  std::vector<std::string> controllers;
  {
    std::shared_lock lock(connection_status_mutex_);
    for (const auto& [id, state] : connection_status_)
      if (state == ConnectionStatus::Connected) controllers.push_back(id);
  }
  RemoteAction action{};
  action.type = ControlType::privacy_status;
  action.ps = status;
  const auto message = action.to_json();
  bool sent = true;
  for (const auto& id : controllers) {
    const int result = SendReliableDataFrameToPeer(
        peer_, message.data(), message.size(), control_data_label_.c_str(),
        id.data(), id.size());
    if (result != 0) {
      sent = false;
      LOG_WARN("Privacy status send failed, peer={}, ret={}", id, result);
    }
  }
  if (sent) {
    last_privacy_revision_ = status.revision;
    last_privacy_status_tick_ = now;
  }
}

}  // namespace crossdesk
