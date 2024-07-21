#ifndef WM_H
#define WM_H

#include <X11/Xlib.h>

typedef struct {
    Display *display;
    Window root;
    int screen;
} WindowManager;

extern WindowManager wm;

int init_x();
void run_event_loop();
void cleanup_x();

#endif
