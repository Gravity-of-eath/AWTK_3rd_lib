#include "breath_ellipse_view.h"

#include <math.h>

#include "tkc/utils.h"

static float_t breath_ellipse_view_clamp_scale(float_t value) {
  if (value < 0.1f) {
    return 0.1f;
  }
  if (value > 3.0f) {
    return 3.0f;
  }
  return value;
}

uint32_t breath_ellipse_view_effective_duration_ms(float_t frequency_bpm, uint32_t duration_ms) {
  if (duration_ms > 0) {
    return duration_ms;
  }
  if (frequency_bpm <= 0.0f) {
    return 1000;
  }
  return tk_max(200, (uint32_t)(60000.0f / frequency_bpm));
}

float_t breath_ellipse_view_eval_scale(float_t min_scale, float_t max_scale, float_t phase) {
  float_t p = phase - floorf(phase);
  float_t min_v = breath_ellipse_view_clamp_scale(min_scale);
  float_t max_v = breath_ellipse_view_clamp_scale(max_scale);
  if (max_v < min_v) {
    float_t tmp = min_v;
    min_v = max_v;
    max_v = tmp;
  }
  {
    float_t midpoint = (min_v + max_v) * 0.5f;
    float_t amplitude = (max_v - min_v) * 0.5f;
    float_t rad = p * 2.0f * (float_t)M_PI;
    return midpoint - amplitude * cosf(rad);
  }
}

color_t breath_ellipse_view_eval_color(const color_t* colors, uint32_t color_count, float_t phase) {
  color_t color = {0};
  float_t p = 0.0f;
  float_t scaled = 0.0f;
  uint32_t index = 0;
  uint32_t next_index = 0;
  float_t t = 0.0f;
  const color_t* c0 = NULL;
  const color_t* c1 = NULL;

  color.rgba.a = 0xff;
  if (colors == NULL || color_count == 0) {
    return color;
  }
  if (color_count == 1) {
    return colors[0];
  }

  p = phase - floorf(phase);
  if (p < 0.0f) {
    p += 1.0f;
  }

  scaled = p * (float_t)color_count;
  index = (uint32_t)scaled;
  if (index >= color_count) {
    index = 0;
  }
  next_index = (index + 1) % color_count;
  t = scaled - (float_t)index;

  c0 = colors + index;
  c1 = colors + next_index;

  color.rgba.r = (uint8_t)((float_t)c0->rgba.r + ((float_t)c1->rgba.r - (float_t)c0->rgba.r) * t + 0.5f);
  color.rgba.g = (uint8_t)((float_t)c0->rgba.g + ((float_t)c1->rgba.g - (float_t)c0->rgba.g) * t + 0.5f);
  color.rgba.b = (uint8_t)((float_t)c0->rgba.b + ((float_t)c1->rgba.b - (float_t)c0->rgba.b) * t + 0.5f);
  color.rgba.a = (uint8_t)((float_t)c0->rgba.a + ((float_t)c1->rgba.a - (float_t)c0->rgba.a) * t + 0.5f);

  return color;
}

color_t breath_ellipse_view_eval_color_near_min(const color_t* colors, uint32_t color_count,
                                                uint64_t cycle, float_t phase, float_t window) {
  color_t color = {0};
  uint32_t index = 0;
  uint32_t next_index = 0;
  float_t p = phase - floorf(phase);
  float_t blend_start = 0.0f;
  float_t t = 0.0f;
  const color_t* c0 = NULL;
  const color_t* c1 = NULL;

  color.rgba.a = 0xff;
  if (colors == NULL || color_count == 0) {
    return color;
  }
  if (color_count == 1) {
    return colors[0];
  }

  if (p < 0.0f) {
    p += 1.0f;
  }

  index = (uint32_t)(cycle % (uint64_t)color_count);
  next_index = (index + 1) % color_count;

  window = tk_clamp(window, 0.02f, 0.45f);
  blend_start = 1.0f - window;

  c0 = colors + index;
  c1 = colors + next_index;
  if (p <= blend_start) {
    return *c0;
  }

  t = (p - blend_start) / window;
  color.rgba.r = (uint8_t)((float_t)c0->rgba.r + ((float_t)c1->rgba.r - (float_t)c0->rgba.r) * t + 0.5f);
  color.rgba.g = (uint8_t)((float_t)c0->rgba.g + ((float_t)c1->rgba.g - (float_t)c0->rgba.g) * t + 0.5f);
  color.rgba.b = (uint8_t)((float_t)c0->rgba.b + ((float_t)c1->rgba.b - (float_t)c0->rgba.b) * t + 0.5f);
  color.rgba.a = (uint8_t)((float_t)c0->rgba.a + ((float_t)c1->rgba.a - (float_t)c0->rgba.a) * t + 0.5f);

  return color;
}

float_t breath_ellipse_view_calc_fps(uint32_t frame_count, uint32_t elapsed_ms) {
  if (elapsed_ms == 0) {
    return 0.0f;
  }
  return ((float_t)frame_count * 1000.0f) / (float_t)elapsed_ms;
}
