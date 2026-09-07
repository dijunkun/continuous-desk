/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-07
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _CUDA_OPENGL_INTEROP_H_
#define _CUDA_OPENGL_INTEROP_H_

#include "minirtc.h"

namespace crossdesk {

bool IsCudaDeviceAvailable();

struct CudaOpenGlVideoBuffer;

// These operations require the matching GL context to be current. The caller
// owns the GL buffer and must allocate its storage before registering it.
CudaOpenGlVideoBuffer* CreateCudaOpenGlVideoBuffer(
    const MiniRtcNativeVideoFrame* frame, uint32_t gl_buffer);

// Asynchronous, GPU-only NV12 copy. Retains the source through the next upload
// or destruction, including on failure. Before reusing the buffer, the caller
// must wait for the previous GL fence. Frames must use the same CUDA context.
int UploadCudaFrameToOpenGlBuffer(CudaOpenGlVideoBuffer* buffer,
                                 const MiniRtcNativeVideoFrame* frame);

// Finish prior GL use before calling. Waits for CUDA work and releases retained
// frames; the caller may then resize/delete the GL buffer. Accepts null.
void DestroyCudaOpenGlVideoBuffer(CudaOpenGlVideoBuffer* buffer);

}  // namespace crossdesk

#endif