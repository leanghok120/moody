#include <X11/X.h>
#include <X11/Xlib.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

void handle_map_request(XEvent ev, Display * dpy, int scr) {
  XWindowAttributes attr;
  XGetWindowAttributes(dpy, ev.xmaprequest.window, & attr);

  if (attr.override_redirect) {
    printf("Override redirect, skipping window\n");
    return;
  }

  // Resize window to fullscreen
  unsigned int value_mask = CWX | CWY | CWWidth | CWHeight;
  XWindowChanges changes;
  changes.x = 0;
  changes.y = 0;
  changes.width = DisplayWidth(dpy, scr);
  changes.height = DisplayHeight(dpy, scr);

  printf("Configuring window to fullscreen\n");
  XConfigureWindow(dpy, ev.xmaprequest.window, value_mask, & changes);

  printf("Mapping window\n");
  XMapWindow(dpy, ev.xmaprequest.window);
}

void run(Display * dpy, XEvent ev, int scr) {
  printf("Event loop started\n");
  for (;;) {
    XNextEvent(dpy, & ev);

    switch (ev.type) {
    case MapRequest:
      printf("Map Request\n");
      handle_map_request(ev, dpy, scr);
      break;

    case KeyPress:
      printf("Key Pressed: %d\n", ev.xkey.keycode);
      break;

    default:
      printf("Other event type: %d\n", ev.type);
      break;
    }
  }
}

static int error_occurred = 0;
int handle_x_error(Display * dpy, XErrorEvent * error_event) {
  // Check if the error is a BadAccess error, which indicates another WM is running
  if (error_event -> error_code == BadAccess) {
    error_occurred = 1;
  }
  return 0;
}

int main() {
  Display * dpy;
  int scr;
  Window root;
  XEvent ev;

  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    errx(1, "Couldn't open display");
  }

  scr = DefaultScreen(dpy);
  root = RootWindow(dpy, scr);

  XSetErrorHandler(handle_x_error);
  error_occurred = 0;

  XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask);

  XSync(dpy, False);

  // Check if there is an error with XSelectInput (if there is then another wm is running)
  if (error_occurred) {
    errx(1, "Another window manager is running");
  }

  printf("Opened display\n");

  run(dpy, ev, scr);

  XCloseDisplay(dpy);
  return 0;
}
