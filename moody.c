#include "src/wm.h"

int main() {
  if (!init_x()) {
    return 1;
  }

  run_event_loop();
  cleanup_x();
  return 0;
}
