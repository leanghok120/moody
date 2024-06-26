#include <X11/X.h>
#include <X11/keysym.h>
#include <time.h>

#ifndef CONFIG_H
#define CONFIG_H


// Moody Settings
#define WM_NAME "moody" // Set wm name for neofetch to use
#define MAX_WORKSPACES 9

// Modifier keys
#define MODIFIER Mod1Mask // Mod1Mask = alt, Mod4Mask = Super key(Windows key)

// Moving and Resizing buttons
#define MOVE_BUTTON Button1 // left mouse button
#define RESIZE_BUTTON Button3 // right mouse button

// Windows
#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT 800
#define MAX_WINDOWS 500 // Set max windows
#define BORDER_WIDTH 4
#define BORDER_COLOR "#000000" // Set border color to white

// Keybindings
typedef struct {
  KeySym keysym;
  unsigned int modifier;
  const char *command; // Command to execute, NULL for workspace switching
  int workspace; // Workspace number, -1  for now workspace change
} Keybinding;

#define KILL_KEY XK_q // mod+q for killing the current window

static Keybinding keybindings[] = {
{ XK_Return, MODIFIER, "xterm", -1 },           // mod+return to open xterm (terminal)
{ XK_B, MODIFIER, "firefox", -1 },              // mod+b to open firefox
{ XK_space, MODIFIER, "rofi -show drun", -1 },  // mod+space to open rofi (app launcher)

  // Workspace
  { XK_1, MODIFIER, NULL, 1 },                     // mod+1 to switch to workspace 1
  { XK_2, MODIFIER, NULL, 2 },                     // mod+2 to switch to workspace 2
  { XK_3, MODIFIER, NULL, 3 },                     // mod+3 to switch to workspace 3
  { XK_4, MODIFIER, NULL, 4 },                     // mod+4 to switch to workspace 4
  { XK_5, MODIFIER, NULL, 5 },                     // mod+5 to switch to workspace 5
  { XK_6, MODIFIER, NULL, 6 },                     // mod+6 to switch to workspace 6
  { XK_7, MODIFIER, NULL, 7 },                     // mod+7 to switch to workspace 7
  { XK_8, MODIFIER, NULL, 8 },                     // mod+8 to switch to workspace 8
  { XK_9, MODIFIER, NULL, 9 }                      // mod+9 to switch to workspace 9
};

#define NUM_KEYBINDINGS (sizeof(keybindings) / sizeof(Keybinding))

#endif
