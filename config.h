#include "moody.h"
#include <X11/X.h>

#define MODKEY Mod1Mask
static const unsigned long border_color = 0x45475a;
static const unsigned long border_color_active = 0x89b4fa;
static const unsigned int border_width = 2;

Key keys[] = {
  // mod      key        func               cmd         arg
  { MODKEY, XK_Return, spawn,            "st",        "" },
  { MODKEY, XK_space,  spawn,            "dmenu_run", "" },
  { MODKEY, XK_F11,    spawn,            "pactl",     "set-sink-volume @DEFAULT_SINK@ -10%" },
  { MODKEY, XK_F12,    spawn,            "pactl",     "set-sink-volume @DEFAULT_SINK@ +10%" },
  { MODKEY, XK_q,      kill_client,      "",          "" },
  { MODKEY, XK_j,      focus_next,       "",          "" },

  { MODKEY, XK_1,      switch_workspace, "1",         "" },
  { MODKEY, XK_2,      switch_workspace, "2",         "" },
  { MODKEY, XK_3,      switch_workspace, "3",         "" },
  { MODKEY, XK_4,      switch_workspace, "4",         "" },
  { MODKEY, XK_5,      switch_workspace, "5",         "" },
  { MODKEY, XK_6,      switch_workspace, "6",         "" },
  { MODKEY, XK_7,      switch_workspace, "7",         "" },
  { MODKEY, XK_8,      switch_workspace, "8",         "" },
  { MODKEY, XK_9,      switch_workspace, "9",         "" },

  { MODKEY | ShiftMask, XK_1,      move_to_workspace, "1", "" },
  { MODKEY | ShiftMask, XK_2,      move_to_workspace, "2", "" },
  { MODKEY | ShiftMask, XK_3,      move_to_workspace, "3", "" },
  { MODKEY | ShiftMask, XK_4,      move_to_workspace, "4", "" },
  { MODKEY | ShiftMask, XK_5,      move_to_workspace, "5", "" },
  { MODKEY | ShiftMask, XK_6,      move_to_workspace, "6", "" },
  { MODKEY | ShiftMask, XK_7,      move_to_workspace, "7", "" },
  { MODKEY | ShiftMask, XK_8,      move_to_workspace, "8", "" },
  { MODKEY | ShiftMask, XK_9,      move_to_workspace, "9", "" },
};
