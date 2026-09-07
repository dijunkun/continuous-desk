/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-07
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _CUDA_DRIVER_H_
#define _CUDA_DRIVER_H_

#if defined(USE_CUDA) && USE_CUDA && \
    (defined(_WIN32) || \
     (defined(__linux__) && (defined(__x86_64__) || defined(__amd64__))))
#define CROSSDESK_HAS_CUDA_DRIVER 1

#include <cuda.h>

namespace crossdesk {

// Application-owned driver entry points. No codec library or MiniRTC internals
// are involved; the module remains loaded for this singleton's lifetime.
class CudaDriver {
 public:
  static const CudaDriver* Get();
  bool Check(CUresult result, const char* operation) const;

  decltype(&cuInit) init = nullptr;
  decltype(&cuDeviceGetCount) device_get_count = nullptr;
  decltype(&cuDeviceGet) device_get = nullptr;
  CUresult(CUDAAPI* context_create)(CUcontext*, unsigned int, CUdevice) = nullptr;
  decltype(&cuCtxDestroy) context_destroy = nullptr;
  decltype(&cuCtxPushCurrent) context_push = nullptr;
  decltype(&cuCtxPopCurrent) context_pop = nullptr;
  decltype(&cuGetErrorName) error_name = nullptr;
  decltype(&cuMemAlloc) mem_alloc = nullptr;
  decltype(&cuMemFree) mem_free = nullptr;
  decltype(&cuMemcpy2D) memcpy_2d = nullptr;
  decltype(&cuMemcpy2DAsync) memcpy_2d_async = nullptr;
  decltype(&cuStreamCreate) stream_create = nullptr;
  decltype(&cuStreamDestroy) stream_destroy = nullptr;
  decltype(&cuStreamSynchronize) stream_synchronize = nullptr;
  CUresult(CUDAAPI* register_gl_buffer)(CUgraphicsResource*, unsigned int,
                                       unsigned int) = nullptr;
  decltype(&cuGraphicsUnregisterResource) unregister_resource = nullptr;
  decltype(&cuGraphicsMapResources) map_resources = nullptr;
  decltype(&cuGraphicsUnmapResources) unmap_resources = nullptr;
  decltype(&cuGraphicsResourceGetMappedPointer) mapped_pointer = nullptr;

 private:
  CudaDriver();
  ~CudaDriver();
  CudaDriver(const CudaDriver&) = delete;
  CudaDriver& operator=(const CudaDriver&) = delete;
  void* Symbol(const char* name) const;
  void* module_ = nullptr;
  bool loaded_ = false;
};

}  // namespace crossdesk
#else
#define CROSSDESK_HAS_CUDA_DRIVER 0
#endif

#endif