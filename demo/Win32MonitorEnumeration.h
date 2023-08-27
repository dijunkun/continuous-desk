#pragma once
#include <dwmapi.h>

struct Monitor {
public:
  Monitor(nullptr_t) {}
  Monitor(HMONITOR hmonitor, std::wstring &className, bool isPrimary) {
    m_hmonitor = hmonitor;
    m_className = className;
    m_bIsPrimary = isPrimary;
  }

  HMONITOR Hmonitor() const noexcept { return m_hmonitor; }
  std::wstring ClassName() const noexcept { return m_className; }
  bool IsPrimary() const noexcept { return m_bIsPrimary; }

private:
  HMONITOR m_hmonitor;
  std::wstring m_className;
  bool m_bIsPrimary;
};

BOOL WINAPI EnumMonitorProc(HMONITOR hmonitor, HDC hdc, LPRECT lprc,
                            LPARAM data) {

  MONITORINFOEX info_ex;
  info_ex.cbSize = sizeof(MONITORINFOEX);

  GetMonitorInfo(hmonitor, &info_ex);

  if (info_ex.dwFlags == DISPLAY_DEVICE_MIRRORING_DRIVER)
    return true;

  auto monitors = ((std::vector<Monitor> *)data);
  std::wstring name = info_ex.szDevice;
  auto monitor =
      Monitor(hmonitor, name, info_ex.dwFlags & MONITORINFOF_PRIMARY);

  monitors->emplace_back(monitor);

  return true;
}

std::vector<Monitor> EnumerateMonitors() {
  std::vector<Monitor> monitors;

  ::EnumDisplayMonitors(NULL, NULL, EnumMonitorProc, (LPARAM)&monitors);

  return monitors;
}