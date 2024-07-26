#include "config.h"

typedef struct {
  Window window;
  int start_x, start_y;
  int x, y;
  int width, height;
  int is_resizing;
} DragState;

typedef struct {
  Window window;
  int x, y;
  int width, height;
  int border_width;
  int is_floating;
} WindowInfo;

typedef struct {
  WindowInfo windows[MAX_WINDOWS];
  int count;
  Window master; // Store the master window
} TilingLayout;

typedef struct {
  TilingLayout layouts[MAX_WORKSPACES];
  int current_workspace;
} WorkspaceManager;

typedef struct {
  int x, y;
  unsigned int width, height;
} DockGeometry;
