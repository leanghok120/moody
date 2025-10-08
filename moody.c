#include <X11/X.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>

void run() {
  while (1) {
    // implement some stuff
  }
}

int xerrorstart(Display *dpy, XErrorEvent *ee) {
  printf("another wm is running\n");
  exit(1);
}

int main() {
  Display *dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    printf("failed to open connection to X server");
    exit(1);
  }
  Window root = DefaultRootWindow(dpy);
	XErrorHandler error = XSetErrorHandler(xerrorstart);
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask | SubstructureNotifyMask);
	XSync(dpy, False);

  run();

  XCloseDisplay(dpy);

  return 0;
}
