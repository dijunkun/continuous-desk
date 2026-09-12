/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-13
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _MAC_INJECTED_EVENT_H_
#define _MAC_INJECTED_EVENT_H_

#include <cstdint>

namespace crossdesk {
// Shared by the keyboard/mouse injectors and the privacy input tap.
inline constexpr int64_t kCrossDeskInjectedEvent = 0x43524f5353444553;
}

#endif