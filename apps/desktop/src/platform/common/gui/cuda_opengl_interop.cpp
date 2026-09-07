#include "platform/common/gui/cuda_opengl_interop.h"
#include "platform/common/gui/cuda_driver.h"

#include <memory>
#include <new>

#include "platform/common/gui/native_video_frame_ref.h"

namespace crossdesk {

#if CROSSDESK_HAS_CUDA_DRIVER

namespace {

class ScopedCudaContext {
 public:
  ScopedCudaContext(const CudaDriver& driver, CUcontext context)
      : cuda(driver),
        active_(cuda.Check(cuda.context_push(context), "cuCtxPushCurrent")) {}
  ~ScopedCudaContext() { Pop(); }
  explicit operator bool() const { return active_; }
  bool Pop() {
    if (!active_) return true;
    active_ = false;
    CUcontext context = nullptr;
    return cuda.Check(cuda.context_pop(&context), "cuCtxPopCurrent");
  }

 private:
  const CudaDriver& cuda;
  bool active_;
};

bool IsCudaFrame(const MiniRtcNativeVideoFrame* frame) {
  return frame && GetOpenGlNativeFrame(*frame) &&
         frame->type == MiniRtcNativeVideoFrameCudaNv12;
}

}  // namespace

struct CudaOpenGlVideoBuffer {
  const CudaDriver* driver = nullptr;
  CUcontext context = nullptr;
  CUstream stream = nullptr;
  CUgraphicsResource resource = nullptr;
  bool mapped = false;
  bool failed = false;
  NativeVideoFrameRef source_frame;
};

bool IsCudaDeviceAvailable() {
  const auto* cuda = CudaDriver::Get();
  int count = 0;
  return cuda && cuda->init(0) == CUDA_SUCCESS &&
         cuda->device_get_count(&count) == CUDA_SUCCESS && count > 0;
}

CudaOpenGlVideoBuffer* CreateCudaOpenGlVideoBuffer(
    const MiniRtcNativeVideoFrame* frame, uint32_t gl_buffer) {
  const auto* driver = CudaDriver::Get();
  if (!IsCudaFrame(frame) || gl_buffer == 0 || !driver) {
    return nullptr;
  }
  std::unique_ptr<CudaOpenGlVideoBuffer, decltype(&DestroyCudaOpenGlVideoBuffer)>
      buffer(new (std::nothrow) CudaOpenGlVideoBuffer,
             &DestroyCudaOpenGlVideoBuffer);
  if (!buffer) return nullptr;
  buffer->driver = driver;
  const auto& cuda = *driver;
  buffer->context = static_cast<CUcontext>(frame->payload.cuda_nv12.context);
  buffer->source_frame = NativeVideoFrameRef(frame);
  ScopedCudaContext context(cuda, buffer->context);
  if (!context ||
      !cuda.Check(cuda.stream_create(&buffer->stream,
                                     CU_STREAM_NON_BLOCKING),
                  "cuStreamCreate") ||
      !cuda.Check(cuda.register_gl_buffer(
                      &buffer->resource, gl_buffer,
                      CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD),
                  "cuGraphicsGLRegisterBuffer") ||
      !context.Pop()) {
    return nullptr;
  }
  return buffer.release();
}

int UploadCudaFrameToOpenGlBuffer(
    CudaOpenGlVideoBuffer* buffer, const MiniRtcNativeVideoFrame* frame) {
  if (!buffer || buffer->failed || !IsCudaFrame(frame) ||
      frame->payload.cuda_nv12.context != buffer->context) {
    return -1;
  }
  const auto& cuda = *buffer->driver;
  ScopedCudaContext context(cuda, buffer->context);
  if (!context) {
    buffer->failed = true;
    return -1;
  }
  // The caller has completed this buffer's previous GL use. Hold the new
  // surface through asynchronous copies, including partially failed uploads.
  buffer->source_frame = NativeVideoFrameRef(frame);
  const char* operation = "cuGraphicsMapResources";
  CUresult result =
      cuda.map_resources(1, &buffer->resource, buffer->stream);
  buffer->mapped = result == CUDA_SUCCESS;
  CUdeviceptr destination = 0;
  size_t mapped_size = 0;
  if (result == CUDA_SUCCESS) {
    operation = "cuGraphicsResourceGetMappedPointer";
    result = cuda.mapped_pointer(
        &destination, &mapped_size, buffer->resource);
  }
  const size_t y_size = static_cast<size_t>(frame->width) * frame->height;
  if (result == CUDA_SUCCESS && mapped_size < y_size + y_size / 2) {
    operation = "validate mapped buffer size";
    result = CUDA_ERROR_INVALID_VALUE;
  }
  if (result == CUDA_SUCCESS) {
    CUDA_MEMCPY2D copy{};
    copy.srcMemoryType = CU_MEMORYTYPE_DEVICE;
    copy.srcDevice = frame->payload.cuda_nv12.y_device_pointer;
    copy.srcPitch = frame->payload.cuda_nv12.y_stride;
    copy.dstMemoryType = CU_MEMORYTYPE_DEVICE;
    copy.dstDevice = destination;
    copy.dstPitch = frame->width;
    copy.WidthInBytes = frame->width;
    copy.Height = frame->height;
    operation = "cuMemcpy2DAsync (Y plane)";
    result = cuda.memcpy_2d_async(&copy, buffer->stream);
    if (result == CUDA_SUCCESS) {
      copy.srcDevice = frame->payload.cuda_nv12.uv_device_pointer;
      copy.srcPitch = frame->payload.cuda_nv12.uv_stride;
      copy.dstDevice = destination + y_size;
      copy.Height = frame->height / 2;
      operation = "cuMemcpy2DAsync (UV plane)";
      result = cuda.memcpy_2d_async(&copy, buffer->stream);
    }
  }
  if (buffer->mapped) {
    const CUresult unmap =
        cuda.unmap_resources(1, &buffer->resource, buffer->stream);
    buffer->mapped = unmap != CUDA_SUCCESS;
    if (result == CUDA_SUCCESS) {
      operation = "cuGraphicsUnmapResources";
      result = unmap;
    } else {
      cuda.Check(unmap, "cuGraphicsUnmapResources (cleanup)");
    }
  }
  const bool copied = cuda.Check(result, operation);
  const bool popped = context.Pop();
  buffer->failed = !copied || !popped;
  return buffer->failed ? -1 : 0;
}

void DestroyCudaOpenGlVideoBuffer(CudaOpenGlVideoBuffer* buffer) {
  if (!buffer) return;
  // Keep the frame's device context alive until after the context is popped.
  std::unique_ptr<CudaOpenGlVideoBuffer> owner(buffer);
  const auto& cuda = *buffer->driver;
  ScopedCudaContext context(cuda, buffer->context);
  if (!context) return;
  if (buffer->stream) {
    cuda.Check(cuda.stream_synchronize(buffer->stream),
               "cuStreamSynchronize (destroy)");
  }
  if (buffer->mapped) {
    cuda.Check(cuda.unmap_resources(
                  1, &buffer->resource, buffer->stream),
               "cuGraphicsUnmapResources (destroy)");
    cuda.Check(cuda.stream_synchronize(buffer->stream),
               "cuStreamSynchronize (unmap)");
  }
  if (buffer->resource) {
    cuda.Check(cuda.unregister_resource(buffer->resource),
               "cuGraphicsUnregisterResource");
  }
  if (buffer->stream) {
    cuda.Check(cuda.stream_destroy(buffer->stream), "cuStreamDestroy");
  }
}

#else

bool IsCudaDeviceAvailable() { return false; }
CudaOpenGlVideoBuffer* CreateCudaOpenGlVideoBuffer(
    const MiniRtcNativeVideoFrame*, uint32_t) {
  return nullptr;
}
int UploadCudaFrameToOpenGlBuffer(
    CudaOpenGlVideoBuffer*, const MiniRtcNativeVideoFrame*) {
  return -1;
}
void DestroyCudaOpenGlVideoBuffer(CudaOpenGlVideoBuffer*) {}

#endif

}  // namespace crossdesk
