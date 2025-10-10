#include "moody.h"
#include <X11/X.h>

static int modkey = Mod1Mask;
static const unsigned long border_color = 0x45475a;
static const unsigned long border_color_active = 0x89b4fa;
static const unsigned int border_width = 2;

Key keys[] = {
  // mod      key        func               cmd         arg
  { Mod1Mask, XK_Return, spawn,            "st",        "" },
  { Mod1Mask, XK_space,  spawn,            "dmenu_run", "" },
  { Mod1Mask, XK_q,      kill_client,      "",          "" },

  { Mod1Mask, XK_1,      switch_workspace, "1",         "" },
  { Mod1Mask, XK_2,      switch_workspace, "2",         "" },
  { Mod1Mask, XK_3,      switch_workspace, "3",         "" },
  { Mod1Mask, XK_4,      switch_workspace, "4",         "" },
  { Mod1Mask, XK_5,      switch_workspace, "5",         "" },
  { Mod1Mask, XK_6,      switch_workspace, "6",         "" },
  { Mod1Mask, XK_7,      switch_workspace, "7",         "" },
  { Mod1Mask, XK_8,      switch_workspace, "8",         "" },
  { Mod1Mask, XK_9,      switch_workspace, "9",         "" },
};
