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
  int border_width;
  int is_floating;
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

// EWMH properties
Atom net_supported, net_wm_name, net_wm_state, net_wm_state_fullscreen, net_wm_desktop,
net_client_list, net_current_desktop, net_number_of_desktops, net_active_window;

// Window decorations
void hex_to_rgb(const char * hex, XColor * color, Display * dpy) {
  unsigned int r, g, b;
  if (sscanf(hex, "#%02x%02x%02x", & r, & g, & b) != 3) {
    fprintf(stderr, "Invalid color format: %s\n", hex);
    return;
  }

  color -> red = r * 257; // Scale up to 16-bit
  color -> green = g * 257; // Scale up to 16-bit
  color -> blue = b * 257; // Scale up to 16-bit
  color -> flags = DoRed | DoGreen | DoBlue;
  XAllocColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), color);
}

void draw_window_border(Display * dpy, Window window, int border_width,
  const char * color_hex) {
  XColor color;
  hex_to_rgb(color_hex, & color, dpy);

  XSetWindowBorder(dpy, window, color.pixel);
  XSetWindowBorderWidth(dpy, window, border_width);
}

void focus_next_window(Display * dpy) {
  TilingLayout * current_layout = & workspace_manager.layouts[workspace_manager.current_workspace];
  if (current_layout -> count == 0) return; // No windows

  Window focused_window;
  int revert_to;
  XGetInputFocus(dpy, & focused_window, & revert_to);

  int index = -1;
  for (int i = 0; i < current_layout -> count; i++) { // Corrected loop condition
    if (current_layout -> windows[i].window == focused_window) {
      index = i;
      break;
    }
  }

  if (index == -1) {
    // Focused window not found in the list, default to the first window
    index = 0;
  } else {
    // Focus the next window in the list
    index = (index + 1) % current_layout -> count;
  }

  Window next_window = current_layout -> windows[index].window;
  if (next_window) {
    XRaiseWindow(dpy, next_window);
    XSetInputFocus(dpy, next_window, RevertToPointerRoot, CurrentTime);
  }
}

void focus_prev_window(Display * dpy) {
  TilingLayout * current_layout = & workspace_manager.layouts[workspace_manager.current_workspace];
  if (current_layout -> count == 0) return; // No windows to focus on

  Window focused_window;
  int revert_to;
  XGetInputFocus(dpy, & focused_window, & revert_to);

  int index = -1;
  for (int i = 0; i < current_layout -> count; i++) {
    if (current_layout -> windows[i].window == focused_window) {
      index = i;
      break;
    }
  }

  if (index == -1) {
    // Focused window not found in the list, default to the last window
    index = current_layout -> count - 1;
  } else {
    // Focus the previous window in the list
    index = (index - 1 + current_layout -> count) % current_layout -> count;
  }

  Window prev_window = current_layout -> windows[index].window;
  if (prev_window) {
    XRaiseWindow(dpy, prev_window);
    XSetInputFocus(dpy, prev_window, RevertToPointerRoot, CurrentTime);
  }
}

// Tiling functions
void init_layout() {
  layout.count = 0;
  layout.master = None;
}

Bool is_floating_window(Display * dpy, Window win) {
  Atom net_wm_window_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", True);
  Atom net_wm_window_type_dock = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", True);
  Atom net_wm_window_type_dialog = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", True);

  if (net_wm_window_type != None) {
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char * prop = NULL;

    if (XGetWindowProperty(dpy, win, net_wm_window_type, 0, 1, False, XA_ATOM, & actual_type, & actual_format, & nitems, & bytes_after, & prop) == Success) {
      if (nitems > 0) {
        Atom window_type = * (Atom * ) prop;
        XFree(prop);
        return window_type == net_wm_window_type_dock || window_type == net_wm_window_type_dialog;
      }
    }
  }

  XClassHint class_hint;
  if (XGetClassHint(dpy, win, & class_hint)) {
    if (strcmp(class_hint.res_class, "Dock") == 0 || strcmp(class_hint.res_class, "Toolbar") == 0) {
      XFree(class_hint.res_name);
      XFree(class_hint.res_class);
      return True;
    }
    XFree(class_hint.res_name);
    XFree(class_hint.res_class);
  }

  return False;
}

void add_window_to_layout(Display * dpy, Window window, TilingLayout * layout) {
  if (layout -> count >= MAX_WINDOWS) {
    fprintf(stderr, "Window limit exceeded\n");
    return;
  }

  // Check if window already exists in layout
  for (int i = 0; i < layout -> count; i++) {
    if (layout -> windows[i].window == window) {
      return; // Window already exists in layout
    }
  }

  int is_floating = is_floating_window(dpy, window);

  // Add window to layout
  layout -> windows[layout -> count].window = window;
  layout -> windows[layout -> count].border_width = BORDER_WIDTH;
  layout -> windows[layout -> count].is_floating = is_floating;
  draw_window_border(dpy, window, BORDER_WIDTH, BORDER_COLOR);
  layout -> count++;

  if (layout -> master == None) {
    layout -> master = window;
  }

  printf("Window 0x%lx added. Total windows: %d\n", window, layout -> count);
}

void remove_window_from_layout(Window window, TilingLayout * layout, Display * dpy) {
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
  if (layout -> count > 0) {
    focus_next_window(dpy);
  }
}

void arrange_window(Display * dpy, int screen_width, int screen_height) {
  TilingLayout * current_layout = & workspace_manager.layouts[workspace_manager.current_workspace];
  if (current_layout -> count == 0) return; // No windows to arrange

  int tiling_count = 0;

  // Count only non-floating windows
  for (int i = 0; i < current_layout -> count; i++) {
    if (!is_floating_window(dpy, current_layout -> windows[i].window)) {
      tiling_count++;
    }
  }

  // No windows to tile or only floating windows
  if (tiling_count == 0) return;

  // Calculate the usable area considering the gaps
  int usable_width = screen_width - 2 * OUTER_GAP;
  int usable_height = screen_height - 2 * OUTER_GAP;

  if (tiling_count == 1) {
    // Only one non-floating window, make it full screen with gaps
    for (int i = 0; i < current_layout -> count; i++) {
      if (!is_floating_window(dpy, current_layout -> windows[i].window)) {
        current_layout -> windows[i].x = OUTER_GAP;
        current_layout -> windows[i].y = OUTER_GAP;
        current_layout -> windows[i].width = usable_width;
        current_layout -> windows[i].height = usable_height;
        break;
      }
    }
  } else {
    // More than one window, apply tiling layout
    int master_width = (usable_width / 2) - (INNER_GAP / 2);
    int stack_width = (usable_width - master_width) - INNER_GAP;

    // Avoid division by zero
    int stack_height = (usable_height - INNER_GAP * (tiling_count - 2)) / (tiling_count - 1);
    if (tiling_count == 2) {
      stack_height = usable_height; // Special case for two windows
    }

    int tiling_index = 0;
    for (int i = 0; i < current_layout -> count; i++) {
      if (!is_floating_window(dpy, current_layout -> windows[i].window)) {
        if (tiling_index == 0) {
          current_layout -> windows[i].x = OUTER_GAP;
          current_layout -> windows[i].y = OUTER_GAP;
          current_layout -> windows[i].width = master_width;
          current_layout -> windows[i].height = usable_height;
        } else {
          current_layout -> windows[i].x = OUTER_GAP + master_width + INNER_GAP;
          current_layout -> windows[i].y = OUTER_GAP + (stack_height + INNER_GAP) * (tiling_index - 1);
          current_layout -> windows[i].width = stack_width;
          current_layout -> windows[i].height = stack_height;
        }
        tiling_index++;
      }
    }
  }
}

void apply_layout(Display * dpy) {
  TilingLayout * current_layout = & workspace_manager.layouts[workspace_manager.current_workspace];
  for (int i = 0; i < current_layout -> count; i++) {
    if (!current_layout -> windows[i].is_floating) {
      XMoveResizeWindow(dpy, current_layout -> windows[i].window,
        current_layout -> windows[i].x,
        current_layout -> windows[i].y,
        current_layout -> windows[i].width,
        current_layout -> windows[i].height);

    }
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

void move_window_to_workspace(Display * dpy, int target_workspace) {
  if (target_workspace < 1 || target_workspace > MAX_WORKSPACES) return;

  TilingLayout * current_layout = & workspace_manager.layouts[workspace_manager.current_workspace];
  Window focused_window;
  int revert_to;

  // Get the currently focused window
  XGetInputFocus(dpy, & focused_window, & revert_to);

  if (focused_window == None || focused_window == PointerRoot) {
    printf("No window is focused\n");
    return;
  }

  // Remove window from current workspace
  remove_window_from_layout(focused_window, current_layout, dpy);
  XUnmapWindow(dpy, focused_window);

  // Add window to the target workspace
  TilingLayout * target_layout = & workspace_manager.layouts[target_workspace];
  add_window_to_layout(dpy, focused_window, target_layout);

  // Set the window state to withdrawn to ensure it is managed correctly in the new workspace
  XEvent ev;
  ev.type = UnmapNotify;
  ev.xunmap.window = focused_window;
  XSendEvent(dpy, focused_window, False, StructureNotifyMask, & ev);

  printf("Moved window 0x%lx to workspace %d\n", focused_window, target_workspace);
}

void switch_workspace(Display * dpy, int workspace_index) {
  if (workspace_index < 1 || workspace_index > MAX_WORKSPACES) return;

  if (workspace_manager.current_workspace == workspace_index) {
    printf("Already on workspace %d\n", workspace_index);
    return;
  }

  TilingLayout * current_layout = & workspace_manager.layouts[workspace_manager.current_workspace];
  TilingLayout * new_layout = & workspace_manager.layouts[workspace_index];

  // Hide windows in current workspace
  for (int i = 0; i < current_layout -> count; i++) {
    XUnmapWindow(dpy, current_layout -> windows[i].window);
  }

  // Change to new workspace
  workspace_manager.current_workspace = workspace_index;

  // Show windows in new workspace
  for (int i = 0; i < new_layout -> count; i++) {
    XMapWindow(dpy, new_layout -> windows[i].window);
  }

  for (int i = new_layout -> count - 1; i >= 0; i--) {
    XRaiseWindow(dpy, new_layout -> windows[i].window);
  }

  // Reapply layout for the new workspace
  arrange_window(dpy, DisplayWidth(dpy, DefaultScreen(dpy)), DisplayHeight(dpy, DefaultScreen(dpy)));
  apply_layout(dpy);

  printf("Switched to workspace %d\n", workspace_index);
}

void add_window_to_current_workspace(Display * dpy, Window window) {
  TilingLayout * current_layout = & workspace_manager.layouts[workspace_manager.current_workspace];
  add_window_to_layout(dpy, window, current_layout);
  XMapWindow(dpy, window);
  arrange_window(dpy, DisplayWidth(dpy, DefaultScreen(dpy)), DisplayHeight(dpy, DefaultScreen(dpy)));
  apply_layout(dpy);
}

void remove_window_from_current_workspace(Display * dpy, Window window) {
  TilingLayout * current_layout = & workspace_manager.layouts[workspace_manager.current_workspace];
  remove_window_from_layout(window, current_layout, dpy);
  XUnmapWindow(dpy, window);
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

  // Focus keybindings
  XGrabKey(dpy, XKeysymToKeycode(dpy, NEXT_WINDOW_KEY), MODIFIER, root, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode(dpy, PREV_WINDOW_KEY), MODIFIER, root, True, GrabModeAsync, GrabModeAsync);
}

// Set Cursor font to avoid no cursor
void set_default_cursor(Display * dpy, Window root) {
  Cursor cursor;
  cursor = XCreateFontCursor(dpy, XC_left_ptr);
  XDefineCursor(dpy, root, cursor);
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
  add_window_to_layout(dpy, ev.xmaprequest.window, & workspace_manager.layouts[workspace_manager.current_workspace]);
  add_window_to_current_workspace(dpy, ev.xmaprequest.window);
  arrange_window(dpy, DisplayWidth(dpy, DefaultScreen(dpy)), DisplayHeight(dpy, DefaultScreen(dpy)));
  apply_layout(dpy);
}

void handle_unmap_request(XEvent ev, Display * dpy) {
  remove_window_from_layout(ev.xunmap.window, & workspace_manager.layouts[workspace_manager.current_workspace], dpy);
  remove_window_from_current_workspace(dpy, ev.xunmap.window);

  focus_next_window(dpy);

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

  // Focus next window
  if (keysym == NEXT_WINDOW_KEY && (ev.xkey.state & MODIFIER)) {
    focus_next_window(dpy);
    return;
  }

  // Focus previous window
  if (keysym == PREV_WINDOW_KEY && (ev.xkey.state & MODIFIER)) {
    focus_prev_window(dpy);
    return;
  }

  // Get all keybindings for programs
  for (int i = 0; i < NUM_KEYBINDINGS; i++) {
    if (keysym == keybindings[i].keysym && (ev.xkey.state & keybindings[i].modifier)) {
      if (keybindings[i].workspace != -1 && (ev.xkey.state & ShiftMask)) {
        move_window_to_workspace(dpy, keybindings[i].workspace);
      } else if (keybindings[i].workspace != -1) {
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

void handle_client_message(XEvent * e, Display * dpy) {
  if (e -> xclient.message_type == net_wm_state) {
    Window window = e -> xclient.window;
    Atom state = (Atom) e -> xclient.data.l[1];
    Bool add = e -> xclient.data.l[0] == 1;

    if (state == net_wm_state_fullscreen) {
      if (add) {
        // Make window fullscreen
        XMoveResizeWindow(dpy, window, 0, 0,
                          XDisplayWidth(dpy, DefaultScreen(dpy)), XDisplayHeight(dpy, DefaultScreen(dpy)));
      } else {
        // Exit fullscreen
        // Implementation depends on your window manager logic
      }
    }
  }
  // Handle other client messages as needed
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
    case ClientMessage:
      handle_client_message(&ev, dpy);
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

// EWMH
void init_ewmh_atoms(Display * dpy) {
  net_supported = XInternAtom(dpy, "_NET_SUPPORTED", False);
  net_wm_name = XInternAtom(dpy, "_NET_WM_NAME", False);
  net_wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
  net_wm_state_fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
  net_wm_desktop = XInternAtom(dpy, "_NET_WM_DESKTOP", False);
  net_client_list = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
  net_current_desktop = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
  net_number_of_desktops = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
  net_active_window = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
}

void set_supported_atoms(Display * dpy, Window root) {
  Atom supported_atoms[] = {
    net_supported,
    net_wm_name,
    net_wm_state,
    net_wm_state_fullscreen,
    net_wm_desktop,
    net_client_list,
    net_current_desktop,
    net_number_of_desktops,
    net_active_window,
  };

  XChangeProperty(dpy, root, net_supported, XA_ATOM, 32, PropModeReplace,
    (unsigned char * ) supported_atoms, sizeof(supported_atoms) / sizeof(Atom));
}

void set_window_title(Display * dpy, Window win,
  const char * title) {
  XChangeProperty(dpy, win, net_wm_name, XA_STRING, 8, PropModeReplace,
    (unsigned char * ) title, strlen(title));
}

void set_window_state(Window window, Atom state, Bool add, Display * dpy) {
  XEvent e;
  memset( & e, 0, sizeof(e));
  e.type = ClientMessage;
  e.xclient.window = window;
  e.xclient.message_type = net_wm_state;
  e.xclient.format = 32;
  e.xclient.data.l[0] = add ? 1 : 0; // 1 for add, 0 for remove
  e.xclient.data.l[1] = state;
  XSendEvent(dpy, DefaultRootWindow(dpy), False, SubstructureNotifyMask | SubstructureRedirectMask, & e);
}

void set_active_window(Window window, Display * dpy) {
  XChangeProperty(dpy, DefaultRootWindow(dpy), net_active_window, XA_WINDOW, 32, PropModeReplace, (unsigned char * ) & window, 1);
}

void update_client_list(Window * windows, int count, Display * dpy) {
  XChangeProperty(dpy, DefaultRootWindow(dpy), net_client_list, XA_WINDOW, 32, PropModeReplace, (unsigned char * ) windows, count);
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

  XSync(dpy, False);

  if (error_occurred) {
    errx(1, "Another window manager is running");
  }

  printf("Opened display\n");

  // Launch startup commands
  system("/usr/bin/autostart.sh &");

  init_layout();

  // EWMH
  init_ewmh_atoms(dpy);
  set_supported_atoms(dpy, root);
  set_window_title(dpy, root, WM_NAME);

  init_workspace_manager();
  setup_keybindings(dpy, root);
  set_default_cursor(dpy, root);

  handle_events(dpy, root, scr);

  XCloseDisplay(dpy);
}
