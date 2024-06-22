#include <X11/X.h>
#include <X11/Xlib.h>
#include <err.h>
#include <stdio.h>

void Frame(Display * dpy, Window root, Window w) {
  // Visual stuff for frame
  const unsigned int BORDER_WITDH = 3;
  const unsigned long BORDER_COLOR = 0xffffff;
  const unsigned long BG_COLOR = 0x000000;

  // Get window attributes
  XWindowAttributes x_win_attrs;
  XGetWindowAttributes(dpy, w, &x_win_attrs);

  // Create frame
  const Window frame = XCreateSimpleWindow(dpy, root, x_win_attrs.x, x_win_attrs.y, x_win_attrs.width, x_win_attrs.height, BORDER_WITDH, BORDER_COLOR, BG_COLOR);

  // Select events on frame
  XSelectInput(dpy, frame, SubstructureRedirectMask | SubstructureNotifyMask);
  XAddToSaveSet(dpy, w);

  // Reparent client windows
  XReparentWindow(dpy, w, frame, 0, 0);

  XMapWindow(dpy, frame);

  printf("Framed window");
}

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
  printf("Resize window");
}

void OnMapRequest(Display * dpy, Window root, const XMapRequestEvent ev) {
  Frame(dpy, root, ev.window);
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
        OnMapRequest(dpy, root, ev.xmaprequest);
        break;
    default:
      printf("Ignore event: %d\n", ev.type);
    }
  }

  XCloseDisplay(dpy);
}
