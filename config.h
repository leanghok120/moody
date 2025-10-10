#include "moody.h"

static int modkey = Mod1Mask;
static const unsigned long border_color = 0x45475a;
static const unsigned long border_color_active = 0x89b4fa;
static const unsigned int border_width = 2;

Key keys[] = {
  // mod      key        func   cmd   arg
  { Mod1Mask, XK_Return, spawn, "st", "" },
  { Mod1Mask, XK_space,  spawn, "dmenu_run", "" },
  { Mod1Mask, XK_q,      kill_client, "", "" },
};
