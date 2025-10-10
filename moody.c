#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "moody.h"
#include "config.h"

#define LEN(x) sizeof(x)/sizeof(x[0])

Client *clients = NULL;
Client *focused = NULL;
Display *dpy;
Window root;
int current_ws = 1;

int xerrorstart(Display *dpy, XErrorEvent *ee) {
  printf("another wm is running\n");
  exit(1);
}

int xerrordummy(Display *dpy, XErrorEvent *ee) {
  return 0;
}

void checkotherwm() {
  XErrorHandler error = XSetErrorHandler(xerrorstart);
  XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask);
  XSync(dpy, False);
  XSetErrorHandler(error);
}

void grabkeys() {
  for (int i = 0; i < LEN(keys); i++) {
    XGrabKey(dpy, XKeysymToKeycode(dpy, keys[i].keysym), keys[i].mod, root, False, GrabModeAsync, GrabModeAsync);
  }
}

void tile() {
  int sw = DisplayWidth(dpy, DefaultScreen(dpy)) - 5;
  int sh = DisplayHeight(dpy, DefaultScreen(dpy)) - 4;

  int n = 0;
  for (Client *c = clients; c; c = c->next)
    if (c->workspace == current_ws) {
      n++;
    }

  if (n == 0)
    return;

  int master_width = (n > 1) ? sw * 0.6 : sw;
  int stack_width = sw - master_width;

  int stack_y = 0;
  int stack_count = n - 1;
  int stack_h = (stack_count > 0) ? sh / stack_count : 0;

  int i = 0;
  for (Client *c = clients; c; c = c->next) {
    if (c->workspace != current_ws) {
      continue;
    }

    if (i == 0) {
      // master window
      c->x = 0;
      c->y = 0;
      c->w = master_width;
      c->h = sh;
    } else {
      // slave windows
      c->x = master_width;
      c->y = stack_y;
      c->w = stack_width;
      c->h = stack_h;
      stack_y += stack_h;
    }
    XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
    i++;
  }
}

void focus(Client *c) {
  if (!c) {
    return;
  }

  if (c->workspace != current_ws) {
    return;
  }

  if (focused && focused != c) {
    XSetWindowBorder(dpy, focused->win, border_color);
  }

  focused = c;
  XRaiseWindow(dpy, c->win);
  XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
  XSetWindowBorder(dpy, c->win, border_color_active);
}

void focus_next(const char *a, const char *b)  {
  if (!focused) {
    return;
  }

  Client *c = focused->next;

  while (c) {
    if (c->workspace == current_ws) {
      focus(focused->next);
      return;
    }
    c = c->next;
  }

  // loop back to the head
  for (c = clients; c; c = c->next) {
    if (c->workspace == current_ws) {
      focus(c);
      return;
    }
  }
}

void spawn(const char *cmd, const char *args) {
  if (fork() == 0) {
    execlp(cmd, args, NULL);
    exit(1);
  }
}

void kill_client(const char *a, const char *b) {
  if (focused && focused->workspace == current_ws) {
    XSetErrorHandler(xerrordummy);
    XSetCloseDownMode(dpy, DestroyAll);
    XKillClient(dpy, focused->win);
    XSync(dpy, False);
  }
}

void switch_workspace(const char *workspace, const char *b) {
  int ws = atoi(workspace);
  if (ws < 1 || ws > 9) {
    return;
  }

  if (ws == current_ws)
    return;

  for (Client *c = clients; c; c = c->next) {
    if (c->workspace == current_ws) {
      XUnmapWindow(dpy, c->win);
    }
  }

  current_ws = ws;

  Client *first = NULL;
  for (Client *c = clients; c; c = c->next) {
    if (c->workspace == current_ws) {
      XMapWindow(dpy, c->win);
      if (!first) first = c;
    }
  }

  XSync(dpy, False);

  tile();

  if (first) {
    focus(first);
  } else {
    if (focused) {
      XSetWindowBorder(dpy, focused->win, border_color);
      focused = NULL;
    }
    XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
  }
}

void move_to_workspace(const char *workspace, const char *b) {
  if (!focused)
    return;

  int ws = atoi(workspace);
  if (ws < 1 || ws > 9)
    return;

  int old_ws = focused->workspace;
  focused->workspace = ws;

  if (ws != current_ws) {
    XUnmapWindow(dpy, focused->win);
    focused = NULL;
    for (Client *c = clients; c; c = c->next) {
      if (c->workspace == old_ws) {
        focus(c);
        break;
      }
    }
  } else {
    XMapWindow(dpy, focused->win);
    XSync(dpy, False);
  }

  tile();
}

void handleMapReq(XMapRequestEvent *ev) {
  XWindowAttributes wa;
  XGetWindowAttributes(dpy, ev->window, &wa);
  if (wa.override_redirect) {
    return;
  }

  Client *c = malloc(sizeof(Client));
  if (!c) return;
  c->win = ev->window;
  c->next = clients;
  clients = c;
  c->workspace = current_ws;

  XMapWindow(dpy, c->win);
  XSetWindowBorderWidth(dpy, c->win, border_width);

  XSync(dpy, False);

  tile();
  focus(c);
}

void handleConfigureReq(XConfigureRequestEvent *ev) {
  XWindowChanges changes;
  changes.x = ev->x;
  changes.y = ev->y;
  changes.width = ev->width;
  changes.height = ev->height;
  changes.border_width = ev->border_width;
  changes.sibling = ev->above;
  changes.stack_mode = ev->detail;
  XConfigureWindow(dpy, ev->window, ev->value_mask, &changes);
}

void handleUnmapReq(Window w) {
  tile();
}

void handleDestroyNotify(Window w) {
  Client **cc = &clients;
  while (*cc) {
    if ((*cc)->win == w) {
      Client *tmp = *cc;
      *cc = (*cc)->next;

      if (focused == tmp) {
        focused = NULL;
      }

      free(tmp);
      break;
    }
    cc = &(*cc)->next;
  }

  Client *first = NULL;
  for (Client *c = clients; c; c = c->next) {
    if (c->workspace == current_ws) {
      first = c;
      break;
    }
  }

  if (first)
    focus(first);
  else {
    if (focused) {
      XSetWindowBorder(dpy, focused->win, border_color);
      focused = NULL;
    }
    XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
  }

  tile();
}

void handleKeyPress(XKeyPressedEvent *ev) {
  KeySym keysym = XLookupKeysym(ev, 0);

  for (int i = 0; i < LEN(keys); i++) {
    if (keys[i].keysym == keysym && keys[i].mod == (ev->state & (Mod1Mask | ShiftMask | ControlMask))) {
      keys[i].func(keys[i].cmd, keys[i].args);
    }
  }
}

void run() {
  XEvent ev;
  while (1) {
    XNextEvent(dpy, &ev);
    switch (ev.type) {
      case CreateNotify:
        break;
      case MapRequest:
        handleMapReq(&ev.xmaprequest);
        break;
      case ConfigureRequest:
        handleConfigureReq(&ev.xconfigurerequest);
        break;
      case UnmapNotify:
        handleUnmapReq(ev.xunmap.window);
        break;
      case DestroyNotify:
        handleDestroyNotify(ev.xdestroywindow.window);
        break;
      case KeyPress:
        handleKeyPress(&ev.xkey);
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

  grabkeys();

  run();

  XCloseDisplay(dpy);

  return 0;
}
