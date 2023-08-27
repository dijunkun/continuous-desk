#include "system_error.h"

#include <Windows.h>

namespace am {

const std::string& system_error::error2str(unsigned long error) {
  // DWORD system_locale = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

  // HLOCAL local_buf = nullptr;

  // BOOL ret = FormatMessage(
  // 	FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
  // FORMAT_MESSAGE_ALLOCATE_BUFFER, 	NULL, error, system_locale,(PSTR)
  // &local_buf, 0, NULL);

  // if (!ret) {
  // 	HMODULE hnetmsg = LoadLibraryEx("netmsg.dll", NULL,
  // DONT_RESOLVE_DLL_REFERENCES); 	if (hnetmsg != nullptr) {
  // ret = FormatMessage( 			FORMAT_MESSAGE_FROM_HMODULE |
  // FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
  // hnetmsg, error, system_locale, (PSTR)&local_buf, 0, NULL);

  // 		FreeLibrary(hnetmsg);
  // 	}
  // }

  // std::string error_str;

  // if (ret) {
  // 	error_str = (LPCTSTR)LocalLock(local_buf);
  // 	LocalFree(local_buf);
  // }

  // return error_str;
  return "";
}

}  // namespace am