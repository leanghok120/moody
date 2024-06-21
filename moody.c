#include <X11/X.h>
#include <X11/Xlib.h>
#include <err.h>
#include <stdio.h>

int main() {
  Display * dpy;
  Window root;

  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    errx(1, "Failed to open display");
  } else {
    printf("Open connection to X server\n");
  }

  root = DefaultRootWindow(dpy);

  XCloseDisplay(dpy);
}
