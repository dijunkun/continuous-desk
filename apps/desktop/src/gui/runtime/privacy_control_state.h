/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-12
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _PRIVACY_CONTROL_STATE_H_
#define _PRIVACY_CONTROL_STATE_H_

#include <cstdint>

#include "remote_action.h"

namespace crossdesk {

struct PrivacyControlState {
  bool active = false;
  bool enabled = false;
  bool off = false;
  bool unsupported = false;
  bool can_toggle = false;
};

// Use the same host-confirmed state for presentation and command admission.
// Missing confirmation or a failure makes only this toolbar control unavailable.
inline PrivacyControlState GetPrivacyControlState(
    const PrivacyStatus& status, bool received, bool command_pending,
    uint64_t status_age_ms, uint64_t command_age_ms) {
  PrivacyControlState result;
  const bool timeout = command_pending && command_age_ms > 15000;
  result.unsupported = timeout ||
      (received && (status_age_ms > 5000 || !status.supported ||
                    status.state == PrivacyState::failed));
  if (result.unsupported) return result;

  result.active = status.overlay_active || status.remote_paused ||
                  status.state == PrivacyState::on ||
                  status.state == PrivacyState::starting ||
                  status.state == PrivacyState::stopping ||
                  status.state == PrivacyState::failed;
  // A command in flight only gates repeated clicks; it adds no display state.
  result.enabled = received &&
                   status.state == PrivacyState::on && status.overlay_active &&
                   !status.remote_paused;
  result.off = received &&
               status.state == PrivacyState::off && !result.active;
  result.can_toggle = !command_pending && (result.active || received);
  return result;
}

}  // namespace crossdesk

#endif
