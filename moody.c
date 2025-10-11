#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
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
Atom NET_SUPPORTED, NET_WM_NAME, NET_NUMBER_OF_DESKTOPS, NET_CURRENT_DESKTOP, NET_ACTIVE_WINDOW, NET_WM_STATE, NET_WM_STATE_FULLSCREEN;
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

Atom get_atom(const char *name) {
  return XInternAtom(dpy, name, False);
}

void init_ewmh() {
  NET_SUPPORTED = get_atom("_NET_SUPPORTED");
  NET_WM_NAME = get_atom("_NET_WM_NAME");
  NET_NUMBER_OF_DESKTOPS = get_atom("_NET_NUMBER_OF_DESKTOPS");
  NET_CURRENT_DESKTOP = get_atom("_NET_CURRENT_DESKTOP");
  NET_ACTIVE_WINDOW = get_atom("_NET_ACTIVE_WINDOW");
  NET_WM_STATE = get_atom("_NET_WM_STATE");
  NET_WM_STATE_FULLSCREEN = get_atom("_NET_WM_STATE_FULLSCREEN");
}

void set_supported_ewmh() {
  Atom supported[] = {
    NET_SUPPORTED,
    NET_WM_NAME,
    NET_NUMBER_OF_DESKTOPS,
    NET_CURRENT_DESKTOP,
    NET_ACTIVE_WINDOW,
    NET_WM_STATE,
    NET_WM_STATE_FULLSCREEN,
  };

  XChangeProperty(dpy, root,
      NET_SUPPORTED, XA_ATOM,
      32, PropModeReplace,
      (unsigned char *)supported, sizeof(supported) / sizeof(supported[0])
      );
}

void set_ewmh_atoms() {
  long num_desktops = 9;

  XChangeProperty(dpy, root, NET_WM_NAME, XA_STRING, 8, PropModeReplace, (unsigned char *)"moody", 5);
  XChangeProperty(dpy, root, NET_NUMBER_OF_DESKTOPS, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&num_desktops, 1);
}

void update_cur_desktop_hints() {
  // _NET_CURRENT_DESKTOP accepts 0 to _NET_NUMBER_OF_DESKTOPS
  long cur_ws = current_ws - 1;

  XChangeProperty(dpy, root, NET_CURRENT_DESKTOP, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&cur_ws, 1);
}

void update_active_window_hint() {
  XChangeProperty(dpy, root, NET_ACTIVE_WINDOW, XA_WINDOW, 32, PropModeReplace, (unsigned char *)&focused->win, 1);
}

void update_window_state_hint(Client *c) {
  if (c->is_fullscreen) {
    XChangeProperty(dpy, c->win, NET_WM_STATE, XA_ATOM, 32, PropModeReplace, (unsigned char *)&NET_WM_STATE_FULLSCREEN, 1);
  } else {
    XDeleteProperty(dpy, c->win, NET_WM_STATE);
  }
}

void init() {
  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    printf("failed to open connection to X server\n");
    exit(1);
  }
  root = DefaultRootWindow(dpy);
  checkotherwm();
  Cursor cursor = XCreateFontCursor(dpy, XC_left_ptr);
  XDefineCursor(dpy, root, cursor);

  init_ewmh();
  set_supported_ewmh();
  set_ewmh_atoms();
  update_cur_desktop_hints();
}

void grabkeys() {
  for (int i = 0; i < LEN(keys); i++) {
    XGrabKey(dpy, XKeysymToKeycode(dpy, keys[i].keysym), keys[i].mod, root, False, GrabModeAsync, GrabModeAsync);
  }
}

Client *wintoclient(Window win) {
  for (Client *c = clients; c; c = c->next) {
    if (c->win == win) {
      return c;
    }
  }
  return NULL;
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
    if (c->workspace != current_ws || c->is_fullscreen) {
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

  update_active_window_hint();
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

  update_cur_desktop_hints();

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

void handle_net_wm_state(XClientMessageEvent *ev) {
  Client *c = wintoclient(ev->window);
  if (!c) return;

  Atom a1 = ev->data.l[1];
  Atom a2 = ev->data.l[2];
  long action = ev->data.l[0];

  if (a1 == NET_WM_STATE_FULLSCREEN || a2 == NET_WM_STATE_FULLSCREEN) {
    if (action == 1 || (action == 2 && !c->is_fullscreen)) {
      c->is_fullscreen = 1;
      XMoveResizeWindow(dpy, c->win, 0, 0,
          DisplayWidth(dpy, DefaultScreen(dpy)),
          DisplayHeight(dpy, DefaultScreen(dpy)));
      XSetWindowBorder(dpy, c->win, 0);
      XChangeProperty(dpy, c->win, NET_WM_STATE, XA_ATOM, 32, PropModeReplace,
          (unsigned char *)&NET_WM_STATE_FULLSCREEN, 1);
    } else if (action == 0 || (action == 2 && c->is_fullscreen)) {
      c->is_fullscreen = 0;
      XDeleteProperty(dpy, c->win, NET_WM_STATE);
      XSetWindowBorder(dpy, c->win, border_width);
      tile();
    }
  }
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
  c->is_fullscreen = 0;
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
      case ClientMessage:
        handle_net_wm_state(&ev.xclient);
        break;
    }
  }
}

int main() {
  init();

  grabkeys();

  run();

  XCloseDisplay(dpy);

  return 0;
}
