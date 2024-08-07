# MoodyWM

![moody screenshot](./screenshots/moody-bar.png)

## Overview

Moody is a very minimal, simple, and buggy tiling window manager written in C. Moody's goal is to be minimal and unusable.

### Features

- [x] Windows management (moving, resizing)
- [x] Windows tiling
- [x] Windows gaps
- [x] Windows navigation
- [x] Multiple workspaces
- [x] Windows decoration
- [x] Keybindings
- [x] Startup commands
- [x] Configurations
- [x] Focus on hover
- [x] Supports Bar (polybar)
- [ ] Fullscreen (buggy)
- [ ] Floating windows (buggy)

### Notes

- Codebase is very messy (Rewriting)

### Dependencies

- libx11
- xterm (default terminal)
- rofi (default app launcher)

1. Arch

```bash
sudo pacman -S libx11 libxrandr libxinerama libxcursor libxext libxt xorg-server-devel xterm rofi
```

2. Ubuntu/Debian

```bash
sudo apt-get install -y libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxext-dev libxt-dev xorg-dev xterm
```

3. Fedora/Red Hat

```bash
sudo dnf install -y libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXext-devel libXt-devel xorg-x11-server-devel xterm
```

### Installation

clone the repo and build moody from source

```bash
git clone https://github.com/leanghok120/moody.git
cd moody
rm -rf .git
```

make sure you're in the moody directory and run:

```bash
sudo make clean build install
```

### Usage

Moody is configured in pure C, although this may sound scary, the `config.h` file is super simple to understand. After configuring everything u need, just compile everything with `sudo make build install` and restart moody.

### Configuration

#### Startup commands

Startup commands are commands that launch when moody starts up, these could be commands to set a wallpaper, open a program and more.

To set startup commands, just put the commands in autostart.sh and moody will run autostart.sh on startup. Example:

```bash
# autostart.sh

nitrogen --restore
xterm
```

This will set your wallpaper (using nitrogen) and open up xterm on moody's startup

#### Keybindings

You can configure keybindings in the config.h file.
There are 2 types of keybindings in moody, keybindings to run commands and keybindings to interact with moody.

##### Command keybindings

A command keybinding is a keybinding that executes a command when triggered, and this is how you would configure it.

```c
static Keybinding keybindings[] = {
  { XK_Return, MODIFIER, "xterm", -1 },           // mod+return to open xterm (terminal)
  { XK_B, MODIFIER, "firefox", -1 },              // mod+b to open firefox
  { XK_space, MODIFIER, "rofi -show drun", -1 },  // mod+space to open rofi (app launcher)

  { XK_F12, MODIFIER, "pactl set-sink-volume @DEFAULT_SINK@ +5%" },   // mod+f12 to increase volume by 5%
  { XK_F11, MODIFIER, "pactl set-sink-volume @DEFAULT_SINK@ -5%" },   // mod+f11 to increase volume by 5%
  { XK_F10, MODIFIER, "pactl set-sink-mute @DEFAULT_SINK@ toggle" },  // mod+f10 to mute

  // Workspace
  { XK_1, MODIFIER, NULL, 0 },                     // mod+1 to switch to workspace 1
  { XK_2, MODIFIER, NULL, 1 },                     // mod+2 to switch to workspace 2
  { XK_3, MODIFIER, NULL, 2 },                     // mod+3 to switch to workspace 3
  { XK_4, MODIFIER, NULL, 3 },                     // mod+4 to switch to workspace 4
  { XK_5, MODIFIER, NULL, 4 },                     // mod+5 to switch to workspace 5
  { XK_6, MODIFIER, NULL, 5 },                     // mod+6 to switch to workspace 6
  { XK_7, MODIFIER, NULL, 6 },                     // mod+7 to switch to workspace 7
  { XK_8, MODIFIER, NULL, 7 },                     // mod+8 to switch to workspace 8
  { XK_9, MODIFIER, NULL, 8 }                      // mod+9 to switch to workspace 9
};
```

##### Moody keybindings

Moody keybindings are keybindings that interact with moody, such as killing windows, focusing windows. This is how you would configure them:

```c
#define KILL_KEY XK_q // mod+q for killing the current window
#define NEXT_WINDOW_KEY XK_k // mod+k to focus next window
#define PREV_WINDOW_KEY XK_j // mod+j to focus previous window
```

### Inspiration

I got this idea of creating my own tiling windows manager in a dream. After I woke up, I decided to create moody since I had no projects to work on.
moody is inspired by dwm and ragnarwm
