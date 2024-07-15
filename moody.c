#include <X11/X.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "structs.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

TilingLayout layout;
WorkspaceManager workspace_manager;
DockGeometry dock_geometry;

// EWMH properties
Atom net_supported, net_wm_name, net_supporting_wm_check, net_wm_state,
    net_wm_state_fullscreen, net_wm_desktop, net_client_list,
    net_current_desktop, net_number_of_desktops, net_active_window;

// EWMH
void init_ewmh_atoms(Display *dpy) {
  net_supported = XInternAtom(dpy, "_NET_SUPPORTED", False);
  net_wm_name = XInternAtom(dpy, "_NET_WM_NAME", False);
  net_supporting_wm_check = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
  net_wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
  net_wm_state_fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
  net_wm_desktop = XInternAtom(dpy, "_NET_WM_DESKTOP", False);
  net_client_list = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
  net_current_desktop = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
  net_number_of_desktops = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
  net_active_window = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
}

void set_supported_atoms(Display *dpy, Window root) {
  Atom supported_atoms[] = {
      net_supported,           net_wm_name,
      net_supporting_wm_check, net_wm_state,
      net_wm_state_fullscreen, net_wm_desktop,
      net_client_list,         net_current_desktop,
      net_number_of_desktops,  net_active_window,
  };

  XChangeProperty(dpy, root, net_supported, XA_ATOM, 32, PropModeReplace,
                  (unsigned char *)supported_atoms,
                  sizeof(supported_atoms) / sizeof(Atom));
}

void set_window_title(Display *dpy, Window win, const char *title) {
  XChangeProperty(dpy, win, net_wm_name, XA_STRING, 8, PropModeReplace,
                  (unsigned char *)title, strlen(title));
}

void set_supporting_wm_check(Display *dpy, Window root) {
  Window wm_check_win = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
  XChangeProperty(dpy, root, net_supporting_wm_check, XA_WINDOW, 32,
                  PropModeReplace, (unsigned char *)&wm_check_win, 1);
  XChangeProperty(dpy, wm_check_win, net_supporting_wm_check, XA_WINDOW, 32,
                  PropModeReplace, (unsigned char *)&wm_check_win, 1);
  XChangeProperty(dpy, wm_check_win, net_wm_name, XA_STRING, 8, PropModeReplace,
                  (unsigned char *)WM_NAME, 16);
  XMapWindow(dpy, wm_check_win);
}

void set_window_state(Window window, Atom state, bool add, Display *dpy) {
  XEvent e;
  memset(&e, 0, sizeof(e));
  e.type = ClientMessage;
  e.xclient.window = window;
  e.xclient.message_type = net_wm_state;
  e.xclient.format = 32;
  e.xclient.data.l[0] = add ? 1 : 0;  // 1 for add, 0 for remove
  e.xclient.data.l[1] = state;
  XSendEvent(dpy, DefaultRootWindow(dpy), False,
             SubstructureNotifyMask | SubstructureRedirectMask, &e);
}

void set_active_window(Display *dpy, Window root, Window active_window) {
  XChangeProperty(dpy, root, net_active_window, XA_WINDOW, 32, PropModeReplace,
                  (unsigned char *)&active_window, 1);
}

void update_client_list(Display *dpy, Window root, WindowInfo *windows,
                        int count) {
  Window client_list[count];
  for (int i = 0; i < count; ++i) {
    client_list[i] = windows[i].window;
  }
  XChangeProperty(dpy, root, net_client_list, XA_WINDOW, 32, PropModeReplace,
                  (unsigned char *)client_list, count);
}

void set_window_desktop(Display *dpy, Window win, int desktop) {
  XChangeProperty(dpy, win, net_wm_desktop, XA_CARDINAL, 32, PropModeReplace,
                  (unsigned char *)&desktop, 1);
}

void set_current_desktop(Display *dpy, Window root, int desktop) {
  XChangeProperty(dpy, root, net_current_desktop, XA_CARDINAL, 32,
                  PropModeReplace, (unsigned char *)&desktop, 1);
}

void set_number_of_desktops(Display *dpy, Window root, int num_desktops) {
  XChangeProperty(dpy, root, net_number_of_desktops, XA_CARDINAL, 32,
                  PropModeReplace, (unsigned char *)&num_desktops, 1);
}

void init_ewmh(Display *dpy, Window root) {
  init_ewmh_atoms(dpy);
  set_supported_atoms(dpy, root);
  set_window_title(dpy, root, WM_NAME);
  set_supporting_wm_check(dpy, root);
  set_number_of_desktops(dpy, root, MAX_WORKSPACES);
  set_current_desktop(dpy, root, 0);
}

// Status bar
bool is_dock_window(Display *dpy, Window win) {
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  Atom *props = NULL;
  bool result = false;

  Atom net_wm_window_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
  Atom net_wm_window_type_dock =
      XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);

  if (XGetWindowProperty(dpy, win, net_wm_window_type, 0, (~0L), False, XA_ATOM,
                         &actual_type, &actual_format, &nitems, &bytes_after,
                         (unsigned char **)&props) == Success) {
    if (actual_type == XA_ATOM && actual_format == 32) {
      for (unsigned long i = 0; i < nitems; i++) {
        if (props[i] == net_wm_window_type_dock) {
          result = true;
          break;
        }
      }
    }
    if (props) {
      XFree(props);
    }
  }

  return result;
}

void update_dock_geometry(Display *dpy, Window win) {
  XWindowAttributes attrs;
  XGetWindowAttributes(dpy, win, &attrs);

  // Assuming the dock is always at the top or bottom of the screen
  if (attrs.y == 0) {
    // Dock is at the top
    dock_geometry.x = attrs.x;
    dock_geometry.y = attrs.y;
    dock_geometry.width = attrs.width;
    dock_geometry.height = attrs.height;
  } else {
    // Dock is at the bottom
    int screen_height = DisplayHeight(dpy, DefaultScreen(dpy));
    dock_geometry.x = attrs.x;
    dock_geometry.y = screen_height - attrs.height;
    dock_geometry.width = attrs.width;
    dock_geometry.height = attrs.height;
  }

  printf("Updated dock geometry: x=%d, y=%d, width=%d, height=%d\n",
         dock_geometry.x, dock_geometry.y, dock_geometry.width,
         dock_geometry.height);
}

// Window decorations
void hex_to_rgb(const char *hex, XColor *color, Display *dpy) {
  unsigned int r, g, b;
  if (sscanf(hex, "#%02x%02x%02x", &r, &g, &b) != 3) {
    fprintf(stderr, "Invalid color format: %s\n", hex);
    return;
  }

  color->red = r * 257;    // Scale up to 16-bit
  color->green = g * 257;  // Scale up to 16-bit
  color->blue = b * 257;   // Scale up to 16-bit
  color->flags = DoRed | DoGreen | DoBlue;
  XAllocColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), color);
}

void draw_window_border(Display *dpy, Window window, int border_width,
                        const char *color_hex) {
  XColor color;
  hex_to_rgb(color_hex, &color, dpy);

  XSetWindowBorder(dpy, window, color.pixel);
  XSetWindowBorderWidth(dpy, window, border_width);
}

// Focus window
void focus_window(Display *dpy, Window window) {
  TilingLayout current_workspace =
      workspace_manager.layouts[workspace_manager.current_workspace];

  if (is_dock_window(dpy, window)) {
    return;
  }

  // Interate through all windows in the current workspace and set an inactive
  // border color for them
  for (int i = 0; i < current_workspace.count; i++) {
    draw_window_border(dpy, current_workspace.windows[i].window, BORDER_WIDTH,
                       INACTIVE_BORDER_COLOR);
  }

  // Focus window and set active border color for window
  XRaiseWindow(dpy, window);
  XSetInputFocus(dpy, window, RevertToPointerRoot, CurrentTime);
  set_active_window(dpy, RootWindow(dpy, DefaultScreen(dpy)), window);
  draw_window_border(dpy, window, BORDER_WIDTH, BORDER_COLOR);
  printf("Window 0x%lx raised and focused\n", window);
}

void focus_next_window(Display *dpy) {
  TilingLayout *current_layout =
      &workspace_manager
           .layouts[workspace_manager.current_workspace];  // get the current
                                                           // workspace layout
                                                           // struct
  if (current_layout->count == 0) return;                  // No windows

  Window focused_window;
  int revert_to;
  XGetInputFocus(dpy, &focused_window, &revert_to);

  int index = -1;
  for (int i = 0; i < current_layout->count;
       i++) {  // loop through all windows in current workspace
    if (current_layout->windows[i].window == focused_window) {
      index = i;
      break;
    }
  }

  if (index == -1) {
    // Focused window not found in the list, default to the first window
    index = 0;
  } else {
    // Focus the next window in the list
    index = (index + 1) % current_layout->count;
  }

  Window next_window = current_layout->windows[index].window;
  if (next_window) {
    // Set border color for inactive window
    draw_window_border(dpy, focused_window, BORDER_WIDTH,
                       INACTIVE_BORDER_COLOR);
    // Focus the next window and set active border
    focus_window(dpy, next_window);
  }
}

void focus_prev_window(Display *dpy) {
  TilingLayout *current_layout =
      &workspace_manager.layouts[workspace_manager.current_workspace];
  if (current_layout->count == 0) return;  // No windows to focus on

  Window focused_window;
  int revert_to;
  XGetInputFocus(dpy, &focused_window, &revert_to);

  int index = -1;
  for (int i = 0; i < current_layout->count; i++) {
    if (current_layout->windows[i].window == focused_window) {
      index = i;
      break;
    }
  }

  if (index == -1) {
    // Focused window not found in the list, default to the last window
    index = current_layout->count - 1;
  } else {
    // Focus the previous window in the list
    index = (index - 1 + current_layout->count) % current_layout->count;
  }

  Window prev_window = current_layout->windows[index].window;
  if (prev_window) {
    draw_window_border(dpy, focused_window, BORDER_WIDTH,
                       INACTIVE_BORDER_COLOR);
    // Focus the previous window and set active border
    focus_window(dpy, prev_window);
  }
}

// Tiling functions
void init_layouts() {
  for (int i = 0; i < MAX_WORKSPACES; i++) {
    workspace_manager.layouts[i].count = 0;
    workspace_manager.layouts[i].master = None;
  }
  workspace_manager.current_workspace = 0;
}

void arrange_windows(Display *dpy) {
  TilingLayout *layout =
      &workspace_manager.layouts[workspace_manager.current_workspace];
  int screen = DefaultScreen(dpy);
  int screen_width = DisplayWidth(dpy, screen);
  int screen_height = DisplayHeight(dpy, screen);

  // Adjust screen dimensions for dock and outer gaps
  int adjusted_screen_width = screen_width - (2 * OUTER_GAPS);
  int adjusted_screen_height =
      screen_height - (2 * OUTER_GAPS) - dock_geometry.height;

  if (layout->count == 0) return;

  // Special case: If there's only one window, make it fullscreen (with outer
  // gaps and respecting dock)
  if (layout->count == 1) {
    WindowInfo *win = &layout->windows[0];
    if (!win->is_floating) {
      win->x = OUTER_GAPS;
      win->y = OUTER_GAPS + (dock_geometry.y == 0 ? dock_geometry.height : 0);
      win->width = adjusted_screen_width - 2 * win->border_width;
      win->height = adjusted_screen_height - 2 * win->border_width;
      XMoveResizeWindow(dpy, win->window, win->x, win->y, win->width,
                        win->height);
    }
    return;
  }

  // Tiling logic for multiple windows, now with gaps and respecting dock
  float master_size = 0.6;  // 60% of the screen width for master window
  int master_width = (adjusted_screen_width * master_size) - INNER_GAPS / 2;
  int stack_width = adjusted_screen_width - master_width - INNER_GAPS / 2;

  for (int i = 0; i < layout->count; i++) {
    WindowInfo *win = &layout->windows[i];
    if (win->is_floating || is_dock_window(dpy, win->window)) continue;

    if (win->window == layout->master) {
      win->x = OUTER_GAPS;
      win->y = OUTER_GAPS + (dock_geometry.y == 0 ? dock_geometry.height : 0);
      win->width = master_width - 2 * win->border_width;
      win->height = adjusted_screen_height - 2 * win->border_width;
    } else {
      int stack_height = (adjusted_screen_height / (layout->count - 1)) -
                         INNER_GAPS - 2 * win->border_width;
      win->x = OUTER_GAPS + master_width + INNER_GAPS;
      win->y = OUTER_GAPS + (dock_geometry.y == 0 ? dock_geometry.height : 0) +
               (i - 1) * (stack_height + INNER_GAPS + 2 * win->border_width);
      win->width = stack_width - 2 * win->border_width;
      win->height = stack_height;
    }

    XMoveResizeWindow(dpy, win->window, win->x, win->y, win->width,
                      win->height);
  }
}

bool is_floating_window(Display *dpy, Window win) {
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  Atom *props = NULL;
  bool result = false;

  Atom net_wm_window_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
  Atom net_wm_window_type_toolbar =
      XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);

  if (XGetWindowProperty(dpy, win, net_wm_window_type, 0, (~0L), False, XA_ATOM,
                         &actual_type, &actual_format, &nitems, &bytes_after,
                         (unsigned char **)&props) == Success) {
    if (actual_type == XA_ATOM && actual_format == 32) {
      for (unsigned long i = 0; i < nitems; i++) {
        if (props[i] == props[i] == net_wm_window_type_toolbar) {
          result = true;
          break;
        }
      }
    }
    if (props) {
      XFree(props);
    }
  }

  return result;
}

void add_window_to_current_workspace(Display *dpy, Window window) {
  if (is_dock_window(dpy, window)) {
    XMapWindow(dpy, window);
    return;
  }

  TilingLayout *layout =
      &workspace_manager.layouts[workspace_manager.current_workspace];
  if (layout->count >= MAX_WINDOWS) return;

  WindowInfo new_window = {.window = window,
                           .x = 0,
                           .y = 0,
                           .width = 100,
                           .height = 100,
                           .border_width = BORDER_WIDTH,
                           .is_floating = 0};

  layout->windows[layout->count] = new_window;
  layout->count++;

  if (layout->master == None) {
    layout->master = window;
  }

  arrange_windows(dpy);
  XMapWindow(dpy, window);
}

void remove_window_from_current_workspace(Display *dpy, Window window) {
  TilingLayout *layout =
      &workspace_manager.layouts[workspace_manager.current_workspace];
  int i;
  for (i = 0; i < layout->count; i++) {
    if (layout->windows[i].window == window) {
      break;
    }
  }
  if (i == layout->count) return;  // Window not found

  // Shift remaining windows
  for (; i < layout->count - 1; i++) {
    layout->windows[i] = layout->windows[i + 1];
  }
  layout->count--;

  // Update master if necessary
  if (layout->master == window) {
    layout->master = (layout->count > 0) ? layout->windows[0].window : None;
  }

  arrange_windows(dpy);
}

void swap_master(Display *dpy) {
  TilingLayout *layout =
      &workspace_manager.layouts[workspace_manager.current_workspace];
  if (layout->count < 2) return;

  Window focused;
  int revert_to;
  XGetInputFocus(dpy, &focused, &revert_to);

  if (focused != layout->master) {
    for (int i = 0; i < layout->count; i++) {
      if (layout->windows[i].window == focused) {
        Window temp = layout->master;
        layout->master = focused;
        layout->windows[i].window = temp;
        break;
      }
    }
    arrange_windows(dpy);
  }
}

// Workspace functions
void move_window_to_workspace(Display *dpy, int target_workspace) {
  if (target_workspace < 0 || target_workspace >= MAX_WORKSPACES) return;

  Window focused;
  int revert_to;
  XGetInputFocus(dpy, &focused, &revert_to);

  TilingLayout *current_layout =
      &workspace_manager.layouts[workspace_manager.current_workspace];
  TilingLayout *target_layout = &workspace_manager.layouts[target_workspace];

  // Find and remove the window from the current workspace
  for (int i = 0; i < current_layout->count; i++) {
    if (current_layout->windows[i].window == focused) {
      // Remove from current workspace
      WindowInfo win_info = current_layout->windows[i];
      remove_window_from_current_workspace(dpy, focused);

      // Add to target workspace
      if (target_layout->count < MAX_WINDOWS) {
        target_layout->windows[target_layout->count] = win_info;
        target_layout->count++;
        if (target_layout->master == None) {
          target_layout->master = focused;
        }
      }

      XUnmapWindow(dpy, focused);
      break;
    }
  }

  arrange_windows(dpy);
}

void switch_workspace(Display *dpy, int new_workspace) {
  if (new_workspace < 0 || new_workspace >= MAX_WORKSPACES) return;
  if (workspace_manager.current_workspace == new_workspace) return;

  // Hide windows in current workspace
  TilingLayout *current_layout =
      &workspace_manager.layouts[workspace_manager.current_workspace];
  for (int i = 0; i < current_layout->count; i++) {
    XUnmapWindow(dpy, current_layout->windows[i].window);
  }

  // Switch to new workspace
  workspace_manager.current_workspace = new_workspace;

  // Show windows in new workspace
  TilingLayout *new_layout =
      &workspace_manager.layouts[workspace_manager.current_workspace];
  for (int i = 0; i < new_layout->count; i++) {
    XMapWindow(dpy, new_layout->windows[i].window);
  }

  // Update ewmh props
  set_current_desktop(dpy, RootWindow(dpy, DefaultScreen(dpy)), new_workspace);

  arrange_windows(dpy);
}

void setup_keybindings(Display *dpy, Window root) {
  // Grab key
  for (int i = 0; i < NUM_KEYBINDINGS; i++) {
    KeyCode keycode = XKeysymToKeycode(dpy, keybindings[i].keysym);
    XGrabKey(dpy, keycode, keybindings[i].modifier, root, True, GrabModeAsync,
             GrabModeAsync);
  }

  // Grab Moving and resizing keybinds
  XGrabButton(dpy, MOVE_BUTTON, MODIFIER, root, True, ButtonPressMask,
              GrabModeAsync, GrabModeAsync, None, None);
  XGrabButton(dpy, RESIZE_BUTTON, MODIFIER, root, True, ButtonPressMask,
              GrabModeAsync, GrabModeAsync, None, None);

  // Window keybindings
  XGrabKey(dpy, XKeysymToKeycode(dpy, KILL_KEY), MODIFIER, root, True,
           GrabModeAsync, GrabModeAsync);

  // Focus keybindings
  XGrabKey(dpy, XKeysymToKeycode(dpy, NEXT_WINDOW_KEY), MODIFIER, root, True,
           GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode(dpy, PREV_WINDOW_KEY), MODIFIER, root, True,
           GrabModeAsync, GrabModeAsync);
}

// Set Cursor font to avoid no cursor
void set_default_cursor(Display *dpy, Window root) {
  Cursor cursor;
  cursor = XCreateFontCursor(dpy, XC_left_ptr);
  XDefineCursor(dpy, root, cursor);
  XFlush(dpy);
}

// Map window
void handle_map_request(XEvent ev, Display *dpy) {
  XWindowAttributes attr;
  XGetWindowAttributes(dpy, ev.xmaprequest.window, &attr);

  if (attr.override_redirect) {
    printf("Override redirect, skipping window\n");
    return;
  }

  if (is_dock_window(dpy, ev.xmaprequest.window)) {
    printf("Dock window, skipping window\n");
    XMapWindow(dpy, ev.xmaprequest.window);
    update_dock_geometry(dpy, ev.xmaprequest.window);
    arrange_windows(dpy);
    return;
  }

  printf("Mapping window 0x%lx\n", ev.xmaprequest.window);
  XSelectInput(dpy, ev.xmaprequest.window,
               EnterWindowMask | FocusChangeMask | StructureNotifyMask);
  // Maps window and tiles it
  add_window_to_current_workspace(dpy, ev.xmaprequest.window);
}

void handle_unmap_request(XEvent ev, Display *dpy) {
  // Unmaps window and tiles everything else
  remove_window_from_current_workspace(dpy, ev.xunmap.window);

  focus_next_window(dpy);
}

void handle_configure_request(XEvent ev, Display *dpy) {
  XConfigureRequestEvent *req = &ev.xconfigurerequest;
  XWindowChanges changes;
  changes.x = req->x;
  changes.y = req->y;
  changes.width = req->width;
  changes.height = req->height;
  changes.border_width = req->border_width;
  changes.sibling = req->above;
  changes.stack_mode = req->detail;

  printf("Configure request: window 0x%lx, (%d, %d, %d, %d)\n", req->window,
         req->x, req->y, req->width, req->height);

  // Check if it's a dock or bar window
  if (is_dock_window(dpy, req->window)) {
    XConfigureWindow(dpy, req->window, req->value_mask, &changes);
    update_dock_geometry(dpy, req->window);
    arrange_windows(dpy);
    return;
  }

  // Check if it's a floating window
  bool is_floating = false;
  TilingLayout *current_layout =
      &workspace_manager.layouts[workspace_manager.current_workspace];
  for (int i = 0; i < current_layout->count; i++) {
    if (current_layout->windows[i].window == req->window) {
      is_floating = current_layout->windows[i].is_floating;
      break;
    }
  }

  if (is_floating) {
    // For floating windows, apply the configuration as requested
    XConfigureWindow(dpy, req->window, req->value_mask, &changes);
  } else {
    // For tiled windows, only apply size changes if they're smaller than
    // requested
    XWindowAttributes attr;
    XGetWindowAttributes(dpy, req->window, &attr);

    changes.width =
        (req->value_mask & CWWidth) ? MAX(attr.width, req->width) : attr.width;
    changes.height = (req->value_mask & CWHeight)
                         ? MAX(attr.height, req->height)
                         : attr.height;

    // Ignore position changes for tiled windows
    req->value_mask &= ~(CWX | CWY);

    XConfigureWindow(dpy, req->window, req->value_mask, &changes);
  }

  // Update the internal layout
  for (int i = 0; i < current_layout->count; i++) {
    if (current_layout->windows[i].window == req->window) {
      if (is_floating) {
        current_layout->windows[i].x = changes.x;
        current_layout->windows[i].y = changes.y;
      }
      current_layout->windows[i].width = changes.width;
      current_layout->windows[i].height = changes.height;
      break;
    }
  }

  // Re-arrange windows if necessary
  if (!is_floating) {
    arrange_windows(dpy);
  }

  XSync(dpy, False);
}

// Handle moving and resizing
void start_drag(Display *dpy, XEvent ev, DragState *drag) {
  if (ev.xbutton.subwindow != None) {
    drag->window = ev.xbutton.subwindow;
    drag->start_x = ev.xbutton.x_root;
    drag->start_y = ev.xbutton.y_root;

    XWindowAttributes attr;
    XGetWindowAttributes(dpy, drag->window, &attr);
    drag->x = attr.x;
    drag->y = attr.y;
    drag->width = attr.width;
    drag->height = attr.height;
    drag->is_resizing = (ev.xbutton.button == RESIZE_BUTTON);

    XGrabPointer(dpy, drag->window, True, PointerMotionMask | ButtonReleaseMask,
                 GrabModeAsync, GrabModeAsync, None, None, CurrentTime);

    printf("Starting %s on window 0x%lx\n",
           drag->is_resizing ? "resize" : "move", drag->window);
  }
}

void update_drag(Display *dpy, XEvent ev, DragState *drag) {
  if (drag->window != None) {
    int xdiff = ev.xmotion.x_root - drag->start_x;
    int ydiff = ev.xmotion.y_root - drag->start_y;

    if (drag->is_resizing) {
      XMoveResizeWindow(dpy, drag->window, drag->x, drag->y,
                        MAX(1, drag->width + xdiff),
                        MAX(1, drag->height + ydiff));
    } else {
      XMoveWindow(dpy, drag->window, drag->x + xdiff, drag->y + ydiff);
    }
  }
}

void end_drag(Display *dpy, DragState *drag) {
  if (drag->window != None) {
    XUngrabPointer(dpy, CurrentTime);
    drag->window = None;
    printf("Drag ended\n");
  }
}

void close_window(Display *dpy, Window window) {
  Atom wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  XEvent event;
  event.type = ClientMessage;
  event.xclient.window = window;
  event.xclient.message_type = XInternAtom(dpy, "WM_PROTOCOLS", True);
  event.xclient.format = 32;
  event.xclient.data.l[0] = wm_delete_window;
  event.xclient.data.l[1] = CurrentTime;
  XSendEvent(dpy, window, False, NoEventMask, &event);
}

void kill_focused_window(Display *dpy) {
  Window focused_window;
  int revert_to;

  // Get the currently focused window
  XGetInputFocus(dpy, &focused_window, &revert_to);

  if (focused_window != None && focused_window != PointerRoot) {
    close_window(dpy, focused_window);
    printf("Killed window 0x%lx\n", focused_window);
  } else {
    printf("No window is focused\n");
  }
}

void handle_keypress_event(XEvent ev, Display *dpy) {
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

  // Swap master
  if (keysym == SWAP_MASTER_KEY && (ev.xkey.state & MODIFIER)) {
    swap_master(dpy);
  }

  // Get all keybindings for programs
  for (int i = 0; i < NUM_KEYBINDINGS; i++) {
    if (keysym == keybindings[i].keysym &&
        (ev.xkey.state & keybindings[i].modifier)) {
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

void handle_client_message(XEvent *e, Display *dpy) {
  if (e->xclient.message_type == net_wm_state) {
    Window window = e->xclient.window;
    Atom state = (Atom)e->xclient.data.l[1];
    bool add = e->xclient.data.l[0] == 1;

    if (state == net_wm_state_fullscreen) {
      if (add) {
        XWindowChanges changes;

        changes.border_width = 0;

        // Make window fullscreen and remove border
        XMoveResizeWindow(dpy, window, 0, 0,
                          XDisplayWidth(dpy, DefaultScreen(dpy)),
                          XDisplayHeight(dpy, DefaultScreen(dpy)));
        XConfigureWindow(dpy, window, CWBorderWidth, &changes);
      } else {
        // Exit fullscreen
        draw_window_border(dpy, window, BORDER_WIDTH, BORDER_COLOR);
      }
    }
  }
  // Handle other client messages as needed
}

void handle_events(Display *dpy, Window root, int scr) {
  DragState drag = {0};
  XEvent ev;
  for (;;) {
    XNextEvent(dpy, &ev);

    switch (ev.type) {
      case MapRequest:
        printf("Map Request\n");
        handle_map_request(ev, dpy);
        focus_window(dpy, ev.xmaprequest.window);
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
          printf("Mouse entered window 0x%lx, raising and focusing it\n",
                 ev.xcrossing.window);

          focus_window(dpy, ev.xcrossing.window);
        }
        break;
      case ButtonPress:
        if (ev.xbutton.subwindow != None) {
          // Resizing and Moving
          if ((ev.xbutton.state & MODIFIER) &&
              (ev.xbutton.button == MOVE_BUTTON ||
               ev.xbutton.button == RESIZE_BUTTON)) {
            start_drag(dpy, ev, &drag);
          }
        }
        break;
      case MotionNotify:
        while (XCheckTypedEvent(dpy, MotionNotify, &ev))
          ;
        update_drag(dpy, ev, &drag);
        break;
      case ButtonRelease:
        end_drag(dpy, &drag);
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

int handle_x_error(Display *dpy, XErrorEvent *error_event) {
  if (error_event->error_code == BadAccess) {
    error_occurred = 1;
  }
  return 0;
}

int main() {
  Display *dpy;
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

  XSelectInput(dpy, root,
               SubstructureRedirectMask | SubstructureNotifyMask |
                   ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                   KeyPressMask | EnterWindowMask);

  XSync(dpy, False);

  if (error_occurred) {
    errx(1, "Another window manager is running");
  }

  printf("Opened display\n");

  // Launch startup commands
  system("/usr/bin/autostart.sh &");

  init_layouts();

  // EWMH
  init_ewmh(dpy, root);

  // Status bar
  dock_geometry.x = 0;
  dock_geometry.y = 0;
  dock_geometry.width = 0;
  dock_geometry.height = 0;

  setup_keybindings(dpy, root);
  set_default_cursor(dpy, root);

  handle_events(dpy, root, scr);

  XCloseDisplay(dpy);
}
