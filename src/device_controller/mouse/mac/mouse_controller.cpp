#include "mouse_controller.h"

#include <ApplicationServices/ApplicationServices.h>

#include "log.h"

MouseController::MouseController() {}

MouseController::~MouseController() {}

int MouseController::Init(int screen_width, int screen_height) {
  screen_width_ = screen_width;
  screen_height_ = screen_height;

  return 0;
}

int MouseController::Destroy() { return 0; }

int MouseController::SendCommand(RemoteAction remote_action) {
  int mouse_pos_x = remote_action.m.x * screen_width_ / 1280;
  int mouse_pos_y = remote_action.m.y * screen_height_ / 720;

  if (remote_action.type == ControlType::mouse) {
    CGEventRef mouse_event;
    CGEventType mouse_type;

    if (remote_action.m.flag == MouseFlag::left_down) {
      mouse_type = kCGEventLeftMouseDown;
    } else if (remote_action.m.flag == MouseFlag::left_up) {
      mouse_type = kCGEventLeftMouseUp;
    } else if (remote_action.m.flag == MouseFlag::right_down) {
      mouse_type = kCGEventRightMouseDown;
    } else if (remote_action.m.flag == MouseFlag::right_up) {
      mouse_type = kCGEventRightMouseUp;
    } else {
      mouse_type = kCGEventMouseMoved;
    }

    mouse_event = CGEventCreateMouseEvent(NULL, mouse_type,
                                          CGPointMake(mouse_pos_x, mouse_pos_y),
                                          kCGMouseButtonLeft);

    CGEventPost(kCGHIDEventTap, mouse_event);
    CFRelease(mouse_event);
  }

  return 0;
}