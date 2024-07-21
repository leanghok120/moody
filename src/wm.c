#include <stdio.h>
#include <stdlib.h>

#include "wm.h"

WindowManager wm;

int init_x() {
  wm.display = XOpenDisplay(NULL);
  if (!wm.display) {
    fprintf(stderr, "Failed to open display\n");
    return 0;
  }

  wm.screen = DefaultScreen(wm.display);
  wm.root = RootWindow(wm.display, wm.screen);

  return 1;
}

void run_event_loop() {
  XEvent ev;

  while (1) {
    XNextEvent(wm.display, &ev);
    switch (ev.type) {
      case MapRequest:
        // Handle new window mapping
        break;
      case ConfigureRequest:
        // Handle window configure requests
        break;
      case DestroyNotify:
        // Handle window destruction
        break;
        // Add more event handling as needed
    }
  }
}

void cleanup_x() {
  if (wm.display) {
    XCloseDisplay(wm.display);
  }
}
