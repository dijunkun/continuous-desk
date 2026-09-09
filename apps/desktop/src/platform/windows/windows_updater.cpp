#include "windows_updater.h"

#include <httplib.h>
#include <shellapi.h>
#include <windows.h>

#include <random>
#include <stdexcept>

#include "installer_download.h"
#include "rd_log.h"

namespace crossdesk {

WindowsUpdater::~WindowsUpdater() {
  Cancel();
  if (worker_.joinable()) worker_.join();
  RemoveDownload();
}

void WindowsUpdater::RemoveDownload() {
  // A running NSIS installer still needs its source file. Leave that one in
  // the user's temporary directory; clean up cancelled/failed attempts.
  if (installer_.empty() || installer_launched_) return;
  std::error_code error;
  std::filesystem::remove(installer_, error);
  std::filesystem::remove(installer_.parent_path(), error);
}

void WindowsUpdater::Start(const nlohmann::json& metadata) {
  const State state = GetState();
  if (state == State::Downloading || state == State::Launching) return;
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
    const auto temp = std::filesystem::temp_directory_path();
    for (int attempt = 0; attempt < 10; ++attempt) {
      const auto directory =
          temp / ("CrossDesk-update-" + std::to_string(random()) + "-" +
                  std::to_string(random()));
      if (std::filesystem::create_directory(directory)) {
        installer_ = directory / download->filename;
        break;
      }
    }
    if (installer_.empty())
      throw std::runtime_error("Cannot create update directory");
    client_ = std::make_shared<httplib::Client>(download->origin);
    client_->set_connection_timeout(5);
    client_->set_read_timeout(5);
    client_->set_write_timeout(5);
    state_ = State::Downloading;
    worker_ = std::thread([this, path = download->path] {
      const bool success =
          DownloadInstaller(*client_, path, installer_,
                            [this](uint64_t received, uint64_t total) {
                              downloaded_ = received;
                              total_ = total;
                              return !cancelled_.load();
                            });
      state_ = success ? State::Ready : State::Failed;
      if (cancelled_) state_ = State::Idle;
    });
  } catch (const std::exception& error) {
    LOG_WARN("Cannot start Windows update: {}", error.what());
    state_ = State::Failed;
  }
}

void WindowsUpdater::Cancel() {
  cancelled_ = true;
  if (client_) client_->stop();
  if (GetState() == State::Ready) state_ = State::Idle;
}

void WindowsUpdater::Launch() {
  if (cancelled_) return;
  const State state = GetState();
  if (state != State::Ready && state != State::LaunchFailed &&
      state != State::Launched)
    return;
  if (worker_.joinable()) worker_.join();
  const std::wstring executable = installer_.wstring();
  const std::wstring directory = installer_.parent_path().wstring();
  state_ = State::Launching;
  SHELLEXECUTEINFOW info{};
  info.cbSize = sizeof(info);
  info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
  info.lpVerb = L"runas";
  info.lpFile = executable.c_str();
  info.lpDirectory = directory.c_str();
  info.nShow = SW_SHOWNORMAL;
  if (!ShellExecuteExW(&info)) {
    LOG_WARN("Cannot launch Windows installer, error={}", GetLastError());
    state_ = State::LaunchFailed;
    return;
  }
  if (info.hProcess) CloseHandle(info.hProcess);
  installer_launched_ = true;
  state_ = State::Launched;
  // NSIS already prompts to close CrossDesk and stops the installed service.
  // Keep this process alive if the user subsequently cancels installation.
}

}  // namespace crossdesk
