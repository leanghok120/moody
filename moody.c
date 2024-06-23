#include <X11/X.h>
#include <X11/Xlib.h>
#include <err.h>
#include <stdlib.h>

static const char TERM[] = "xterm";

static Display * dpy;
static int scr;
static Window root;
static XEvent ev;

int main() {
  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    errx(1, "Couldn't open display");
  }
  scr = DefaultScreen(dpy);
  root = RootWindow(dpy, scr);

  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("t")), Mod1Mask, root,
          True, GrabModeAsync, GrabModeAsync);

  for (;;) {
    XNextEvent(dpy, &ev); 
    switch (ev.type) {
      case KeyPress:
        system(TERM);
    }
  }

  XCloseDisplay(dpy);
}
