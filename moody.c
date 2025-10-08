#include <X11/X.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>

Display *dpy;
Window root;

int xerrorstart(Display *dpy, XErrorEvent *ee) {
  printf("another wm is running\n");
  exit(1);
}

void checkotherwm() {
  XErrorHandler error = XSetErrorHandler(xerrorstart);
  XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask | SubstructureNotifyMask);
  XSync(dpy, False);
}

void run() {
  XEvent ev;
  while (1) {
    XNextEvent(dpy, &ev);
    switch (ev.type) {
      case CreateNotify:
        break;
      case DestroyNotify:
        break;
      case ReparentNotify:
        break;
    }
  }
}

int main() {
  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    printf("failed to open connection to X server\n");
    exit(1);
  }
  root = DefaultRootWindow(dpy);
  checkotherwm();

  run();

  XCloseDisplay(dpy);

  return 0;
}
