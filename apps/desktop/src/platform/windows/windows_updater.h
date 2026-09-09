/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-09
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _WINDOWS_UPDATER_H_
#define _WINDOWS_UPDATER_H_

#include <atomic>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <thread>

namespace httplib {
class Client;
}

namespace crossdesk {

class WindowsUpdater {
 public:
  enum class State {
    Idle,
    Downloading,
    Ready,
    Launching,
    Failed,
    LaunchFailed,
    Launched
  };

  ~WindowsUpdater();
  void Start(const nlohmann::json& metadata);
  void Cancel();
  void Launch();
  State GetState() const { return state_.load(); }
  uint64_t Downloaded() const { return downloaded_.load(); }
  uint64_t Total() const { return total_.load(); }

 private:
  void RemoveDownload();

  std::atomic<State> state_{State::Idle};
  std::atomic<bool> cancelled_{false};
  std::atomic<uint64_t> downloaded_{0};
  std::atomic<uint64_t> total_{0};
  std::shared_ptr<httplib::Client> client_;
  std::thread worker_;
  std::filesystem::path installer_;
  bool installer_launched_ = false;
};

}  // namespace crossdesk

#endif