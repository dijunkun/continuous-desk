/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-09
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _INSTALLER_DOWNLOAD_H_
#define _INSTALLER_DOWNLOAD_H_

#include <cstdint>
#include <filesystem>
#include <functional>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace httplib {
class Client;
}

namespace crossdesk {

struct InstallerDownload {
  std::string origin;
  std::string path;
  std::string filename;
};

// Only accept the official Windows installer for the advertised version.
std::optional<InstallerDownload> GetWindowsInstallerDownload(
    const nlohmann::json& metadata);

// Streams to a .part file and publishes the installer only after a complete,
// successful response. Returning false from progress cancels the transfer.
bool DownloadInstaller(httplib::Client& client, const std::string& path,
                       const std::filesystem::path& destination,
                       const std::function<bool(uint64_t, uint64_t)>& progress);

}  // namespace crossdesk

#endif