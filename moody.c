#include <X11/X.h>
#include <X11/Xlib.h>
#include <err.h>
#include <stdio.h>

int main() {
  Display * dpy;
  Window root;
  XEvent ev;

  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    errx(1, "Failed to open display");
  } else {
    printf("Open connection to X server\n");
  }

  root = DefaultRootWindow(dpy);
  XSelectInput(dpy, root, SubstructureNotifyMask);

  for (;;) {
    XNextEvent(dpy, &ev);
    printf("Recieved an event\n");

    switch (ev.type) {
      case CreateNotify: 
        printf("CreateNotify event: Window ID: %ld\n", ev.xcreatewindow.window);
        break;
      case DestroyNotify:
        printf("DestroyNotify event: Window ID: %ld\n", ev.xdestroywindow.window);
        break;
      case ReparentNotify:
        printf("ReparentNotify event: Window ID: %ld\n", ev.xreparent.window);
        break;
    default:
      printf("Ignore event\n");
    }
  }

  XCloseDisplay(dpy);
}
