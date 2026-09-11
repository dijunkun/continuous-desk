/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-10
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#include "privacy_input_guard.h"

#include "rd_log.h"
#include "windows_input_marker.h"

namespace crossdesk {

PrivacyInputGuard::~PrivacyInputGuard() { Stop(); }

void PrivacyInputGuard::Fault(const std::string& operation, DWORD code) {
  {
    std::lock_guard lock(error_mutex_);
    error_ = operation + " (Windows error " + std::to_string(code) + ")";
    LOG_ERROR("Privacy input: {}", error_);
  }
  healthy_.store(false);
  // Wake the owner's message loop; recovery is never performed in a hook.
  PostMessageW(owner_, WM_NULL, 0, 0);
}

bool PrivacyInputGuard::Start(HWND owner, UINT exit_message,
                              std::string& error) {
  if (thread_.joinable()) {
    error = "Privacy input thread is already running";
    return false;
  }
  owner_ = owner;
  exit_message_ = exit_message;
  healthy_.store(false);
  {
    std::lock_guard lock(error_mutex_);
    error_.clear();
  }
  stop_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
  ready_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
  if (!stop_ || !ready_) {
    Fault("Create privacy input events failed", GetLastError());
    Healthy(error);
    Stop();
    return false;
  }
  try {
    thread_ = std::thread([this] { Run(); });
  } catch (const std::exception& e) {
    error = std::string("Start privacy input thread failed: ") + e.what();
    Stop();
    return false;
  }
  if (WaitForSingleObject(ready_, 2000) != WAIT_OBJECT_0) {
    error = "Privacy input thread startup timed out";
    Stop();
    return false;
  }
  if (!Healthy(error)) {
    Stop();
    return false;
  }
  return true;
}

void PrivacyInputGuard::Stop() {
  if (stop_) SetEvent(stop_);
  if (thread_.joinable()) thread_.join();
  if (stop_) CloseHandle(stop_);
  if (ready_) CloseHandle(ready_);
  stop_ = ready_ = nullptr;
  owner_ = nullptr;
}

bool PrivacyInputGuard::Healthy(std::string& error) const {
  if (healthy_.load() && running_.load() &&
      GetTickCount64() - last_pump_.load() < 1500)
    return true;
  std::lock_guard lock(error_mutex_);
  error = error_.empty() ? "Privacy input message thread stopped responding"
                         : error_;
  return false;
}

bool PrivacyInputGuard::Renew(HHOOK& hook, int kind, HOOKPROC procedure) {
  HHOOK replacement =
      SetWindowsHookExW(kind, procedure, GetModuleHandleW(nullptr), 0);
  if (!replacement) {
    Fault("Renew privacy input hook failed", GetLastError());
    return false;
  }
  HHOOK previous = hook;
  hook = replacement;
  if (UnhookWindowsHookEx(previous)) return true;
  const DWORD code = GetLastError();
  if (code != ERROR_INVALID_HOOK_HANDLE) retired_.push_back(previous);
  Fault("Privacy input hook was lost or could not be retired", code);
  return false;
}

bool PrivacyInputGuard::PumpMessages() {
  MSG message{};
  while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
    if (message.message == WM_QUIT) return false;
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
  last_pump_.store(GetTickCount64());
  return WaitForSingleObject(stop_, 0) == WAIT_TIMEOUT;
}

bool PrivacyInputGuard::ReleasePressedInputs() {
  // Both hooks must already be installed: otherwise a new local press can
  // slip between this snapshot and the transition to blocked input.
  std::array<bool, 256> pressed{};
  for (int key = 1; key < 256; ++key)
    pressed[key] = (GetAsyncKeyState(key) & 0x8000) != 0;
  // Keep held modifiers available to the physical emergency-exit chord.
  // Our injected releases intentionally do not clear this hook-side state.
  physical_keys_ = pressed;

  std::vector<INPUT> releases;
  for (int key = 1; key < 256; ++key) {
    if (!pressed[key]) continue;
    // Generic modifier VKs alias their left/right variants. Release each
    // side once; use the generic VK only if Windows reports no specific side.
    if ((key == VK_SHIFT && (pressed[VK_LSHIFT] || pressed[VK_RSHIFT])) ||
        (key == VK_CONTROL && (pressed[VK_LCONTROL] || pressed[VK_RCONTROL])) ||
        (key == VK_MENU && (pressed[VK_LMENU] || pressed[VK_RMENU])))
      continue;
    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dwExtraInfo = kInjectedMouseInputMarker;
    switch (key) {
      // GetAsyncKeyState reports physical mouse buttons, as do these flags,
      // including when Windows swaps the primary and secondary buttons.
      case VK_LBUTTON:
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        break;
      case VK_RBUTTON:
        input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        break;
      case VK_MBUTTON:
        input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        break;
      case VK_XBUTTON1:
      case VK_XBUTTON2:
        input.mi.dwFlags = MOUSEEVENTF_XUP;
        input.mi.mouseData = key == VK_XBUTTON1 ? XBUTTON1 : XBUTTON2;
        break;
      default: {
        input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = static_cast<WORD>(key);
        const UINT scan = MapVirtualKeyW(key, MAPVK_VK_TO_VSC_EX);
        input.ki.wScan = static_cast<WORD>(scan & 0xff);
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        if ((scan & 0xff00) == 0xe000)
          input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        input.ki.dwExtraInfo = kInjectedKeyboardInputMarker;
        break;
      }
    }
    LOG_INFO("Privacy input handoff: releasing held input vk=0x{:02X}", key);
    releases.push_back(input);
  }
  if (!releases.empty()) {
    const UINT count = static_cast<UINT>(releases.size());
    SetLastError(ERROR_SUCCESS);
    const UINT sent = SendInput(count, releases.data(), sizeof(INPUT));
    if (sent != count) {
      Fault("Release held privacy inputs failed (sent " + std::to_string(sent) +
                "/" + std::to_string(count) + ")",
            GetLastError());
      return false;
    }
  }

  // SendInput acknowledges insertion, not delivery. Pump this hook thread
  // until the tagged releases have passed through and Windows reports no
  // pressed inputs. The caller cannot publish input_blocked/ready earlier.
  const auto deadline = GetTickCount64() + 1000;
  for (;;) {
    if (!PumpMessages()) {
      Fault("Privacy input handoff cancelled", ERROR_CANCELLED);
      return false;
    }
    int held = 0;
    for (int key = 1; key < 256; ++key) {
      if (GetAsyncKeyState(key) & 0x8000) {
        held = key;
        break;
      }
    }
    if (!held) {
      LOG_INFO("Privacy input handoff complete: released {} held inputs",
               releases.size());
      return true;
    }
    if (GetTickCount64() >= deadline) {
      Fault("Privacy input release timed out, still pressed vk=" +
                std::to_string(held),
            ERROR_TIMEOUT);
      return false;
    }
    if (MsgWaitForMultipleObjectsEx(1, &stop_, 10, QS_ALLINPUT,
                                    MWMO_INPUTAVAILABLE) == WAIT_FAILED) {
      Fault("Wait for privacy input releases failed", GetLastError());
      return false;
    }
  }
}

void PrivacyInputGuard::Run() {
  current_ = this;
  running_.store(true);
  try {
    keyboard_ = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHook,
                              GetModuleHandleW(nullptr), 0);
    if (!keyboard_)
      Fault("Install privacy keyboard hook failed", GetLastError());
    if (keyboard_) {
      mouse_ =
          SetWindowsHookExW(WH_MOUSE_LL, MouseHook, GetModuleHandleW(nullptr), 0);
      if (!mouse_) Fault("Install privacy mouse hook failed", GetLastError());
    }
    healthy_.store(keyboard_ && mouse_ && ReleasePressedInputs());
    last_pump_.store(GetTickCount64());
    SetEvent(ready_);
    auto last_refresh = GetTickCount64();
    LOG_INFO("Privacy input message thread started: thread={}",
             GetCurrentThreadId());
    while (keyboard_ && mouse_ &&
           WaitForSingleObject(stop_, 0) == WAIT_TIMEOUT) {
      if (!PumpMessages()) break;
      const auto now = GetTickCount64();
      if (healthy_.load() && now - last_refresh >= 1000) {
        last_refresh = now;
        // Install replacements before retiring old hooks, with no input gap.
        if (Renew(keyboard_, WH_KEYBOARD_LL, KeyboardHook))
          Renew(mouse_, WH_MOUSE_LL, MouseHook);
      }
      if (MsgWaitForMultipleObjectsEx(1, &stop_, 100, QS_ALLINPUT,
                                      MWMO_INPUTAVAILABLE) == WAIT_FAILED) {
        Fault("Wait for privacy input messages failed", GetLastError());
        break;
      }
    }
  } catch (const std::exception& e) {
    Fault(std::string("Privacy input thread failed: ") + e.what(),
          ERROR_GEN_FAILURE);
    SetEvent(ready_);
  }
  healthy_.store(false);
  auto unhook = [this](HHOOK hook) {
    if (hook && !UnhookWindowsHookEx(hook) &&
        GetLastError() != ERROR_INVALID_HOOK_HANDLE)
      Fault("Release privacy input hook failed", GetLastError());
  };
  unhook(keyboard_);
  unhook(mouse_);
  for (auto hook : retired_) unhook(hook);
  keyboard_ = mouse_ = nullptr;
  retired_.clear();
  physical_keys_.fill(false);
  current_ = nullptr;
  running_.store(false);
  PostMessageW(owner_, WM_NULL, 0, 0);
  // Windows also removes hooks installed by a thread when that thread exits.
  LOG_INFO("Privacy input message thread stopped");
}

LRESULT CALLBACK PrivacyInputGuard::KeyboardHook(int code, WPARAM wp,
                                                 LPARAM lp) {
  auto self = current_;
  if (code < 0 || !self) return CallNextHookEx(nullptr, code, wp, lp);
  const auto& key = *reinterpret_cast<KBDLLHOOKSTRUCT*>(lp);
  if ((key.flags & LLKHF_INJECTED) &&
      key.dwExtraInfo == kInjectedKeyboardInputMarker)
    return CallNextHookEx(nullptr, code, wp, lp);
  if (!(key.flags & LLKHF_INJECTED) && key.vkCode < 256) {
    self->physical_keys_[key.vkCode] = !(key.flags & LLKHF_UP);
    const auto& keys = self->physical_keys_;
    if (key.vkCode == VK_F12 && !(key.flags & LLKHF_UP) &&
        (keys[VK_LCONTROL] || keys[VK_RCONTROL]) &&
        (keys[VK_LMENU] || keys[VK_RMENU]) &&
        (keys[VK_LSHIFT] || keys[VK_RSHIFT]))
      PostMessageW(self->owner_, self->exit_message_, 0, 0);
  }
  return 1;
}

LRESULT CALLBACK PrivacyInputGuard::MouseHook(int code, WPARAM wp, LPARAM lp) {
  auto self = current_;
  if (code < 0 || !self) return CallNextHookEx(nullptr, code, wp, lp);
  const auto& mouse = *reinterpret_cast<MSLLHOOKSTRUCT*>(lp);
  if ((mouse.flags & LLMHF_INJECTED) &&
      mouse.dwExtraInfo == kInjectedMouseInputMarker)
    return CallNextHookEx(nullptr, code, wp, lp);
  return 1;
}

}  // namespace crossdesk
