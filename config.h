#include <X11/X.h>
#include <X11/keysym.h>

#ifndef CONFIG_H
#define CONFIG_H

// Modifier keys
#define MODIFIER Mod1Mask // Alt key, change to Mod4Mask for Super key (Windows key)

// Other settings
#define MOVE_BUTTON Button1
#define RESIZE_BUTTON Button3

// Moody Keybindings

// Close current window
#define KILL_KEY XK_q // sets "q" as the letter for killing the current window

#endif
