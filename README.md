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

1. Clone this repo

```bash
git clone git@github.com:leanghok120/moody.git
cd moody
```

2. Install moody

make sure you're in the moody directory and run:

```bash
sudo make clean build install
```

### Inspiration

I got this idea of creating my own tiling windows manager in a dream. After I woke up, I decided to create moody since I had no projects to work on.
moody is inspired by dwm's minimalism and i3's simplicity
