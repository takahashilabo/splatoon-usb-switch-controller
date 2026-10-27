#pragma once

#include "gamepad_state.h"

// Reimplementation (for the Pico, in C) of the dummy-play state machine from
// ../../pseude-splatoon-controller/core/human_splatoon_ai.py: weighted
// random transitions between a handful of "what a human is doing right now"
// actions, each with its own stick/button targets, blended frame-to-frame
// with inertia (lerp) + small noise so the input doesn't look robotic.

typedef enum {
  PLAYSTYLE_BALANCED,
  PLAYSTYLE_STEALTH,
  PLAYSTYLE_AGGRESSIVE,
} PlayStyle;

typedef enum {
  ACTION_PAINT_ADVANCE,
  ACTION_SWIM_DASH,
  ACTION_SEARCH_LOOK,
  ACTION_RECOVER_INK,
  ACTION_COMBAT_EVADE,
  ACTION_COUNT,
} ActionState;

typedef struct {
  PlayStyle style;
  ActionState current_action;
  float action_timer;
  float action_duration;

  float target_lx, target_ly, target_rx, target_ry;

  ProConInputState state;
} SplatoonAI;

void splatoon_ai_init(SplatoonAI *ai, PlayStyle style);

// dt in seconds since the last call. Fills *out with this frame's
// button/stick state.
void splatoon_ai_update(SplatoonAI *ai, float dt, ProConInputState *out);
