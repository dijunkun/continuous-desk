#include "platform/common/gui/cuda_driver.h"

#if CROSSDESK_HAS_CUDA_DRIVER
#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <type_traits>

#include "rd_log.h"

namespace crossdesk {

const CudaDriver* CudaDriver::Get() {
  static const CudaDriver driver;
  return driver.loaded_ ? &driver : nullptr;
}

void* CudaDriver::Symbol(const char* name) const {
#if defined(_WIN32)
  return reinterpret_cast<void*>(
      GetProcAddress(static_cast<HMODULE>(module_), name));
#else
  return dlsym(module_, name);
#endif
}

CudaDriver::CudaDriver() {
#if defined(_WIN32)
  module_ = LoadLibraryW(L"nvcuda.dll");
#else
  module_ = dlopen("libcuda.so.1", RTLD_LAZY | RTLD_LOCAL);
#endif
  if (!module_) return;
  const auto load = [this](auto& function, const char* name) {
    function = reinterpret_cast<std::decay_t<decltype(function)>>(Symbol(name));
    return function != nullptr;
  };
  loaded_ = load(init, "cuInit") &&
      load(device_get_count, "cuDeviceGetCount") &&
      load(device_get, "cuDeviceGet") &&
      load(context_create, "cuCtxCreate_v2") &&
      load(context_destroy, "cuCtxDestroy_v2") &&
      load(context_push, "cuCtxPushCurrent_v2") &&
      load(context_pop, "cuCtxPopCurrent_v2") &&
      load(error_name, "cuGetErrorName") &&
      load(mem_alloc, "cuMemAlloc_v2") &&
      load(mem_free, "cuMemFree_v2") &&
      load(memcpy_2d, "cuMemcpy2D_v2") &&
      load(memcpy_2d_async, "cuMemcpy2DAsync_v2") &&
      load(stream_create, "cuStreamCreate") &&
      load(stream_destroy, "cuStreamDestroy_v2") &&
      load(stream_synchronize, "cuStreamSynchronize") &&
      load(register_gl_buffer, "cuGraphicsGLRegisterBuffer") &&
      load(unregister_resource, "cuGraphicsUnregisterResource") &&
      load(map_resources, "cuGraphicsMapResources") &&
      load(unmap_resources, "cuGraphicsUnmapResources") &&
      load(mapped_pointer, "cuGraphicsResourceGetMappedPointer_v2");
}

CudaDriver::~CudaDriver() {
  if (!module_) return;
#if defined(_WIN32)
  FreeLibrary(static_cast<HMODULE>(module_));
#else
  dlclose(module_);
#endif
}

bool CudaDriver::Check(CUresult result, const char* operation) const {
  if (result == CUDA_SUCCESS) return true;
  const char* name = nullptr;
  error_name(result, &name);
  LOG_WARN("CUDA/OpenGL {} failed: {} ({})", operation,
           name ? name : "unknown CUDA error", static_cast<int>(result));
  return false;
}

}  // namespace crossdesk
#endif
