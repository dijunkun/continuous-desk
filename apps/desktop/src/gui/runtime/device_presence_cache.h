/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-04
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _DEVICE_PRESENCE_CACHE_H_
#define _DEVICE_PRESENCE_CACHE_H_

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

namespace crossdesk {

class DevicePresenceCache {
public:
  using Clock = std::chrono::steady_clock;
  static constexpr auto kMaxAge = std::chrono::seconds(60);

  void SetSignalConnected(bool connected) {
    std::lock_guard<std::mutex> lock(mutex_);
    signal_connected_ = connected;
    cache_.clear();
  }

  void SetOnline(const std::string &device_id, bool online,
                 Clock::time_point now = Clock::now()) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (signal_connected_) {
      cache_[device_id] = {online, now};
    }
  }

  bool IsOnline(const std::string &device_id,
                Clock::time_point now = Clock::now()) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = cache_.find(device_id);
    return signal_connected_ && it != cache_.end() && it->second.online &&
           now - it->second.updated_at < kMaxAge;
  }

  void Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
  }

private:
  struct Entry {
    bool online;
    Clock::time_point updated_at;
  };
  bool signal_connected_ = false;
  std::unordered_map<std::string, Entry> cache_;
  mutable std::mutex mutex_;
};

} // namespace crossdesk

#endif