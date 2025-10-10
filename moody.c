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
    n++;
  if (n == 0)
    return;

  int master_width = (n > 1) ? sw * 0.6 : sw;
  int stack_width = sw - master_width;

  int stack_y = 0;
  int stack_count = n - 1;
  int stack_h = (stack_count > 0) ? sh / stack_count : 0;

  int i = 0;
  for (Client *c = clients; c; c = c->next, i++) {
    if (i == 0) {
      // master window
      c->x = 0;
      c->y = 0;
      c->w = master_width;
      c->h = sh;

      XMoveResizeWindow(dpy, c->win, 0, 0, master_width, sh);
    } else {
      // slave windows
      c->x = master_width;
      c->y = stack_y;
      c->w = stack_width;
      c->h = stack_h;

      XMoveResizeWindow(dpy, c->win, master_width, stack_y, stack_width, stack_h);
      stack_y += stack_h;
    }
  }
}

void focus(Client *c) {
  if (!c) {
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

void spawn(const char *cmd, const char *args) {
  if (fork() == 0) {
    execlp(cmd, args, NULL);
    exit(1);
  }
}

void kill_client(const char *a, const char *b) {
  if (focused) {
    XSetErrorHandler(xerrordummy);
    XSetCloseDownMode(dpy, DestroyAll);
    XKillClient(dpy, focused->win);
    XSync(dpy, False);
  }
}

void handleMapReq(XMapRequestEvent *ev) {
  XWindowAttributes wa;
  XGetWindowAttributes(dpy, ev->window, &wa);
  if (wa.override_redirect) {
    return;
  }

  Client *c = malloc(sizeof(Client));
  c->win = ev->window;
  c->next = clients;
  clients = c;

  XMapWindow(dpy, c->win);
  XSetWindowBorderWidth(dpy, c->win, border_width);

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

void handleDestroyNotify(Window w) {
  Client **cc = &clients;
  while (*cc) {
    if ((*cc)->win == w) {
      Client *tmp = *cc;
      *cc = (*cc)->next;

      if (focused == tmp) {
        focused = clients;
        focus(focused);
      }

      free(tmp);
      break;
    }
    cc = &(*cc)->next;
  }

  tile();
}

void handleKeyPress(XKeyPressedEvent *ev) {
  KeySym keysym = XLookupKeysym(ev, 0);

  for (int i = 0; i < LEN(keys); i++) {
    if (keys[i].keysym == keysym && keys[i].mod & modkey) {
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
        handleDestroyNotify(ev.xunmap.window);
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
