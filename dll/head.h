#ifndef _HEAD_H_
#define _HEAD_H_

#include "record_desktop.h"

class rd : public am::record_desktop {
 public:
  rd(){};
  virtual ~rd(){};

  int init(const RECORD_DESKTOP_RECT &rect, const int fps) { return 0; };

  int start() { return 0; };
  int pause() { return 0; };
  int resume() { return 0; };
  int stop() { return 0; };

  void clean_up() {}
};

#endif