#include "windows_installer.h"

#include <windows.h>

#include <shobjidl.h>
#include <winhttp.h>
#include <wrl/client.h>

#include <array>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <system_error>

#include "rd_log.h"

namespace crossdesk {
namespace {

constexpr uint64_t kMaxInstallerSize = 1024ull * 1024 * 1024;
// Stable identity used only for Windows Attachment Services policy/UI.
constexpr GUID kAttachmentClient = {
    0x72d17a6b,
    0xeac1,
    0x4816,
    {0x91, 0x40, 0x33, 0x1b, 0xa7, 0x76, 0x22, 0xcd}};

void CheckWin32(BOOL success, const char* operation) {
  if (!success) {
    throw std::system_error(static_cast<int>(GetLastError()),
                            std::system_category(), operation);
  }
}

struct InternetCloser {
  void operator()(HINTERNET handle) const { WinHttpCloseHandle(handle); }
};
using InternetHandle = std::unique_ptr<void, InternetCloser>;

struct PartialFile {
  std::filesystem::path path;
  ~PartialFile() {
    std::error_code error;
    std::filesystem::remove(path, error);
  }
};

bool IsWindowsExecutable(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  std::array<unsigned char, 64> header{};
  input.read(reinterpret_cast<char*>(header.data()), header.size());
  if (!input || header[0] != 'M' || header[1] != 'Z') return false;
  const uint32_t pe_offset =
      uint32_t(header[60]) | (uint32_t(header[61]) << 8) |
      (uint32_t(header[62]) << 16) | (uint32_t(header[63]) << 24);
  if (pe_offset < header.size()) return false;
  input.seekg(pe_offset);
  std::array<char, 4> signature{};
  input.read(signature.data(), signature.size());
  return input && signature == std::array<char, 4>{'P', 'E', 0, 0};
}

uint64_t ContentLength(HINTERNET request) {
  std::array<wchar_t, 32> value{};
  DWORD bytes = static_cast<DWORD>(sizeof(value));
  if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_CONTENT_LENGTH,
                           WINHTTP_HEADER_NAME_BY_INDEX, value.data(), &bytes,
                           WINHTTP_NO_HEADER_INDEX)) {
    if (GetLastError() == ERROR_WINHTTP_HEADER_NOT_FOUND) return 0;
    CheckWin32(FALSE, "WinHttpQueryHeaders(Content-Length)");
  }
  const std::wstring length(value.data());
  if (length.empty() ||
      length.find_first_not_of(L"0123456789") != std::wstring::npos) {
    throw std::runtime_error("Invalid installer content length");
  }
  return std::stoull(length);
}

class ComApartment {
 public:
  ComApartment() : status_(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) {}
  ~ComApartment() {
    if (SUCCEEDED(status_)) CoUninitialize();
  }
  bool Ready() const {
    return SUCCEEDED(status_) || status_ == RPC_E_CHANGED_MODE;
  }

 private:
  HRESULT status_;
};

bool HandleAttachment(const std::filesystem::path& installer,
                      const std::string& source, bool execute) {
  ComApartment apartment;
  if (!apartment.Ready()) {
    LOG_WARN("Cannot initialize Windows Attachment Services");
    return false;
  }
  Microsoft::WRL::ComPtr<IAttachmentExecute> attachment;
  HRESULT result =
      CoCreateInstance(CLSID_AttachmentServices, nullptr, CLSCTX_INPROC_SERVER,
                       IID_PPV_ARGS(&attachment));
  const std::wstring source_url(source.begin(), source.end());
  if (SUCCEEDED(result)) result = attachment->SetClientGuid(kAttachmentClient);
  if (SUCCEEDED(result))
    result = attachment->SetClientTitle(L"CrossDesk Update");
  if (SUCCEEDED(result)) result = attachment->SetLocalPath(installer.c_str());
  if (SUCCEEDED(result)) result = attachment->SetSource(source_url.c_str());
  if (SUCCEEDED(result)) {
    if (execute) {
      HANDLE process = nullptr;
      // Execute applies attachment policy, scanning, and any required prompts.
      // The installer's manifest, rather than this process, requests elevation.
      result = attachment->Execute(GetActiveWindow(), L"open", &process);
      if (process) CloseHandle(process);
    } else {
      // Save records the source zone and invokes installed trust/AV services.
      result = attachment->Save();
    }
  }
  if (FAILED(result)) {
    LOG_WARN("Windows Attachment Services {} failed/blocked, HRESULT=0x{:08X}",
             execute ? "execute" : "save", static_cast<uint32_t>(result));
    return false;
  }
  return true;
}

}  // namespace

bool DownloadWindowsInstaller(
    const InstallerDownload& download, const std::filesystem::path& destination,
    const std::function<bool(uint64_t, uint64_t)>& progress) {
  auto partial_path = destination;
  partial_path += ".part";
  PartialFile partial{partial_path};
  try {
    if (!progress(0, 0)) return false;
    const std::string agent = "CrossDesk/" CROSSDESK_VERSION " WindowsUpdater";
    const std::wstring user_agent(agent.begin(), agent.end());
    InternetHandle session(
        WinHttpOpen(user_agent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    CheckWin32(session != nullptr, "WinHttpOpen");
    CheckWin32(WinHttpSetTimeouts(session.get(), 5000, 5000, 5000, 5000),
               "WinHttpSetTimeouts");
    InternetHandle connection(WinHttpConnect(session.get(),
                                             L"downloads.crossdesk.cn",
                                             INTERNET_DEFAULT_HTTPS_PORT, 0));
    CheckWin32(connection != nullptr, "WinHttpConnect");
    const std::wstring path(download.path.begin(), download.path.end());
    InternetHandle request(WinHttpOpenRequest(
        connection.get(), L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
    CheckWin32(request != nullptr, "WinHttpOpenRequest");
    DWORD disabled = WINHTTP_DISABLE_REDIRECTS;
    CheckWin32(WinHttpSetOption(request.get(), WINHTTP_OPTION_DISABLE_FEATURE,
                                &disabled, sizeof(disabled)),
               "Disable redirects");
    // Use Windows certificate validation and configured proxies as-is. Do not
    // ignore TLS errors, spoof a browser, or retry through alternative hosts.
    CheckWin32(WinHttpSendRequest(request.get(), WINHTTP_NO_ADDITIONAL_HEADERS,
                                  0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0),
               "WinHttpSendRequest");
    CheckWin32(WinHttpReceiveResponse(request.get(), nullptr),
               "WinHttpReceiveResponse");
    DWORD status = 0;
    DWORD status_size = sizeof(status);
    CheckWin32(WinHttpQueryHeaders(
                   request.get(),
                   WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                   WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size,
                   WINHTTP_NO_HEADER_INDEX),
               "WinHttpQueryHeaders(status)");
    if (status != 200) {
      LOG_WARN("Windows installer download returned HTTP {}", status);
      return false;
    }
    const uint64_t total = ContentLength(request.get());
    if (total > kMaxInstallerSize || !progress(0, total)) return false;
    std::ofstream output(partial.path, std::ios::binary | std::ios::trunc);
    if (!output)
      throw std::runtime_error("Cannot create installer download file");
    uint64_t received = 0;
    std::array<unsigned char, 64 * 1024> buffer{};
    while (progress(received, total)) {
      DWORD count = 0;
      CheckWin32(WinHttpReadData(request.get(), buffer.data(),
                                 static_cast<DWORD>(buffer.size()), &count),
                 "WinHttpReadData");
      if (count == 0) break;
      received += count;
      if (received > kMaxInstallerSize || (total > 0 && received > total))
        return false;
      output.write(reinterpret_cast<const char*>(buffer.data()), count);
      if (!output)
        throw std::runtime_error("Cannot write installer download file");
    }
    output.close();
    if (output.fail() || received == 0 || (total > 0 && received != total) ||
        !progress(received, total) || !IsWindowsExecutable(partial.path))
      return false;
    LOG_INFO("Windows installer downloaded, bytes={}", received);
    std::filesystem::rename(partial.path, destination);
    return true;
  } catch (const std::exception& error) {
    LOG_WARN("Windows installer download failed: {}", error.what());
    return false;
  }
}

bool CheckWindowsInstaller(const std::filesystem::path& installer,
                           const std::string& source) {
  return HandleAttachment(installer, source, false);
}

bool LaunchWindowsInstaller(const std::filesystem::path& installer,
                            const std::string& source) {
  return HandleAttachment(installer, source, true);
}

}  // namespace crossdesk
