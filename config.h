#include <X11/X.h>
#include <X11/keysym.h>

#ifndef CONFIG_H
#define CONFIG_H


// Moody Settings

// Modifier keys
#define MODIFIER Mod1Mask // Alt key, change to Mod4Mask for Super key (Windows key)

// Moving and Resizing buttons
#define MOVE_BUTTON Button1 // left mouse button
#define RESIZE_BUTTON Button3 // right mouse button

// Windows
#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT 800


// Moody Keybindings

// Close current window
#define KILL_KEY XK_q // sets "q" as the letter for killing the current window

#endif
