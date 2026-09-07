#include "platform/linux/gui/cuda_graphics.h"

#include <cstdlib>
#include <initializer_list>

#include "rd_log.h"

#include "platform/common/gui/cuda_opengl_interop.h"

namespace crossdesk {

void ConfigureLinuxCudaGraphics(bool hardware_acceleration) {
#if USE_CUDA && (defined(__x86_64__) || defined(__amd64__))
  if (!hardware_acceleration) {
    return;
  }

  // Respect explicit GPU/driver selection, including a request to use Mesa
  // or software rendering. Native frames can still use the CPU fallback.
  for (const char* name : {"__NV_PRIME_RENDER_OFFLOAD",
                           "__NV_PRIME_RENDER_OFFLOAD_PROVIDER",
                           "__GLX_VENDOR_LIBRARY_NAME",
                           "__EGL_VENDOR_LIBRARY_FILENAMES",
                           "__EGL_VENDOR_LIBRARY_DIRS", "DRI_PRIME",
                           "LIBGL_ALWAYS_SOFTWARE",
                           "MESA_LOADER_DRIVER_OVERRIDE"}) {
    const char* value = std::getenv(name);
    if (value && value[0] != '\0') {
      LOG_INFO("Keeping explicit Linux graphics selection: {}={}", name,
               value);
      return;
    }
  }

  if (!IsCudaDeviceAvailable()) {
    return;
  }

  // Both EGL and GLX need to render on NVIDIA to share CUDA frame buffers.
  // These are process-local, not desktop settings.
  // https://download.nvidia.com/XFree86/Linux-x86_64/595.45.04/README/primerenderoffload.html
  if (setenv("__NV_PRIME_RENDER_OFFLOAD", "1", 1) != 0 ||
      setenv("__GLX_VENDOR_LIBRARY_NAME", "nvidia", 1) != 0) {
    LOG_WARN("Unable to select NVIDIA PRIME rendering for CUDA video");
    return;
  }
  LOG_INFO("Selected NVIDIA PRIME rendering for CUDA/OpenGL video interop");
#else
  (void)hardware_acceleration;
#endif
}

}  // namespace crossdesk
