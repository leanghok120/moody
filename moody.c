#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

#define MAX(a, b)((a) > (b) ? (a) : (b))

typedef struct {
  Window window;
  int start_x, start_y;
  int x, y;
  int width, height;
  int is_resizing;
}
DragState;

typedef struct {
  Window window;
  int x, y;
  int width, height;
}
WindowInfo;

typedef struct {
  WindowInfo windows[MAX_WINDOWS];
  int count;
  Window master; // Store the master window
}
TilingLayout;

TilingLayout layout;

typedef struct {
  TilingLayout layouts[MAX_WORKSPACES];
  int current_workspace;
}
WorkspaceManager;

WorkspaceManager workspace_manager;

// Tiling functions
void init_layout() {
  layout.count = 0;
  layout.master = None;
}

void add_window_to_layout(Window window, TilingLayout * layout) {
  if (layout -> count < MAX_WINDOWS) {
    for (int i = 0; i < layout -> count; i++) {
      if (layout -> windows[i].window == window) {
        return; // Window already exists in layout
      }
    }
    layout -> windows[layout -> count].window = window;
    layout -> count++;
    if (layout -> master == None) {
      layout -> master = window;
    }
    printf("Window 0x%lx added. Total windows: %d\n", window, layout -> count);
  } else {
    fprintf(stderr, "Window limit exceeded\n");
  }
}

void remove_window_from_layout(Window window, TilingLayout * layout) {
  int found = 0;
  for (int i = 0; i < layout -> count; i++) {
    if (layout -> windows[i].window == window) {
      found = 1;
    }
    if (found && i < layout -> count - 1) {
      layout -> windows[i] = layout -> windows[i + 1];
    }
  }
  if (found) {
    layout -> count--;
    if (layout -> master == window) {
      layout -> master = (layout -> count > 0) ? layout -> windows[0].window : None;
    }
    printf("Window 0x%lx removed. Total windows: %d\n", window, layout -> count);
  }
}

void arrange_window(Display * dpy, int screen_width, int screen_height) {
  TilingLayout * current_layout = & workspace_manager.layouts[workspace_manager.current_workspace];
  if (current_layout -> count == 0) return;

  if (current_layout -> count == 1) {
    current_layout -> windows[0].x = 0;
    current_layout -> windows[0].y = 0;
    current_layout -> windows[0].width = screen_width;
    current_layout -> windows[0].height = screen_height;
  } else {
    int master_width = screen_width / 2;
    int stack_width = screen_width - master_width;
    int stack_height = screen_height / (current_layout -> count - 1);

    for (int i = 0; i < current_layout -> count; i++) {
      if (current_layout -> windows[i].window == current_layout -> master) {
        current_layout -> windows[i].x = 0;
        current_layout -> windows[i].y = 0;
        current_layout -> windows[i].width = master_width;
        current_layout -> windows[i].height = screen_height;
      } else {
        current_layout -> windows[i].x = master_width;
        current_layout -> windows[i].y = stack_height * (i - 1);
        current_layout -> windows[i].width = stack_width;
        current_layout -> windows[i].height = stack_height;
      }
    }
  }
}

void apply_layout(Display * dpy) {
  TilingLayout * current_layout = & workspace_manager.layouts[workspace_manager.current_workspace];
  for (int i = 0; i < current_layout -> count; i++) {
    XMoveResizeWindow(dpy, current_layout -> windows[i].window,
      current_layout -> windows[i].x, current_layout -> windows[i].y,
      current_layout -> windows[i].width, current_layout -> windows[i].height);
  }
}

// Workspace functions
void init_workspace_manager() {
  workspace_manager.current_workspace = 1;
  for (int i = 1; i < MAX_WORKSPACES; i++) {
    workspace_manager.layouts[i].count = 0;
    workspace_manager.layouts[i].master = None;
  }
}

void switch_workspace(Display * dpy, int workspace_index) {
  if (workspace_index < 1 || workspace_index > MAX_WORKSPACES) return;

  // Hide windows in current workspace
  TilingLayout * current_layout = & workspace_manager.layouts[workspace_manager.current_workspace];
  for (int i = 0; i < current_layout -> count; i++) {
    XUnmapWindow(dpy, current_layout -> windows[i].window);
  }

  // Change to new workspace
  workspace_manager.current_workspace = workspace_index;

  // Show windows in new workspace
  TilingLayout * new_layout = & workspace_manager.layouts[workspace_manager.current_workspace];
  for (int i = 0; i < new_layout -> count; i++) {
    XMapWindow(dpy, new_layout -> windows[i].window);
  }

  // Reapply layout for the new workspace
  arrange_window(dpy, DisplayWidth(dpy, DefaultScreen(dpy)), DisplayHeight(dpy, DefaultScreen(dpy)));
  apply_layout(dpy);

  printf("Switched to workspace %d\n", workspace_index);
}

void add_window_to_current_workspace(Display * dpy, Window window) {
  TilingLayout * current_layout = & workspace_manager.layouts[workspace_manager.current_workspace];
  add_window_to_layout(window, current_layout);
  arrange_window(dpy, DisplayWidth(dpy, DefaultScreen(dpy)), DisplayHeight(dpy, DefaultScreen(dpy)));
  apply_layout(dpy);
}

void remove_window_from_current_workspace(Display * dpy, Window window) {
  TilingLayout * current_layout = & workspace_manager.layouts[workspace_manager.current_workspace];
  remove_window_from_layout(window, current_layout);
  arrange_window(dpy, DisplayWidth(dpy, DefaultScreen(dpy)), DisplayHeight(dpy, DefaultScreen(dpy)));
  apply_layout(dpy);
}

void setup_keybindings(Display * dpy, Window root) {
  for (int i = 0; i < NUM_KEYBINDINGS; i++) {
    KeyCode keycode = XKeysymToKeycode(dpy, keybindings[i].keysym);
    XGrabKey(dpy, keycode, keybindings[i].modifier, root, True, GrabModeAsync, GrabModeAsync);
  }

  // Grab Moving and resizing keybinds
  XGrabButton(dpy, MOVE_BUTTON, MODIFIER, root, True, ButtonPressMask, GrabModeAsync,
    GrabModeAsync, None, None);
  XGrabButton(dpy, RESIZE_BUTTON, MODIFIER, root, True, ButtonPressMask, GrabModeAsync,
    GrabModeAsync, None, None);

  // Window keybindings
  XGrabKey(dpy, XKeysymToKeycode(dpy, KILL_KEY), MODIFIER, root, True, GrabModeAsync, GrabModeAsync);
}

// Set Cursor font to avoid no cursor
void set_default_cursor(Display * dpy, Window root) {
  Cursor cursor;
  cursor = XCreateFontCursor(dpy, XC_left_ptr);
  XDefineCursor(dpy, root, cursor);
  XFlush(dpy);
}

// Set wm name for fetches to use
void set_window_manager_name(Display * dpy, Window root,
  const char * wm_name) {
  Atom wmNameAtom = XInternAtom(dpy, "_NET_WM_NAME", False);
  XChangeProperty(dpy, root, wmNameAtom, XA_STRING, 8, PropModeReplace,
    (unsigned char * ) wm_name, strlen(wm_name));
  XFlush(dpy);
}

// Map window and Move window to 50, 50
void handle_map_request(XEvent ev, Display * dpy) {
  XWindowAttributes attr;
  XGetWindowAttributes(dpy, ev.xmaprequest.window, & attr);

  if (attr.override_redirect) {
    printf("Override redirect, skipping window\n");
    return;
  }

  printf("Mapping window 0x%lx\n", ev.xmaprequest.window);
  XSelectInput(dpy, ev.xmaprequest.window, EnterWindowMask | FocusChangeMask | StructureNotifyMask);
  XMapWindow(dpy, ev.xmaprequest.window);
  add_window_to_layout(ev.xmaprequest.window, & workspace_manager.layouts[workspace_manager.current_workspace]);
  add_window_to_current_workspace(dpy, ev.xmaprequest.window);
  arrange_window(dpy, DisplayWidth(dpy, DefaultScreen(dpy)), DisplayHeight(dpy, DefaultScreen(dpy)));
  apply_layout(dpy);
}

void handle_unmap_request(XEvent ev, Display * dpy) {
  remove_window_from_layout(ev.xunmap.window, & workspace_manager.layouts[workspace_manager.current_workspace]);
  remove_window_from_current_workspace(dpy, ev.xunmap.window);
  arrange_window(dpy, DisplayWidth(dpy, DefaultScreen(dpy)), DisplayHeight(dpy, DefaultScreen(dpy)));
  apply_layout(dpy);
}

void handle_configure_request(XEvent ev, Display * dpy) {
  XConfigureRequestEvent * req = & ev.xconfigurerequest;
  XWindowChanges changes;
  changes.x = req -> x;
  changes.y = req -> y;
  changes.width = req -> width;
  changes.height = req -> height;
  changes.border_width = req -> border_width;

  printf("Configure request: window 0x%lx, (%d, %d, %d, %d)\n",
    req -> window, req -> x, req -> y, req -> width, req -> height);

  // Determine if this window should have `XConfigureWindow` applied
  char * window_name = NULL;
  XFetchName(dpy, req -> window, & window_name);
  int should_configure = 1;

  if (window_name) {
    if (strcmp(window_name, "firefox") == 0) {
      should_configure = 0; // Skip Firefox
    }
    XFree(window_name);
  }

  // If not Firefox or similar apps, apply configuration
  if (should_configure) {
    XConfigureWindow(dpy, req -> window, req -> value_mask, & changes);
  }

  // Regardless, maintain the internal layout
  for (int i = 0; i < layout.count; i++) {
    if (layout.windows[i].window == req -> window) {
      layout.windows[i].x = changes.x;
      layout.windows[i].y = changes.y;
      layout.windows[i].width = changes.width;
      layout.windows[i].height = changes.height;
      break;
    }
  }

  arrange_window(dpy, DisplayWidth(dpy, DefaultScreen(dpy)), DisplayHeight(dpy, DefaultScreen(dpy)));
  apply_layout(dpy);
  XSync(dpy, False);
}

// Handle moving and resizing
void start_drag(Display * dpy, XEvent ev, DragState * drag) {
  if (ev.xbutton.subwindow != None) {
    drag -> window = ev.xbutton.subwindow;
    drag -> start_x = ev.xbutton.x_root;
    drag -> start_y = ev.xbutton.y_root;

    XWindowAttributes attr;
    XGetWindowAttributes(dpy, drag -> window, & attr);
    drag -> x = attr.x;
    drag -> y = attr.y;
    drag -> width = attr.width;
    drag -> height = attr.height;
    drag -> is_resizing = (ev.xbutton.button == RESIZE_BUTTON);

    XGrabPointer(dpy, drag -> window, True,
      PointerMotionMask | ButtonReleaseMask, GrabModeAsync,
      GrabModeAsync, None, None, CurrentTime);

    printf("Starting %s on window 0x%lx\n", drag -> is_resizing ? "resize" : "move", drag -> window);
  }
}

void update_drag(Display * dpy, XEvent ev, DragState * drag) {
  if (drag -> window != None) {
    int xdiff = ev.xmotion.x_root - drag -> start_x;
    int ydiff = ev.xmotion.y_root - drag -> start_y;

    if (drag -> is_resizing) {
      XMoveResizeWindow(dpy, drag -> window,
        drag -> x, drag -> y,
        MAX(1, drag -> width + xdiff),
        MAX(1, drag -> height + ydiff));
    } else {
      XMoveWindow(dpy, drag -> window,
        drag -> x + xdiff,
        drag -> y + ydiff);
    }
  }
}

void end_drag(Display * dpy, DragState * drag) {
  if (drag -> window != None) {
    XUngrabPointer(dpy, CurrentTime);
    drag -> window = None;
    printf("Drag ended\n");
  }
}

// Focus window
void raise_window(Display * dpy, Window window) {
  XRaiseWindow(dpy, window);
  XSetInputFocus(dpy, window, RevertToPointerRoot, CurrentTime);
  printf("Window 0x%lx raised and focused\n", window);
}

void kill_focused_window(Display * dpy) {
  Window focused_window;
  int revert_to;

  // Get the currently focused window
  XGetInputFocus(dpy, & focused_window, & revert_to);

  if (focused_window != None && focused_window != PointerRoot) {
    XDestroyWindow(dpy, focused_window);
    printf("Killed window 0x%lx\n", focused_window);
  } else {
    printf("No window is focused\n");
  }
}

void handle_keypress_event(XEvent ev, Display * dpy) {
  KeySym keysym = XkbKeycodeToKeysym(dpy, ev.xkey.keycode, 0, 0);

  // Kill focused window
  if (keysym == KILL_KEY && (ev.xkey.state & MODIFIER)) {
    kill_focused_window(dpy);
    return;
  }

  // Get all keybindings for programs
  for (int i = 0; i < NUM_KEYBINDINGS; i++) {
    if (keysym == keybindings[i].keysym && (ev.xkey.state & keybindings[i].modifier)) {
      if (keybindings[i].workspace != -1) {
        switch_workspace(dpy, keybindings[i].workspace);
      } else if (keybindings[i].command) {
        // Execute the command
        char command[256];
        snprintf(command, sizeof(command), "%s &", keybindings[i].command);
        system(command);
        printf("Executed command: %s\n", keybindings[i].command);
      }
      return;
    }
  }
}

void handle_events(Display * dpy, Window root, int scr) {
  DragState drag = {
    0
  };
  XEvent ev;
  for (;;) {
    XNextEvent(dpy, & ev);

    switch (ev.type) {
    case MapRequest:
      printf("Map Request\n");
      handle_map_request(ev, dpy);
      raise_window(dpy, ev.xmaprequest.window);
      break;
    case UnmapNotify:
      printf("Unmap Notify\n");
      handle_unmap_request(ev, dpy);
    case ConfigureRequest:
      printf("Configure Request\n");
      handle_configure_request(ev, dpy);
      break;
    case Expose:
      if (ev.xexpose.count == 0) {
        XClearWindow(dpy, ev.xexpose.window);
      }
    case EnterNotify:
      if (ev.xcrossing.window != root) {
        printf("Mouse entered window 0x%lx, raising and focusing it\n", ev.xcrossing.window);
        raise_window(dpy, ev.xcrossing.window);
      }
      break;
    case ButtonPress:
      if (ev.xbutton.subwindow != None) {
        // Resizing and Moving
        if ((ev.xbutton.state & MODIFIER) && (ev.xbutton.button == MOVE_BUTTON || ev.xbutton.button == RESIZE_BUTTON)) {
          start_drag(dpy, ev, & drag);
        }
      }
      break;
    case MotionNotify:
      while (XCheckTypedEvent(dpy, MotionNotify, & ev));
      update_drag(dpy, ev, & drag);
      break;
    case ButtonRelease:
      end_drag(dpy, & drag);
      break;
    case KeyPress:
      handle_keypress_event(ev, dpy);
      break;
    default:
      printf("Other event type: %d\n", ev.type);
      break;
    }
  }
}

static int error_occurred = 0;

int handle_x_error(Display * dpy, XErrorEvent * error_event) {
  if (error_event -> error_code == BadAccess) {
    error_occurred = 1;
  }
  return 0;
}

int main() {
  Display * dpy;
  int scr;
  Window root;

  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    errx(1, "Couldn't open display");
  }

  scr = DefaultScreen(dpy);
  root = RootWindow(dpy, scr);
  XSetErrorHandler(handle_x_error);
  error_occurred = 0;

  XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask |
    ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask | EnterWindowMask);

  system("/usr/bin/autostart.sh &");

  XSync(dpy, False);

  if (error_occurred) {
    errx(1, "Another window manager is running");
  }

  printf("Opened display\n");

  init_layout();
  init_workspace_manager();
  setup_keybindings(dpy, root);
  set_default_cursor(dpy, root);
  set_window_manager_name(dpy, root, WM_NAME);

  handle_events(dpy, root, scr);

  XCloseDisplay(dpy);
  return 0;
}
