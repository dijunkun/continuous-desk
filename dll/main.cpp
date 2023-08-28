
#define AMRECORDER_IMPORT
#include <iostream>

#include "export.h"
#include "head.h"
#include "record_desktop_wgc.h"

int main() {
  // bool is_supported = wgc_is_supported();
  // if (!wgc_is_supported) {
  //   std::cout << "Not support wgc" << std::endl;
  //   return -1;
  // }

  static am::record_desktop *recorder = new am::record_desktop_wgc();

  RECORD_DESKTOP_RECT rect;
  rect.left = 0;
  rect.top = 0;
  rect.right = GetSystemMetrics(SM_CXSCREEN);
  rect.bottom = GetSystemMetrics(SM_CYSCREEN);

  recorder->init(rect, 10);

  recorder->start();
  // int pause() override;
  // int resume() override;
  // int stop() override;

  while (1) {
  }

  return 0;
}