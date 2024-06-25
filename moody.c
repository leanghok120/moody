#include <X11/X.h>
#include <X11/Xlib.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#define MODIFIER Mod4Mask // Super key
#define MAX(a, b) ((a) > (b) ? (a) : (b))

void handle_map_request(XEvent ev, Display * dpy, int scr) {
  XWindowAttributes attr;
  XGetWindowAttributes(dpy, ev.xmaprequest.window, & attr);

  if (attr.override_redirect) {
    printf("Override redirect, skipping window\n");
    return;
  }

  // Resize window to fullscreen
  // unsigned long value_mask = CWX | CWY | CWWidth | CWHeight;
  // XWindowChanges changes;
  // changes.x = 0;
  // changes.y = 0;
  // changes.width = DisplayWidth(dpy, scr);
  // changes.height = DisplayHeight(dpy, scr);

  // printf("Configuring window to fullscreen\n");
  // XConfigureWindow(dpy, ev.xmaprequest.window, value_mask, &changes);

  printf("Mapping window\n");
  XMapWindow(dpy, ev.xmaprequest.window);
}

void handle_configure_request(XEvent ev, Display * dpy) {
  unsigned long value_mask = CWX | CWY | CWWidth | CWHeight | CWBorderWidth | CWSibling | CWStackMode;
  XWindowChanges changes;
  changes.x = ev.xconfigurerequest.x;
  changes.y = ev.xconfigurerequest.y;
  changes.width = ev.xconfigurerequest.width;
  changes.height = ev.xconfigurerequest.height;
  changes.border_width = ev.xconfigurerequest.border_width;
  changes.sibling = ev.xconfigurerequest.above;
  changes.stack_mode = ev.xconfigurerequest.detail;

  printf("Configuring window\n");
  XConfigureWindow(dpy, ev.xconfigurerequest.window, value_mask, &changes);
}

void run(Display * dpy, Window root, XEvent ev, int scr, XWindowAttributes attr, XButtonEvent start) {
  printf("Event loop started\n");
  for (;;) {
    XNextEvent(dpy, & ev);

    switch (ev.type) {
      case MapRequest:
        printf("Map Request\n");
        handle_map_request(ev, dpy, scr);
        break;
      case ConfigureRequest:
        printf("Configure Request\n");
        handle_configure_request(ev, dpy);
        break;
      case ButtonPress:
        if (ev.xbutton.subwindow != None) {
          printf("Move windows") ;
          XGrabPointer(dpy, ev.xbutton.subwindow, True,
                  PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
                  GrabModeAsync, None, None, CurrentTime);
          XGetWindowAttributes(dpy, ev.xbutton.subwindow, &attr);
          start = ev.xbutton;
        } 
        break;
      case MotionNotify:
        int xdiff, ydiff;
        while(XCheckTypedEvent(dpy, MotionNotify, &ev));
        xdiff = ev.xbutton.x_root - start.x_root;
        ydiff = ev.xbutton.y_root - start.y_root;
        XMoveResizeWindow(dpy, ev.xmotion.window,
            attr.x + (start.button==1 ? xdiff : 0),
            attr.y + (start.button==1 ? ydiff : 0),
            MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
            MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
        break;
      case ButtonRelease:
        XUngrabPointer(dpy, CurrentTime);
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
  XWindowAttributes attr;
  XButtonEvent start;

  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    errx(1, "Couldn't open display");
  }

  scr = DefaultScreen(dpy);
  root = RootWindow(dpy, scr);
  XSetErrorHandler(handle_x_error);
  error_occurred = 0;

  XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask |
               ButtonPressMask | ButtonReleaseMask | PointerMotionMask);

  XGrabButton(dpy, Button1, MODIFIER, root, True, ButtonPressMask, GrabModeAsync,
          GrabModeAsync, None, None);

  XSync(dpy, False);

  // Check if there is an error with XSelectInput (if there is then another wm is running)
  if (error_occurred) {
    errx(1, "Another window manager is running");
  }

  printf("Opened display\n");

  run(dpy, root, ev, scr, attr, start);

  XCloseDisplay(dpy);
  return 0;
}
