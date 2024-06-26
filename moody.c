#include <X11/X.h>
#include <X11/Xlib.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#define MODIFIER Mod1Mask // Alt key
#define MAX(a, b)((a) > (b) ? (a) : (b))

typedef struct {
  Window window;
  int start_x, start_y;
  int x, y;
  int width, height;
  int is_resizing;
}
DragState;

void handle_map_request(XEvent ev, Display * dpy) {
  XWindowAttributes attr;
  XGetWindowAttributes(dpy, ev.xmaprequest.window, & attr);

  if (attr.override_redirect) {
    printf("Override redirect, skipping window\n");
    return;
  }

  XRaiseWindow(dpy, ev.xmaprequest.window);
  printf("Mapping and raising window 0x%lx\n", ev.xmaprequest.window);
  XMapWindow(dpy, ev.xmaprequest.window);
  XSetInputFocus(dpy, ev.xmaprequest.window, RevertToPointerRoot, CurrentTime);
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

  printf("Configuring window 0x%lx\n", ev.xconfigurerequest.window);
  XConfigureWindow(dpy, ev.xconfigurerequest.window, value_mask, & changes);
}

void start_drag(Display * dpy, XEvent ev, DragState * drag) {
  if (ev.xbutton.subwindow != None) {
    drag -> window = ev.xbutton.subwindow;
    drag -> start_x = ev.xbutton.x_root;
    drag -> start_y = ev.xbutton.y_root;

    XWindowAttributes attr;
    XGetWindowAttributes(dpy, drag -> window, & attr);
    drag -> x = attr.x;
    drag -> y = attr.y;
    drag -> width = attr.width;
    drag -> height = attr.height;
    drag -> is_resizing = (ev.xbutton.button == Button3);

    XGrabPointer(dpy, drag -> window, True,
      PointerMotionMask | ButtonReleaseMask, GrabModeAsync,
      GrabModeAsync, None, None, CurrentTime);

    printf("Starting %s on window 0x%lx\n", drag -> is_resizing ? "resize" : "move", drag -> window);
  }
}

void update_drag(Display * dpy, XEvent ev, DragState * drag) {
  if (drag -> window != None) {
    int xdiff = ev.xmotion.x_root - drag -> start_x;
    int ydiff = ev.xmotion.y_root - drag -> start_y;

    if (drag -> is_resizing) {
      XMoveResizeWindow(dpy, drag -> window,
        drag -> x, drag -> y,
        MAX(1, drag -> width + xdiff),
        MAX(1, drag -> height + ydiff));
    } else {
      XMoveWindow(dpy, drag -> window,
        drag -> x + xdiff,
        drag -> y + ydiff);
    }
  }
}

void end_drag(Display * dpy, DragState * drag) {
  if (drag -> window != None) {
    XUngrabPointer(dpy, CurrentTime);
    drag -> window = None;
    printf("Drag ended\n");
  }
}

void raise_window(Display * dpy, Window window) {
  XRaiseWindow(dpy, window);
  XSetInputFocus(dpy, window, RevertToPointerRoot, CurrentTime);
  printf("Window 0x%lx raised and focused\n", window);
}

void handle_events(Display * dpy, Window root, int scr) {
  DragState drag = {
    0
  };
  XEvent ev;
  for (;;) {
    XNextEvent(dpy, & ev);

    switch (ev.type) {
    case MapRequest:
      printf("Map Request\n");
      handle_map_request(ev, dpy);
      break;
    case ConfigureRequest:
      printf("Configure Request\n");
      handle_configure_request(ev, dpy);
      break;
    case ButtonPress:
      if (ev.xbutton.subwindow != None) {
        // Raise window on left click (Button1) without modifier
        if (ev.xbutton.button == Button1 && (ev.xbutton.state & MODIFIER) == 0) {
          raise_window(dpy, ev.xbutton.subwindow);
        }
        // Start dragging/resizing if left-click (Button1) or right-click (Button3) with modifier
        if ((ev.xbutton.state & MODIFIER) && (ev.xbutton.button == Button1 || ev.xbutton.button == Button3)) {
          start_drag(dpy, ev, & drag);
        }
      }
      break;
    case MotionNotify:
      while (XCheckTypedEvent(dpy, MotionNotify, & ev));
      update_drag(dpy, ev, & drag);
      break;
    case ButtonRelease:
      end_drag(dpy, & drag);
      break;
    default:
      printf("Other event type: %d\n", ev.type);
      break;
    }
  }
}

static int error_occurred = 0;

int handle_x_error(Display * dpy, XErrorEvent * error_event) {
  if (error_event -> error_code == BadAccess) {
    error_occurred = 1;
  }
  return 0;
}

int main() {
  Display * dpy;
  int scr;
  Window root;

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

  // Grab left-click (Button1) and right-click (Button3) with modifier for moving/resizing
  XGrabButton(dpy, Button1, MODIFIER, root, True, ButtonPressMask, GrabModeAsync,
    GrabModeAsync, None, None);
  XGrabButton(dpy, Button3, MODIFIER, root, True, ButtonPressMask, GrabModeAsync,
    GrabModeAsync, None, None);

  // Grab left-click (Button1) without any modifier for raising the window
  XGrabButton(dpy, Button1, AnyModifier, root, True, ButtonPressMask, GrabModeAsync,
    GrabModeAsync, None, None);

  XSync(dpy, False);

  if (error_occurred) {
    errx(1, "Another window manager is running");
  }

  printf("Opened display\n");

  handle_events(dpy, root, scr);

  XCloseDisplay(dpy);
  return 0;
}
