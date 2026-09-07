/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-07
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _CUDA_GRAPHICS_H_
#define _CUDA_GRAPHICS_H_

namespace crossdesk {

// Call before SDL or Slint initializes EGL/GLX. Hybrid laptops otherwise
// create the GUI context on the integrated GPU while NVDEC uses NVIDIA.
void ConfigureLinuxCudaGraphics(bool hardware_acceleration);

}  // namespace crossdesk

#endif
