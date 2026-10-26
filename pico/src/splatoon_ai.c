#include "splatoon_ai.h"

#include <math.h>
#include <stdlib.h>

#include "pico/rand.h"

static float frand_uniform(float lo, float hi) {
  uint32_t r = get_rand_32();
  float t = (float)r / (float)UINT32_MAX;
  return lo + t * (hi - lo);
}

// Box-Muller, same role as Python's random.gauss().
static float frand_gauss(float mean, float stddev) {
  float u1 = frand_uniform(1e-6f, 1.0f);
  float u2 = frand_uniform(0.0f, 1.0f);
  float z = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * (float)M_PI * u2);
  return mean + z * stddev;
}

static float lerp(float cur, float target, float factor) { return cur + (target - cur) * factor; }

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Picks an index with probability proportional to weights[n].
static int choose_weighted(const float *weights, int n) {
  float total = 0;
  for (int i = 0; i < n; i++) total += weights[i];
  float r = frand_uniform(0, total);
  float acc = 0;
  for (int i = 0; i < n; i++) {
    acc += weights[i];
    if (r <= acc) return i;
  }
  return n - 1;
}

static void init_action_targets(SplatoonAI *ai, ActionState action) {
  ProConButtons *btn = &ai->state.buttons;
  *btn = (ProConButtons){0};

  switch (action) {
  case ACTION_PAINT_ADVANCE:
    ai->target_ly = frand_uniform(70.0f, 100.0f);
    ai->target_lx = frand_uniform(-25.0f, 25.0f);
    ai->target_rx = frand_uniform(-40.0f, 40.0f);
    ai->target_ry = 0.0f;
    btn->zr = true; // shoot
    break;

  case ACTION_SWIM_DASH: {
    float angle = frand_uniform(-(float)M_PI / 3.0f, (float)M_PI / 3.0f);
    ai->target_lx = sinf(angle) * 100.0f;
    ai->target_ly = cosf(angle) * 100.0f;
    ai->target_rx = 0.0f;
    ai->target_ry = 0.0f;
    btn->zl = true; // squid form
    if (frand_uniform(0, 1) < 0.25f) btn->b = true;
    break;
  }

  case ACTION_SEARCH_LOOK:
    ai->target_lx = frand_uniform(-20.0f, 20.0f);
    ai->target_ly = frand_uniform(-10.0f, 20.0f);
    ai->target_rx = (frand_uniform(0, 1) < 0.5f ? -1.0f : 1.0f) * frand_uniform(40.0f, 80.0f);
    ai->target_ry = frand_uniform(-10.0f, 10.0f);
    if (frand_uniform(0, 1) < 0.4f) btn->zl = true;
    break;

  case ACTION_RECOVER_INK:
    ai->target_lx = frand_uniform(-5.0f, 5.0f);
    ai->target_ly = frand_uniform(-5.0f, 5.0f);
    ai->target_rx = 0.0f;
    ai->target_ry = 0.0f;
    btn->zl = true;
    break;

  case ACTION_COMBAT_EVADE:
    ai->target_lx = (frand_uniform(0, 1) < 0.5f ? -1.0f : 1.0f) * frand_uniform(60.0f, 100.0f);
    ai->target_ly = frand_uniform(0.0f, 40.0f);
    ai->target_rx = frand_uniform(-30.0f, 30.0f);
    btn->zr = true;
    btn->b = true;
    break;

  default:
    break;
  }
}

static void choose_next_action(SplatoonAI *ai) {
  float weights[ACTION_COUNT];
  switch (ai->style) {
  case PLAYSTYLE_STEALTH:
    weights[ACTION_PAINT_ADVANCE] = 20;
    weights[ACTION_SWIM_DASH] = 25;
    weights[ACTION_SEARCH_LOOK] = 25;
    weights[ACTION_RECOVER_INK] = 25;
    weights[ACTION_COMBAT_EVADE] = 5;
    break;
  case PLAYSTYLE_AGGRESSIVE:
    weights[ACTION_PAINT_ADVANCE] = 35;
    weights[ACTION_SWIM_DASH] = 25;
    weights[ACTION_SEARCH_LOOK] = 10;
    weights[ACTION_RECOVER_INK] = 10;
    weights[ACTION_COMBAT_EVADE] = 20;
    break;
  default: // BALANCED
    weights[ACTION_PAINT_ADVANCE] = 30;
    weights[ACTION_SWIM_DASH] = 30;
    weights[ACTION_SEARCH_LOOK] = 15;
    weights[ACTION_RECOVER_INK] = 15;
    weights[ACTION_COMBAT_EVADE] = 10;
    break;
  }

  // Natural continuity between actions (same bias as human_splatoon_ai.py).
  ActionState prev = ai->current_action;
  if (prev == ACTION_PAINT_ADVANCE) {
    weights[ACTION_SWIM_DASH] *= 1.8f;
  } else if (prev == ACTION_SWIM_DASH) {
    weights[ACTION_SEARCH_LOOK] *= 1.5f;
    weights[ACTION_PAINT_ADVANCE] *= 1.4f;
  }

  ActionState next = (ActionState)choose_weighted(weights, ACTION_COUNT);
  ai->current_action = next;

  float mean, std;
  switch (next) {
  case ACTION_PAINT_ADVANCE: mean = 2.2f; std = 0.4f; break;
  case ACTION_SWIM_DASH: mean = 1.8f; std = 0.3f; break;
  case ACTION_SEARCH_LOOK: mean = 1.2f; std = 0.25f; break;
  case ACTION_RECOVER_INK: mean = 1.6f; std = 0.35f; break;
  case ACTION_COMBAT_EVADE: mean = 1.7f; std = 0.3f; break;
  default: mean = 1.5f; std = 0.3f; break;
  }
  ai->action_duration = fmaxf(0.5f, frand_gauss(mean, std));
  ai->action_timer = 0.0f;

  init_action_targets(ai, next);
}

void splatoon_ai_init(SplatoonAI *ai, PlayStyle style) {
  *ai = (SplatoonAI){0};
  ai->style = style;
  ai->current_action = ACTION_SEARCH_LOOK;
  choose_next_action(ai);
}

void splatoon_ai_update(SplatoonAI *ai, float dt, ProConInputState *out) {
  ai->action_timer += dt;
  if (ai->action_timer >= ai->action_duration) {
    choose_next_action(ai);
  }

  ActionState action = ai->current_action;

  if (action == ACTION_PAINT_ADVANCE) {
    ai->target_rx = sinf(ai->action_timer * 4.5f) * 35.0f;
  }
  if (action == ACTION_COMBAT_EVADE) {
    bool jump_active = (((int)(ai->action_timer * 3.5f)) % 2) == 0;
    ai->state.buttons.b = jump_active;
  }

  float noise_lx = frand_gauss(0, 1.5f);
  float noise_ly = frand_gauss(0, 1.5f);
  float noise_rx = frand_gauss(0, 2.0f);

  const float lerp_rate = 0.22f;
  ai->state.lx = clampf(lerp(ai->state.lx, ai->target_lx + noise_lx, lerp_rate), -100, 100);
  ai->state.ly = clampf(lerp(ai->state.ly, ai->target_ly + noise_ly, lerp_rate), -100, 100);
  ai->state.rx = clampf(lerp(ai->state.rx, ai->target_rx + noise_rx, lerp_rate), -100, 100);
  ai->state.ry = clampf(lerp(ai->state.ry, ai->target_ry, lerp_rate), -100, 100);

  *out = ai->state;
}
