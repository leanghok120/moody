#pragma once
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

typedef struct Client {
  Window win;
  int x, y, w, h;
  struct Client *next;
} Client;

typedef struct {
  unsigned int mod;
  KeySym keysym;
  void (*func)(const char *cmd, const char *args);
  const char *cmd;
  const char *args;
} Key;

extern Client *clients;
extern Client *focused;
extern Display *dpy;
extern Window root;

void spawn(const char *cmd, const char *args);
void kill_client(const char *a, const char *b);
