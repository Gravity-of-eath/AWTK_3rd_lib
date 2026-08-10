#include "jump_label.h"

float_t jump_label_ease_out_bounce(float_t t) {
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;

  if (t < 1.0f / 2.75f) {
    return 7.5625f * t * t;
  } else if (t < 2.0f / 2.75f) {
    t -= 1.5f / 2.75f;
    return 7.5625f * t * t + 0.75f;
  } else if (t < 2.5f / 2.75f) {
    t -= 2.25f / 2.75f;
    return 7.5625f * t * t + 0.9375f;
  } else {
    t -= 2.625f / 2.75f;
    return 7.5625f * t * t + 0.984375f;
  }
}
