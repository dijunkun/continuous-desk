#include "installer_download.h"

#include <httplib.h>

#include <array>
#include <fstream>

#include "rd_log.h"

namespace crossdesk {
namespace {

// Reject error pages and files without Windows executable headers before
// invoking the loader. The download uses the verified official HTTPS URL.
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

struct PartialFile {
  std::filesystem::path path;
  ~PartialFile() {
    std::error_code error;
    std::filesystem::remove(path, error);
  }
};

}  // namespace

std::optional<InstallerDownload> GetWindowsInstallerDownload(
    const nlohmann::json& metadata) {
  if (!metadata.is_object()) return std::nullopt;
  const auto version_it = metadata.find(
      metadata.contains("latest_version") ? "latest_version" : "version");
  if (version_it == metadata.end() || !version_it->is_string()) {
    return std::nullopt;
  }
  std::string version = version_it->get<std::string>();
  if (!version.empty() && version.front() == 'v') version.erase(0, 1);
  if (version.empty() ||
      version.find_first_not_of(
          "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-") !=
          std::string::npos) {
    return std::nullopt;
  }
  const auto downloads = metadata.find("downloads");
  if (downloads == metadata.end() || !downloads->is_object())
    return std::nullopt;
  const auto installer = downloads->find("windows-x64");
  if (installer == downloads->end() || !installer->is_object())
    return std::nullopt;
  const auto url = installer->find("url");
  const auto filename = installer->find("filename");
  InstallerDownload result{"https://downloads.crossdesk.cn", "",
                           "crossdesk-win-x64-v" + version + ".exe"};
  result.path = "/" + result.filename;
  if (url == installer->end() || !url->is_string() ||
      url->get<std::string>() != result.origin + result.path ||
      (filename != installer->end() &&
       (!filename->is_string() ||
        filename->get<std::string>() != result.filename))) {
    return std::nullopt;
  }
  return result;
}

bool DownloadInstaller(
    httplib::Client& client, const std::string& path,
    const std::filesystem::path& destination,
    const std::function<bool(uint64_t, uint64_t)>& progress) {
  auto partial_path = destination;
  partial_path += ".part";
  PartialFile partial{partial_path};
  try {
    std::ofstream output(partial.path, std::ios::binary | std::ios::trunc);
    if (!output) return false;
    uint64_t received = 0;
    uint64_t total = 0;
    bool accepted = false;
    // Do not follow a redirect to an unverified origin or downgrade to HTTP.
    client.set_follow_location(false);
    client.set_decompress(false);
    const auto response = client.Get(
        path,
        [&](const httplib::Response& headers) {
          if (headers.status != 200) return false;
          if (headers.has_header("Content-Length")) {
            total = headers.get_header_value_u64("Content-Length");
          }
          accepted = progress(0, total);
          return accepted;
        },
        [&](const char* data, size_t size) {
          if (!accepted || !progress(received, total)) return false;
          output.write(data, static_cast<std::streamsize>(size));
          received += size;
          return output.good() && progress(received, total);
        });
    output.close();
    if (!response || response->status != 200 || !accepted || output.fail() ||
        received == 0 || (total != 0 && received != total) ||
        !progress(received, total) || !IsWindowsExecutable(partial.path)) {
      LOG_WARN(
          "Installer download failed or cancelled: HTTP={}, error={}, "
          "received={}, expected={}",
          response ? response->status : 0, httplib::to_string(response.error()),
          received, total);
      return false;
    }
    std::filesystem::rename(partial.path, destination);
    return true;
  } catch (const std::exception& error) {
    LOG_WARN("Installer download failed: {}", error.what());
    return false;
  }
}

}  // namespace crossdesk
