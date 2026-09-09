#include "windows_updater.h"

#include <shlobj.h>
#include <windows.h>

#include <random>
#include <stdexcept>

#include "installer_download.h"
#include "rd_log.h"
#include "windows_installer.h"

namespace crossdesk {

WindowsUpdater::~WindowsUpdater() {
  Cancel();
  if (worker_.joinable()) worker_.join();
  RemoveDownload();
}

void WindowsUpdater::RemoveDownload() {
  // A running NSIS installer still needs its source file. Leave that one in
  // the user's update cache; clean up cancelled/failed attempts.
  if (installer_.empty() || installer_launched_) return;
  std::error_code error;
  std::filesystem::remove(installer_, error);
  std::filesystem::remove(installer_.parent_path(), error);
}

void WindowsUpdater::Start(const nlohmann::json& metadata) {
  const State state = GetState();
  if (state == State::Downloading || state == State::Checking ||
      state == State::Launching || state == State::SecurityBlocked)
    return;
  if (state == State::Ready || state == State::LaunchFailed ||
      state == State::Launched) {
    cancelled_ = false;
    Launch();
    return;
  }
  if (worker_.joinable()) worker_.join();
  RemoveDownload();
  installer_.clear();
  installer_launched_ = false;
  cancelled_ = false;
  downloaded_ = 0;
  total_ = 0;
  const auto download = GetWindowsInstallerDownload(metadata);
  if (!download) {
    LOG_WARN("No valid Windows installer for the advertised version");
    state_ = State::Failed;
    return;
  }
  try {
    std::random_device random;
    PWSTR local_app_data = nullptr;
    const HRESULT path_result = SHGetKnownFolderPath(
        FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &local_app_data);
    if (FAILED(path_result))
      throw std::runtime_error("Cannot locate LocalAppData");
    const std::filesystem::path data_path(local_app_data);
    CoTaskMemFree(local_app_data);
    const auto updates = data_path / L"CrossDesk" / L"Updates";
    std::filesystem::create_directories(updates);
    for (int attempt = 0; attempt < 10; ++attempt) {
      const auto directory = updates / ("download-" + std::to_string(random()) +
                                        "-" + std::to_string(random()));
      if (std::filesystem::create_directory(directory)) {
        installer_ = directory / download->filename;
        break;
      }
    }
    if (installer_.empty())
      throw std::runtime_error("Cannot create update directory");
    source_url_ = download->origin + download->path;
    state_ = State::Downloading;
    worker_ = std::thread([this, download = *download] {
      const bool success = DownloadWindowsInstaller(
          download, installer_, [this](uint64_t received, uint64_t total) {
            downloaded_ = received;
            total_ = total;
            return !cancelled_.load();
          });
      if (success && !cancelled_) {
        state_ = State::Checking;
        state_ = CheckWindowsInstaller(installer_, source_url_)
                     ? State::Ready
                     : State::SecurityBlocked;
      } else {
        state_ = State::Failed;
      }
      if (cancelled_) state_ = State::Idle;
    });
  } catch (const std::exception& error) {
    LOG_WARN("Cannot start Windows update: {}", error.what());
    state_ = State::Failed;
  }
}

void WindowsUpdater::Cancel() {
  cancelled_ = true;
  if (GetState() == State::Ready) state_ = State::Idle;
}

void WindowsUpdater::Launch() {
  if (cancelled_) return;
  const State state = GetState();
  if (state != State::Ready && state != State::LaunchFailed &&
      state != State::Launched)
    return;
  if (worker_.joinable()) worker_.join();
  state_ = State::Launching;
  if (!LaunchWindowsInstaller(installer_, source_url_)) {
    state_ = State::LaunchFailed;
    return;
  }
  installer_launched_ = true;
  state_ = State::Launched;
  // NSIS already prompts to close CrossDesk and stops the installed service.
  // Keep this process alive if the user subsequently cancels installation.
}

}  // namespace crossdesk
