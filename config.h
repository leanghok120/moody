#include "moody.h"

#define MODKEY Mod1Mask

const char* term[] = { "st", NULL };
const char* launcher[] = { "cmenu-run", NULL };

Key keys[] = {
  { MODKEY, XK_Return, spawn, {.cmd = term} },
  { MODKEY, XK_space, spawn, {.cmd = launcher} },
  { MODKEY, XK_q, kill, {0} },
  { MODKEY, XK_j, focusnext, {0} },

  { MODKEY, XK_1, switchws, {.i = 1} },
  { MODKEY, XK_2, switchws, {.i = 2} },
  { MODKEY, XK_3, switchws, {.i = 3} },
  { MODKEY, XK_4, switchws, {.i = 4} },
  { MODKEY, XK_5, switchws, {.i = 5} },
  { MODKEY, XK_6, switchws, {.i = 6} },
  { MODKEY, XK_7, switchws, {.i = 7} },
  { MODKEY, XK_8, switchws, {.i = 8} },
  { MODKEY, XK_9, switchws, {.i = 9} },

  { MODKEY | ShiftMask, XK_1, sendws, {.i = 1} },
  { MODKEY | ShiftMask, XK_2, sendws, {.i = 2} },
  { MODKEY | ShiftMask, XK_3, sendws, {.i = 3} },
  { MODKEY | ShiftMask, XK_4, sendws, {.i = 4} },
  { MODKEY | ShiftMask, XK_5, sendws, {.i = 5} },
  { MODKEY | ShiftMask, XK_6, sendws, {.i = 6} },
  { MODKEY | ShiftMask, XK_7, sendws, {.i = 7} },
  { MODKEY | ShiftMask, XK_8, sendws, {.i = 8} },
  { MODKEY | ShiftMask, XK_9, sendws, {.i = 9} },
};
