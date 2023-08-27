#include "utils_string.h"

#include <Windows.h>
#ifdef WIN32

#include <Windows.h>

#endif

namespace am {

std::wstring utils_string::ascii_unicode(const std::string &str) {
  int unicodeLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);

  wchar_t *pUnicode = (wchar_t *)malloc(sizeof(wchar_t) * unicodeLen);

  MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, pUnicode, unicodeLen);

  std::wstring ret_str = pUnicode;

  free(pUnicode);

  return ret_str;
}

std::string utils_string::unicode_ascii(const std::wstring &wstr) {
  int ansiiLen = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0,
                                     nullptr, nullptr);
  char *pAssii = (char *)malloc(sizeof(char) * ansiiLen);
  WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, pAssii, ansiiLen, nullptr,
                      nullptr);
  std::string ret_str = pAssii;
  free(pAssii);
  return ret_str;
}

std::string utils_string::ascii_utf8(const std::string &str) {
  return unicode_utf8(ascii_unicode(str));
}

std::string utils_string::utf8_ascii(const std::string &utf8) {
  return unicode_ascii(utf8_unicode(utf8));
}

std::string utils_string::unicode_utf8(const std::wstring &wstr) {
  int ansiiLen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0,
                                     nullptr, nullptr);
  char *pAssii = (char *)malloc(sizeof(char) * ansiiLen);
  WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, pAssii, ansiiLen, nullptr,
                      nullptr);
  std::string ret_str = pAssii;
  free(pAssii);
  return ret_str;
}

std::wstring utils_string::utf8_unicode(const std::string &utf8) {
  int unicodeLen =
      MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
  wchar_t *pUnicode = (wchar_t *)malloc(sizeof(wchar_t) * unicodeLen);
  MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, pUnicode, unicodeLen);
  std::wstring ret_str = pUnicode;
  free(pUnicode);
  return ret_str;
}

}  // namespace am