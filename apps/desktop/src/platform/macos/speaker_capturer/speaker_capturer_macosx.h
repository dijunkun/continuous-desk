/*
 * @Author: DI JUNKUN
 * @Date: 2024-08-02
 * Copyright (c) 2024 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _SPEAKER_CAPTURER_MACOSX_H_
#define _SPEAKER_CAPTURER_MACOSX_H_

#include <thread>
#include <vector>

#include "speaker_capturer.h"

namespace crossdesk {

class SpeakerCapturerMacosx : public SpeakerCapturer {
 public:
  SpeakerCapturerMacosx();
  ~SpeakerCapturerMacosx() override;

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