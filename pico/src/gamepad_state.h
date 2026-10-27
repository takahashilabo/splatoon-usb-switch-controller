#pragma once

#include <stdbool.h>

// Abstract button/stick state, independent of whatever wire protocol
// actually gets sent over USB (see hori_gamepad.h for that). Kept separate
// so splatoon_ai.c doesn't need to know or care which protocol is in use.
typedef struct {
  bool y, x, b, a;
  bool right_sr, right_sl;
  bool r, zr;
  bool minus, plus;
  bool rclick, lclick;
  bool home, capture;
  bool dpad_up, dpad_down, dpad_left, dpad_right;
  bool left_sr, left_sl;
  bool l, zl;
} ProConButtons;

typedef struct {
  ProConButtons buttons;
  // -100.0 .. +100.0, 0 = centered.
  float lx, ly, rx, ry;
} ProConInputState;
