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

void handleMapReq(XMapRequestEvent *ev) {
  int sw = DisplayWidth(dpy, DefaultScreen(dpy));
  int sh = DisplayHeight(dpy, DefaultScreen(dpy));

  Client *c = malloc(sizeof(Client));
  c->win = ev->window;
  c->x = 0;
  c->y = 0;
  c->w = sw;
  c->h = sh;
  c->next = clients;
  clients = c;

  XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);

  XMapWindow(dpy, c->win);
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
