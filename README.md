# MoodyWM

## Overview

Moody is a minimal and cozy tiling windows manager written in C. Moody's goal is to be blazingly fast, very minimal but still easy for new users to pick it up.

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
- [ ] EWMH (once ported xcb)

### Dependencies

1. Arch

```bash
sudo pacman -S libx11 libxrandr libxinerama libxcursor libxext libxt xorg-server-devel
```

2. Ubuntu/Debian

```bash
sudo apt-get install -y libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxext-dev libxt-dev xorg-dev
```

3. Fedora/Red Hat

```bash
sudo dnf install -y libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXext-devel libXt-devel xorg-x11-server-devel
```

### Installation

#### 1. Clone this repo

```bash
git clone git@github.com:leanghok120/moody.git
cd moody
```

#### 2. Install moody

make sure you're in the moody directory and run:

```bash
sudo make clean build install
```

#### 3. Start moody

#### startx

add this into your .xinitrc (no need to include startup commands)
```bash
moody
```

#### display manager

you can also start moody with your display manager by adding a moody.desktop to /usr/share/xsessions/
the contents of moody.desktop should look like this:

```bash
[Desktop Entry]
Encoding=UTF-8
Name=moody
Comment=Cozy tiling window manager
Exec=moody
Type=XSession
```

Now, your display manager should show moody as a session and you can log into it

### Inspiration

I got this idea of creating my own tiling windows manager in a dream. After I woke up, I decided to create moody since I had no projects to work on.
moody is inspired by dwm's minimalism and i3's simplicity
