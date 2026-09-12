/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-13
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _PRIVACY_GUARD_H_
#define _PRIVACY_GUARD_H_

#include <cstdint>

namespace crossdesk {

inline constexpr char kLinuxPrivacyGuardArgument[] = "--linux-privacy-guard";
inline constexpr int kLinuxPrivacyGuardSocket = 3;

// Private protocol over an inherited SOCK_SEQPACKET socket. No listening
// socket, shell, elevated privileges or separately installed helper is needed.
enum class LinuxPrivacyCommand : uint8_t {
  query,
  enable,
  enable_blocking,
  recover,
  heartbeat,
};

struct LinuxPrivacyReply {
  uint64_t generation = 0;
  bool supported = false;
  bool input_supported = false;
  bool active = false;
  bool recovered = true;
  bool emergency = false;
  char capability_reason[256]{};
  char failure[256]{};
};

int RunLinuxPrivacyGuard();

}  // namespace crossdesk

#endif