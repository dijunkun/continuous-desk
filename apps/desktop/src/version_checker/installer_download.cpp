#include "installer_download.h"

namespace crossdesk {
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

}  // namespace crossdesk
