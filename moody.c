#include <X11/X.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Client {
  Window win;
  int x, y, w, h;
  struct Client *next;
} Client;

Client *clients = NULL;

Display *dpy;
Window root;

int xerrorstart(Display *dpy, XErrorEvent *ee) {
  printf("another wm is running\n");
  exit(1);
}

void checkotherwm() {
  XErrorHandler error = XSetErrorHandler(xerrorstart);
  XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask | SubstructureNotifyMask);
  XSync(dpy, False);
  XSetErrorHandler(error);
}

void tile() {
  int sw = DisplayWidth(dpy, DefaultScreen(dpy));
  int sh = DisplayHeight(dpy, DefaultScreen(dpy));

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

void handleMapReq(XMapRequestEvent *ev) {
  Client *c = malloc(sizeof(Client));
  c->win = ev->window;
  c->next = clients;
  clients = c;

  XMapWindow(dpy, c->win);

  tile();
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
      free(tmp);
      break;
    }
    cc = &(*cc)->next;
  }

  tile();
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
      case DestroyNotify:
        handleDestroyNotify(ev.xdestroywindow.window);
        break;
      case ReparentNotify:
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

  run();

  XCloseDisplay(dpy);

  return 0;
}
