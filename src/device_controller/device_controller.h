/*
 * @Author: DI JUNKUN
 * @Date: 2023-12-14
 * Copyright (c) 2023 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _DEVICE_CONTROLLER_H_
#define _DEVICE_CONTROLLER_H_

#include <stdio.h>

typedef enum { mouse = 0, keyboard } ControlType;
typedef enum { move = 0, left_down, left_up, right_down, right_up } MouseFlag;
typedef enum { key_down = 0, key_up } KeyFlag;
typedef struct {
  size_t x;
  size_t y;
  MouseFlag flag;
} Mouse;

typedef struct {
  size_t key_value;
  KeyFlag flag;
} Key;

typedef struct {
  ControlType type;
  union {
    Mouse m;
    Key k;
  };
} RemoteAction;

class DeviceController {
 public:
  virtual ~DeviceController() {}

 public:
  virtual int Init(int screen_width, int screen_height) = 0;
  virtual int Destroy() = 0;
  virtual int SendCommand(RemoteAction remote_action) = 0;
};

#endif