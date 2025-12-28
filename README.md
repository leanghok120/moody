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
Exec=moody
Type=Application
```

## Keybindings

| keys                      | action                 |
| ------------------------- | -----------------------|
| `Alt` + `enter`           | open st (terminal)     |
| `Alt` + `space`           | open cmenu (launcher)  |
| `Alt` + `q`               | kill window            |
| `Alt` + `1-9`             | swap workspaces        |
| `Alt` + `Shift` +`1-9`    | send to workspace      |
| `Alt` + `j`               | focus cycle            |

## Configuration

You can configure moody using the `config.h` file which is similar to dwm. Just make sure to recompile after editting `config.h`.

## Thanks

- [dwm](https://dwm.suckless.org)
- [sowm](https://github.com/dylanaraps/sowm)
