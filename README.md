# moody

A tiny tiling windows manager (*300 sloc*)

- Tiling only
- Workspaces
- Keyboard only workflow
- Alt-Tab window focusing
- No window borders
- No ICCCM
- No EWMH

## Dependencies

- libX11

## Installation

- Clone this repo
- Run `sudo make clean install`

If you are using a display manager, save the following to `/usr/share/xsessions/moody.desktop`

```
[Desktop Entry]
Name=moody
Comment=This session runs sowm as desktop manager
Exec=sowm
Type=Application
```

## Thanks

- [dwm](https://dwm.suckless.org)
- [sowm](https://github.com/dylanaraps/sowm)
