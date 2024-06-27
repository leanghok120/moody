#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"

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

  printf("Mapping window 0x%lx\n", ev.xmaprequest.window);
  XMapWindow(dpy, ev.xmaprequest.window);

  int screen_width = DisplayWidth(dpy, DefaultScreen(dpy));
  int screen_height = DisplayHeight(dpy, DefaultScreen(dpy));
  if (attr.x + attr.width > screen_width || attr.y + attr.height > screen_height) {
    XMoveWindow(dpy, ev.xmaprequest.window, 50, 50);
    printf("Window moved to (50, 50) to ensure visibility\n");
  }
}

void handle_configure_request(XEvent ev, Display *dpy) {
    unsigned long value_mask = CWX | CWY | CWWidth | CWHeight | CWBorderWidth;
    XWindowChanges changes;
    changes.x = ev.xconfigurerequest.x;
    changes.y = ev.xconfigurerequest.y;
    changes.width = ev.xconfigurerequest.width;
    changes.height = ev.xconfigurerequest.height;
    changes.border_width = ev.xconfigurerequest.border_width;

    printf("Configuring window 0x%lx to (%d, %d, %d, %d)\n",
           ev.xconfigurerequest.window,
           changes.x, changes.y,
           changes.width, changes.height);

    XConfigureWindow(dpy, ev.xconfigurerequest.window, value_mask, &changes);
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

void kill_focused_window(Display * dpy) {
  Window root, parent;
  Window * children;
  unsigned int nchildren;
  Window focused;
  int revert_to;

  // Get focused window
  XGetInputFocus(dpy, &focused, &revert_to);

  if (focused != None && focused != PointerRoot) {
    XDestroyWindow(dpy, focused);
    printf("Killed window 0x%lx\n", focused);
  } else {
    printf("No window is focused\n");
  }
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
      raise_window(dpy, ev.xmaprequest.window);
      break;
    case ConfigureRequest:
      printf("Configure Request\n");
      handle_configure_request(ev, dpy);
      break;
    case Expose:
      if (ev.xexpose.count == 0) {
        XClearWindow(dpy, ev.xexpose.window); 
      }
    case ButtonPress:
      if (ev.xbutton.subwindow != None) {
        // Resizing and Moving
        if ((ev.xbutton.state & MODIFIER) && (ev.xbutton.button == MOVE_BUTTON || ev.xbutton.button == RESIZE_BUTTON)) {
          start_drag(dpy, ev, & drag);
        }
      }
      break;
    case EnterNotify:
      printf("EnterNotify event received for window 0x%lx\n", ev.xcrossing.window);
      raise_window(dpy, ev.xcrossing.window);
      break;
    case MotionNotify:
      while (XCheckTypedEvent(dpy, MotionNotify, & ev));
      update_drag(dpy, ev, & drag);
      break;
    case ButtonRelease:
      end_drag(dpy, & drag);
      break;
    case KeyPress:
      if (ev.xkey.keycode == XKeysymToKeycode(dpy, KILL_KEY) && (ev.xkey.state & MODIFIER)) {
        printf("Modifier + q pressed, killing window\n");
        kill_focused_window(dpy);
      }
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
    ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask | EnterWindowMask);

  // Grab Moving and resizing keybinds
  XGrabButton(dpy, MOVE_BUTTON, MODIFIER, root, True, ButtonPressMask, GrabModeAsync,
    GrabModeAsync, None, None);
  XGrabButton(dpy, RESIZE_BUTTON, MODIFIER, root, True, ButtonPressMask, GrabModeAsync,
    GrabModeAsync, None, None);

  // Grab Windows keybinds
  // kill focused window
  XGrabKey(dpy, XKeysymToKeycode(dpy, KILL_KEY), MODIFIER, root, True,
           GrabModeAsync, GrabModeAsync);

  XSync(dpy, False);

  if (error_occurred) {
    errx(1, "Another window manager is running");
  }

  printf("Opened display\n");

  handle_events(dpy, root, scr);

  XCloseDisplay(dpy);
  return 0;
}
