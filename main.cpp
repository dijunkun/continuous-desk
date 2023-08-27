
#define AMRECORDER_IMPORT
#include <iostream>

#include "export.h"
#include "head.h"

int main() {
  bool is_supported = wgc_is_supported();
  if (!wgc_is_supported) {
    std::cout << "Not support wgc" << std::endl;
    return -1;
  }

  rd rd;
  return 0;
}