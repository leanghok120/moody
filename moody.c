#include <X11/X.h>
#include <X11/Xlib.h>
#include <err.h>
#include <stdio.h>

void OnConfigureRequest(Display * dpy, const XConfigureRequestEvent ev) {
  XWindowChanges changes;
  changes.x = ev.x;
  changes.y = ev.y;
  changes.width = ev.width;
  changes.height = ev.height;
  changes.border_width = ev.border_width;
  changes.sibling = ev.above;
  changes.stack_mode = ev.detail;
  XConfigureWindow(dpy, ev.window, ev.value_mask, &changes);
}

void OnMapRequest(Display * dpy, const XMapRequestEvent ev) {
  XMapWindow(dpy, ev.window);
}

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
      case ConfigureRequest:
        OnConfigureRequest(dpy, ev.xconfigurerequest);
        break;
      case DestroyNotify:
        printf("DestroyNotify event: Window ID: %ld\n", ev.xdestroywindow.window);
        break;
      case ReparentNotify:
        printf("ReparentNotify event: Window ID: %ld\n", ev.xreparent.window);
        break;
      case MapRequest:
        OnMapRequest(dpy, ev.xmaprequest);
        break;
    default:
      printf("Ignore event\n");
    }
  }

  XCloseDisplay(dpy);
}
