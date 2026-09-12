/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-13
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _PRIVACY_BACKEND_LINUX_H_
#define _PRIVACY_BACKEND_LINUX_H_

#include <poll.h>
#include <spawn.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <thread>

#include "privacy_backend.h"
#include "privacy_guard.h"

extern char** environ;

namespace crossdesk {
namespace {
using Clock = std::chrono::steady_clock;

class LinuxPrivacyBackend final : public PrivacyBackend {
 public:
  LinuxPrivacyBackend() : wake_(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)) {}
  ~LinuxPrivacyBackend() override {
    // EOF also recovers after a crash, SIGKILL, or a normal shutdown. Never
    // terminate the guard: it must finish restoring the saved display ramps.
    if (socket_ >= 0) close(socket_);
    if (wake_ >= 0) close(wake_);
    if (child_ > 0) {
      const pid_t child = child_;
      std::thread([child] {
        while (waitpid(child, nullptr, 0) < 0 && errno == EINTR) {
        }
      }).detach();
    }
  }

  PrivacyCapabilities Query() override {
    if (wake_ < 0)
      return {false, false, "Cannot create Linux privacy wake event"};
    if (!StartGuard()) return {false, false, failure_};
    if (!Send(LinuxPrivacyCommand::query)) return {false, false, failure_};
    // The first query also serves automatic privacy on the first connection.
    // Bound process startup so an unavailable helper cannot stall the GUI.
    if (!discovered_) {
      pollfd fd{socket_, POLLIN, 0};
      poll(&fd, 1, 1000);
    }
    ReadReplies();
    return {reply_.supported, reply_.input_supported,
            reply_.capability_reason[0] ? reply_.capability_reason
                                        : "Checking Linux privacy support"};
  }

  bool Enable(bool block_input, const PrivacyScreenText&,
              std::string& error) override {
    failure_.clear();
    emergency_ = false;
    if (!Send(block_input ? LinuxPrivacyCommand::enable_blocking
                          : LinuxPrivacyCommand::enable)) {
      error = failure_;
      return false;
    }
    engaged_ = true;
    ++generation_;
    last_reply_ = Clock::now();
    recovered_ = false;
    reply_.active = false;
    return true;
  }

  void Recover() override {
    ReadReplies();
    if (recovered_) {
      failure_.clear();
      return;
    }
    if (recovering_) return;
    if (Send(LinuxPrivacyCommand::recover)) recovering_ = true;
  }
  bool IsRecovered() const override { return recovered_; }
  bool RecoveryPending() const override { return recovering_; }

  PrivacyHealth Poll() override {
    ReadReplies();
    if (engaged_ && Clock::now() - last_reply_ > std::chrono::seconds(1))
      failure_ = "Linux privacy guard stopped responding";
    if (engaged_ &&
        Clock::now() - last_heartbeat_ >= std::chrono::milliseconds(100)) {
      Send(LinuxPrivacyCommand::heartbeat);
      last_heartbeat_ = Clock::now();
    }
    const bool emergency = emergency_;
    emergency_ = false;
    return {failure_, emergency, reply_.active};
  }

  void WaitForEvents(unsigned timeout_ms) override {
    pollfd fds[] = {{wake_, POLLIN, 0}, {socket_, POLLIN, 0}};
    poll(fds, 2, static_cast<int>(timeout_ms));
    uint64_t value;
    if (fds[0].revents & POLLIN) {
      while (read(wake_, &value, sizeof(value)) == sizeof(value)) {
      }
    }
  }
  void Wake() override {
    const uint64_t value = 1;
    if (wake_ >= 0) {
      const auto ignored = write(wake_, &value, sizeof(value));
      (void)ignored;
    }
  }

 private:
  bool StartGuard() {
    if (socket_ >= 0) return true;
    if (child_ > 0) {
      const auto result = waitpid(child_, nullptr, WNOHANG);
      if (result == 0) return false;
      child_ = -1;
    }
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0,
                   sockets) != 0) {
      failure_ = "Cannot create Linux privacy guard socket";
      return false;
    }
    posix_spawn_file_actions_t actions;
    int result = posix_spawn_file_actions_init(&actions);
    if (result == 0) {
      // Close the parent end first: it may occupy the guard's destination fd.
      result = posix_spawn_file_actions_addclose(&actions, sockets[0]);
      if (result == 0)
        result = posix_spawn_file_actions_adddup2(&actions, sockets[1],
                                                  kLinuxPrivacyGuardSocket);
      if (result == 0 && sockets[1] != kLinuxPrivacyGuardSocket)
        result = posix_spawn_file_actions_addclose(&actions, sockets[1]);
      if (result == 0) {
        char executable[] = "/proc/self/exe";
        char* args[] = {executable,
                        const_cast<char*>(kLinuxPrivacyGuardArgument), nullptr};
        result =
            posix_spawn(&child_, executable, &actions, nullptr, args, environ);
      }
      posix_spawn_file_actions_destroy(&actions);
    }
    close(sockets[1]);
    if (result != 0) {
      close(sockets[0]);
      child_ = -1;
      failure_ = std::string("Cannot start Linux privacy guard: ") +
                 std::strerror(result);
      return false;
    }
    socket_ = sockets[0];
    reply_ = {};
    discovered_ = false;
    generation_ = 0;
    failure_.clear();
    return true;
  }

  void LostGuard() {
    if (socket_ >= 0) close(socket_);
    socket_ = -1;
    reply_ = {};
    recovering_ = false;
    failure_ =
        "Linux privacy guard disconnected; local protection is unavailable";
    // Do not claim successful recovery after an unexpected guard failure.
    // A normal guard reports restoration before closing its socket.
  }
  bool Send(LinuxPrivacyCommand command) {
    if (socket_ < 0) return false;
    if (send(socket_, &command, sizeof(command), MSG_NOSIGNAL) ==
        sizeof(command))
      return true;
    if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) LostGuard();
    return false;
  }
  void ReadReplies() {
    if (socket_ < 0) return;
    for (;;) {
      LinuxPrivacyReply next;
      const auto bytes = recv(socket_, &next, sizeof(next), 0);
      if (bytes < 0 &&
          (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
        return;
      if (bytes != sizeof(next)) {
        LostGuard();
        return;
      }
      next.capability_reason[sizeof(next.capability_reason) - 1] = '\0';
      next.failure[sizeof(next.failure) - 1] = '\0';
      discovered_ = true;
      // Ignore queued capability/heartbeat replies from before this enable.
      // A matching generation can also acknowledge watchdog-driven recovery.
      if (next.generation != generation_) continue;
      reply_ = next;
      last_reply_ = Clock::now();
      emergency_ = emergency_ || next.emergency;
      if (next.failure[0]) failure_ = next.failure;
      recovered_ = next.recovered;
      if (next.recovered) {
        recovering_ = false;
        engaged_ = false;
      }
    }
  }

  int wake_ = -1;
  int socket_ = -1;
  pid_t child_ = -1;
  LinuxPrivacyReply reply_;
  uint64_t generation_ = 0;
  bool engaged_ = false;
  bool discovered_ = false;
  bool recovering_ = false;
  bool recovered_ = true;
  bool emergency_ = false;
  std::string failure_;
  Clock::time_point last_heartbeat_{};
  Clock::time_point last_reply_{};
};
}  // namespace

std::unique_ptr<PrivacyBackend> CreateLinuxPrivacyBackend() {
  return std::make_unique<LinuxPrivacyBackend>();
}
}  // namespace crossdesk

#endif
