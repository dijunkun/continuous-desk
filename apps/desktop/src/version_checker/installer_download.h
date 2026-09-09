/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-09
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _INSTALLER_DOWNLOAD_H_
#define _INSTALLER_DOWNLOAD_H_

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace crossdesk {

struct InstallerDownload {
  std::string origin;
  std::string path;
  std::string filename;
};

// Only accept the official Windows installer for the advertised version.
std::optional<InstallerDownload> GetWindowsInstallerDownload(
    const nlohmann::json& metadata);

}  // namespace crossdesk

#endif
