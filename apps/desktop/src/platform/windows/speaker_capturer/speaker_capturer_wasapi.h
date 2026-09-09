/*
 * @Author: DI JUNKUN
 * @Date: 2024-08-15
 * Copyright (c) 2024 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _SPEAKER_CAPTURER_WASAPI_H_
#define _SPEAKER_CAPTURER_WASAPI_H_

#include "speaker_capturer.h"

namespace crossdesk {

class SpeakerCapturerWasapi : public SpeakerCapturer {
 public:
  SpeakerCapturerWasapi();
  ~SpeakerCapturerWasapi() override;

 public:
  int Init(speaker_data_cb cb) override;
  int Destroy() override;
  int Start() override;
  int Stop() override;
  bool IsRunning() const override;

  int Pause();
  int Resume();

 private:
  class Impl;
  Impl* impl_ = nullptr;
};
}  // namespace crossdesk
#endif