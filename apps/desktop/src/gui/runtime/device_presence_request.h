/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-07
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _DEVICE_PRESENCE_REQUEST_H_
#define _DEVICE_PRESENCE_REQUEST_H_

#include <algorithm>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace crossdesk {

inline nlohmann::json BuildDevicePresenceRequest(
    const std::string& client_id, const std::vector<std::string>& recent_ids,
    bool subscribe, const std::string& probe_id = {}) {
  std::vector<std::string> devices;
  for (const auto& id : recent_ids) {
    // Recent connection keys contain Y/N plus display settings after the ID.
    const std::string pure_id = id.substr(0, id.find_first_of("YN"));
    if (!pure_id.empty() &&
        std::find(devices.begin(), devices.end(), pure_id) == devices.end()) {
      devices.push_back(pure_id);
    }
  }
  if (!probe_id.empty() &&
      std::find(devices.begin(), devices.end(), probe_id) == devices.end()) {
    devices.push_back(probe_id);
  }
  // Include recent devices even in a one-shot probe: older servers ignore
  // subscribe=false and replace their subscriptions with this device list.
  return {{"type", "recent_connections_presence"},
          {"user_id", client_id},
          {"devices", devices},
          {"subscribe", subscribe}};
}

}  // namespace crossdesk

#endif