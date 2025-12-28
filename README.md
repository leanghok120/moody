# moody

A tiny tiling windows manager (*300 sloc*)

- Tiling only
- Workspaces
- No gaps
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

## Configuration

You can configure moody using the `config.h` file which is similar to dwm. Just make sure to recompile after editting `config.h`.

## Thanks

- [dwm](https://dwm.suckless.org)
- [sowm](https://github.com/dylanaraps/sowm)
