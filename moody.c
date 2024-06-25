#include <X11/X.h>
#include <X11/Xlib.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

// Function to handle MapRequest events
void handle_map_request(XEvent ev, Display *dpy, int scr) {
    XWindowAttributes attr;
    XGetWindowAttributes(dpy, ev.xmaprequest.window, &attr);

    if (attr.override_redirect) {
        printf("Override redirect, skipping window\n");
        return;
    }

    // Resize window to fullscreen
    unsigned int value_mask = CWX | CWY | CWWidth | CWHeight;
    XWindowChanges changes;
    changes.x = 0;
    changes.y = 0;
    changes.width = DisplayWidth(dpy, scr);
    changes.height = DisplayHeight(dpy, scr);

    printf("Configuring window to fullscreen\n");
    XConfigureWindow(dpy, ev.xmaprequest.window, value_mask, &changes);

    printf("Mapping window\n");
    XMapWindow(dpy, ev.xmaprequest.window);
}

int main() {
    Display *dpy;
    int scr;
    Window root;
    XEvent ev;

    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        errx(1, "Couldn't open display");
    }

    scr = DefaultScreen(dpy);
    root = RootWindow(dpy, scr);

    // Select events to listen for on the root window
    XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask);

    printf("Event loop started\n");

    for (;;) {
        XNextEvent(dpy, &ev);

        switch (ev.type) {
            case MapRequest:
                printf("Map Request\n");
                handle_map_request(ev, dpy, scr);
                break;

            case KeyPress:
                printf("Key Pressed: %d\n", ev.xkey.keycode);
                break;

            default:
                printf("Other event type: %d\n", ev.type);
                break;
        }
    }

    XCloseDisplay(dpy);
    return 0;
}
