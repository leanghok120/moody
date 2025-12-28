#include <X11/X.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "config.h"

Client *clients = NULL;
Client *focused = NULL;

Display *dpy;
Window root;
int current_ws = 1;

// functions for error handling
int xerrordummy(Display *dpy, XErrorEvent *ee) {
  return 0;
}

int xerrorstart(Display *dpy, XErrorEvent *ee) {
  fprintf(stderr, "another wm is running\n");
  exit(1);
  return -1;
}

// taken from sowm: https://github.com/dylanaraps/sowm
void spawn(const Arg *arg) {
  if (fork()) return;
  if (dpy) close(ConnectionNumber(dpy));

  setsid();
  execvp((char*)arg->cmd[0], (char**)arg->cmd);
}

void kill(const Arg *arg) {
  if (focused == NULL)
    return;

  XSetCloseDownMode(dpy, DestroyAll);
  XSetErrorHandler(xerrordummy);
  XKillClient(dpy, focused->win);
}

void focus(Client *c) {
  if (!c) {
    return;
  }

  focused = c;
  XRaiseWindow(dpy, focused->win);
  XSetInputFocus(dpy, focused->win, RevertToPointerRoot, CurrentTime);
}

void focusnext(const Arg *arg) {
  Client *c = NULL;

  if (!clients || !focused)
    return;

  // find next window in the current workspace
  for (Client *i = focused->next; i; i = i->next) {
    if (i->workspace == current_ws) {
      c = i;
      break;
    }
  }

  // find first window in the current workspace
  if (!c) {
    for (Client *i = clients; i && i != focused; i = i->next) {
      if (i->workspace == current_ws) {
        c = i;
        break;
      }
    }
  }

  if (!c)
    return;

  focus(c);
}

void switchws(const Arg *arg) {
  int next = arg->i;
  if (next == current_ws) return;

  current_ws = next;

  Client *c;
  for (c = clients; c; c = c->next) {
    if (c->workspace == current_ws) {
      XMapWindow(dpy, c->win);
      focus(c);
    } else {
      XUnmapWindow(dpy, c->win);
    }
  }
}

void sendws(const Arg *arg) {
  int next = arg->i;
  if (next == current_ws) return;

  focused->workspace = next;
  XUnmapWindow(dpy, focused->win);
  focusnext(NULL);
}

void tile() {
  int n = 0;
  for (Client *c = clients; c; c = c->next) {
    if (c->workspace == current_ws) {
      n++;
    }
  }

  if (n == 0) return;

  int i = 0;
  int masterw = WIDTH / 1.6;
  int stacky = 0;
  int stackh = (n > 1) ? HEIGHT / (n-1) : 0;

  for (Client *c = clients; c; c = c->next) {
    if (n == 1) {
      XMoveResizeWindow(dpy, c->win, 0, 0, WIDTH, HEIGHT);
      break;
    }

    if (c->workspace == current_ws) {
      if (i == 0) {
        // master
        XMoveResizeWindow(dpy, c->win, 0, 0, masterw, HEIGHT);
      } else {
        // slave
        XMoveResizeWindow(dpy, c->win, masterw, stacky, masterw, stackh);
        stacky += stackh;
      }
      i++;
    }
  }
}

void init() {
  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    perror("XOpenDisplay");
    exit(1);
  }
  root = XDefaultRootWindow(dpy);
  int scr = XDefaultScreen(dpy);

  // check other wm
  XSetErrorHandler(xerrorstart);
  XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask);
  XSync(dpy, False);
  XSetErrorHandler(xerrorstart);
  XSync(dpy, False);
}

void cleanup() {
  Client *c = clients;
  while (c) {
    Client *next = c->next;
    free(c);
    c = next;
  }

  XCloseDisplay(dpy);
}

void grabkeys() {
  for (int i = 0; i < LEN(keys); i++) {
    XGrabKey(dpy, XKeysymToKeycode(dpy, keys[i].keysym), keys[i].mod, root, True, GrabModeAsync, GrabModeAsync);
  }
}

void mapreq(XEvent *ev) {
  XMapRequestEvent *e = &ev->xmaprequest;
  XWindowAttributes wa;

  XGetWindowAttributes(dpy, e->window, &wa);
  if (wa.override_redirect) {
    return;
  }

  // add to clients linked list
  Client *c = malloc(sizeof(Client));
  c->win = e->window;
  c->workspace = current_ws;
  c->next = clients;
  clients = c;
  focused = c;

  tile();
  XMapWindow(dpy, e->window);
  XRaiseWindow(dpy, e->window);
  XSetInputFocus(dpy, e->window, RevertToPointerRoot, CurrentTime);

  XSync(dpy, False);
}

void destroynoti(XEvent *ev) {
  XDestroyWindowEvent *e = &ev->xdestroywindow;

  // remove from clients linked list and change focus
  Client *cur = clients;
  Client *prev = NULL;
  while (cur) {
    if (cur->win == e->window) {
      if (prev) {
        prev->next = cur->next;
      } else {
        clients = cur->next;
      }

      // change focus
      if (focused == cur) {
        focusnext(NULL);
      }

      free(cur);
      return;
    }
    prev = cur;
    cur = cur->next;
  }
}

void configreq(XEvent *ev) {
  XConfigureRequestEvent *e = &ev->xconfigurerequest;
  XWindowChanges wc;

  wc.x = e->x;
  wc.y = e->y;
  wc.width = e->width;
  wc.height = e->height;
  XConfigureWindow(dpy, e->window, e->value_mask, &wc);
}

void keypressreq(XEvent *ev) {
  XKeyPressedEvent *e = &ev->xkey;
  KeySym keysym = XLookupKeysym(e, 0);

  for (int i = 0; i < LEN(keys); i++) {
    if (keys[i].keysym == keysym && keys[i].mod == CLEANMASK(e->state)) {
      keys[i].func(&keys[i].arg);
    }
  }
}

void run() {
  XEvent ev;
  while (1) {
    XNextEvent(dpy, &ev);
    switch (ev.type) {
      case MapRequest:
        mapreq(&ev);
        break;
      case UnmapNotify:
        tile();
        break;
      case DestroyNotify:
        destroynoti(&ev);
        tile();
        break;
      case ConfigureRequest:
        configreq(&ev);
        break;
      case KeyPress:
        keypressreq(&ev);
        break;
    }
  }
}

int main() {
  init();
  grabkeys();
  run();
  cleanup();
  return 0;
}
