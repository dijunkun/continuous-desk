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
  bool warning = false;
  bool stale = false;
  bool timeout = false;
  bool can_toggle = false;
};

// Use the same host-confirmed state for presentation and command admission.
// An engaged or failed screen can still be disabled when its status goes stale.
inline PrivacyControlState GetPrivacyControlState(
    const PrivacyStatus& status, bool received, bool command_pending,
    uint64_t status_age_ms, uint64_t command_age_ms) {
  PrivacyControlState result;
  result.timeout = command_pending && command_age_ms > 15000;
  result.stale = received && status_age_ms > 5000;
  const bool request_pending = command_pending && !result.timeout;
  const bool fresh = received && !result.stale;
  result.active = status.overlay_active || status.remote_paused ||
                  status.state == PrivacyState::on ||
                  status.state == PrivacyState::starting ||
                  status.state == PrivacyState::stopping ||
                  status.state == PrivacyState::failed;
  // A command in flight only gates repeated clicks; it adds no display state.
  result.enabled = fresh && !result.timeout &&
                   status.state == PrivacyState::on && status.overlay_active &&
                   !status.remote_paused;
  result.off = fresh && !result.timeout &&
               status.state == PrivacyState::off && !result.active;
  result.unsupported = received && !status.supported;
  result.warning = result.timeout || result.stale ||
      (received && status.state == PrivacyState::failed);
  result.can_toggle = !request_pending &&
                      (result.active || (fresh && status.supported));
  return result;
}

}  // namespace crossdesk

#endif
