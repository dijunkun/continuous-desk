#include "log_helper.h"

#include <share.h>
#include <stdarg.h>
#include <stdio.h>

#include <mutex>

#define AMLOCK(A) std::lock_guard<std::mutex> lock(A)

#define LOG_ROLL_SIZE (1024 * 1024)

AMLog* AMLog::_log = NULL;
std::mutex _lock;

AMLog::AMLog(FILE* handle) : _handle(handle) { _log = this; }

AMLog::~AMLog() {
  AMLOCK(_lock);
  if (_log && _handle) {
    fclose(_handle);
    _log = NULL;
  }
}

AMLog* AMLog::get(const char* path) {
  if (_log || !path) {
    return _log;
  }
  // DWORD size = 0;
  // HANDLE file = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING,
  // FILE_ATTRIBUTE_NORMAL, 	NULL); if (file != INVALID_HANDLE_VALUE) {
  // size = GetFileSize(file, NULL); 	CloseHandle(file);
  // }
  // if (size != INVALID_FILE_SIZE && size > LOG_ROLL_SIZE) {
  // 	if (DeleteFileA(path) == FALSE) {
  // 		TCHAR roll_path[MAX_PATH];
  // 		sprintf_s(roll_path, MAX_PATH, "%s.1", path);
  // 		if (!MoveFileEx(path, roll_path, MOVEFILE_REPLACE_EXISTING)) {
  // 			return NULL;
  // 		}
  // 	}
  // }
  // FILE* handle = _fsopen(path, "a+", _SH_DENYNO);
  // if (!handle) {
  // 	return NULL;
  // }
  // _log = new AMLog(handle);
  return _log;
}

void AMLog::printf(const char* format, ...) {
  AMLOCK(_lock);
  va_list args;

  va_start(args, format);
  vfprintf(_handle, format, args);
  va_end(args);
  fflush(_handle);
}
