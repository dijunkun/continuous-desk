#include <X11/Xlib.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>

using namespace std;

int fd;
Display *dpy;

void initMouse();
void destroyMouse();
void mouseLeftClick();
void mouseRightClick();
void mouseGetPosition(int &x, int &y);
void mouseMove(int xdelta, int ydelta);

void initMouse() {
  fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  ioctl(fd, UI_SET_EVBIT, EV_KEY);
  ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
  ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
  ioctl(fd, UI_SET_EVBIT, EV_ABS);
  ioctl(fd, UI_SET_ABSBIT, ABS_X);
  ioctl(fd, UI_SET_ABSBIT, ABS_Y);
  ioctl(fd, UI_SET_EVBIT, EV_REL);

  struct uinput_user_dev uidev;
  memset(&uidev, 0, sizeof(uidev));
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "VirtualMouse");
  uidev.id.bustype = BUS_USB;
  uidev.id.version = 1;
  uidev.id.vendor = 0x1;
  uidev.id.product = 0x1;
  uidev.absmin[ABS_X] = 0;
  uidev.absmax[ABS_X] = 3200;
  uidev.absmin[ABS_Y] = 0;
  uidev.absmax[ABS_Y] = 900;

  write(fd, &uidev, sizeof(uidev));
  ioctl(fd, UI_DEV_CREATE);

  sleep(2);
}

void mouseLeftClick() {
  struct input_event ev_click, ev_sync;
  memset(&ev_click, 0, sizeof(ev_click));
  memset(&ev_sync, 0, sizeof(ev_sync));

  ev_click.type = EV_KEY;
  ev_click.code = BTN_LEFT;
  ev_click.value = 1;

  // write left click event
  write(fd, &ev_click, sizeof(ev_click));

  // sync left click event
  ev_sync.type = EV_SYN;
  write(fd, &ev_sync, sizeof(ev_sync));
}

void mouseRightClick() {
  struct input_event ev_click, ev_sync;
  memset(&ev_click, 0, sizeof(ev_click));
  memset(&ev_sync, 0, sizeof(ev_sync));

  ev_click.type = EV_KEY;
  ev_click.code = BTN_RIGHT;
  ev_click.value = 1;

  // write right click event
  write(fd, &ev_click, sizeof(ev_click));

  // sync right click event
  ev_sync.type = EV_SYN;
  write(fd, &ev_sync, sizeof(ev_sync));
}

void mouseSetPosition(int x, int y) {
  struct input_event ev[2], ev_sync;
  memset(ev, 0, sizeof(ev));
  memset(&ev_sync, 0, sizeof(ev_sync));

  ev[0].type = EV_ABS;
  ev[0].code = ABS_X;
  ev[0].value = x;
  ev[1].type = EV_ABS;
  ev[1].code = ABS_Y;
  ev[1].value = y;

  int res_w = write(fd, ev, sizeof(ev));

  std::cout << "res w : " << res_w << "\n";

  ev_sync.type = EV_SYN;
  ev_sync.value = 0;
  ev_sync.code = 0;
  int res_ev_sync = write(fd, &ev_sync, sizeof(ev_sync));

  std::cout << "res syn : " << res_ev_sync << "\n";
}

void initDisplay() { dpy = XOpenDisplay(NULL); }

void destroyMouse() { ioctl(fd, UI_DEV_DESTROY); }

void mouseMove(int xdelta, int ydelta) {
  int xx, yy;
  mouseGetPosition(xx, yy);

  mouseSetPosition(xx + xdelta, yy + ydelta);
}

void mouseGetPosition(int &x, int &y) {
  Window root, child;
  int rootX, rootY, winX, winY;
  unsigned int mask;

  XQueryPointer(dpy, DefaultRootWindow(dpy), &root, &child, &rootX, &rootY,
                &winX, &winY, &mask);

  std::cout << "root x : " << rootX << "\n";
  std::cout << "root y : " << rootY << "\n";

  x = rootX;
  y = rootY;
}

int main() {
  initMouse();
  initDisplay();

  int tempx, tempy;

  for (int i = 0; i < 5; ++i) {
    mouseMove(100, 100);
    sleep(1);
    std::cout << "i : " << i << "\n";
  }

  destroyMouse();
  return 0;
}