#pragma once
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

// macros
#define LEN(a) sizeof(a)/sizeof(a[0])
#define WIDTH DisplayWidth(dpy, DefaultScreen(dpy))
#define HEIGHT DisplayHeight(dpy, DefaultScreen(dpy))
// taken from dwm
#define CLEANMASK(mask) (mask & ~(LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

// linked list to keep track of windows
typedef struct Client {
  Window win;
  int workspace;
  struct Client *next;
} Client;

typedef union {
	int i;
	const char **cmd;
} Arg;

typedef struct {
  unsigned int mod;
  KeySym keysym;
  void (*func)(const Arg *arg);
  const Arg arg;
} Key;

// globals
extern Client *clients;
extern Client *focused;
extern int current_ws;

extern Display *dpy;
extern Window root;

// functions prototype
int xerrordummy(Display *dpy, XErrorEvent *ee);
int xerrorstart(Display *dpy, XErrorEvent *ee);
void spawn(const Arg *arg);
void kill(const Arg *arg);
void focus(Client *c);
void focusnext(const Arg *arg);
void switchws(const Arg *arg);
void sendws(const Arg *arg);
void tile();
void init();
void cleanup();
void grabkeys();
void mapreq(XEvent *ev);
void destroynoti(XEvent *ev);
void configreq(XEvent *ev);
void keypressreq(XEvent *ev);
void run();
