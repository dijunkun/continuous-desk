/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-09
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _WINDOWS_INSTALLER_H_
#define _WINDOWS_INSTALLER_H_

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

#include "installer_download.h"

namespace crossdesk {

bool DownloadWindowsInstaller(
    const InstallerDownload& download, const std::filesystem::path& destination,
    const std::function<bool(uint64_t, uint64_t)>& progress);

// Attachment Services records the Internet source and applies Windows policy
// and installed antivirus checks. Failure must never fall back to direct
// launch.
bool CheckWindowsInstaller(const std::filesystem::path& installer,
                           const std::string& source);
bool LaunchWindowsInstaller(const std::filesystem::path& installer,
                            const std::string& source);

}  // namespace crossdesk

#endif
