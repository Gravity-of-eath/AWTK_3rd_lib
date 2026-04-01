#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "breath_ellipse_view.h"

static int nearly_equal(float a, float b, float eps) {
  return fabsf(a - b) <= eps;
}

static color_t make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  color_t c = {0};
  c.rgba.r = r;
  c.rgba.g = g;
  c.rgba.b = b;
  c.rgba.a = a;
  return c;
}

int main(void) {
  assert(breath_ellipse_view_effective_duration_ms(12.0f, 0) == 5000);
  assert(breath_ellipse_view_effective_duration_ms(0.0f, 0) == 1000);
  assert(breath_ellipse_view_effective_duration_ms(1000.0f, 0) == 200);
  assert(breath_ellipse_view_effective_duration_ms(12.0f, 3200) == 3200);

  assert(nearly_equal(breath_ellipse_view_eval_scale(0.8f, 1.2f, 0.0f), 0.8f, 1e-4f));
  assert(nearly_equal(breath_ellipse_view_eval_scale(0.8f, 1.2f, 0.5f), 1.2f, 1e-4f));
  assert(nearly_equal(breath_ellipse_view_eval_scale(1.2f, 0.8f, 0.0f), 0.8f, 1e-4f));
  assert(nearly_equal(breath_ellipse_view_eval_scale(-10.0f, 10.0f, 0.5f), 3.0f, 1e-4f));
  assert(nearly_equal(breath_ellipse_view_eval_scale(-10.0f, 10.0f, 0.0f), 0.1f, 1e-4f));

  assert(nearly_equal(breath_ellipse_view_calc_fps(60, 1000), 60.0f, 1e-4f));
  assert(nearly_equal(breath_ellipse_view_calc_fps(120, 2000), 60.0f, 1e-4f));
  assert(nearly_equal(breath_ellipse_view_calc_fps(1, 0), 0.0f, 1e-4f));

  {
    color_t sequence[3];
    color_t c0;
    color_t c1;
    color_t c2;
    color_t c3;
    sequence[0] = make_color(255, 0, 0, 255);
    sequence[1] = make_color(0, 255, 0, 255);
    sequence[2] = make_color(0, 0, 255, 255);

    c0 = breath_ellipse_view_eval_color(sequence, 3, 0.0f);
    c1 = breath_ellipse_view_eval_color(sequence, 3, 1.0f / 3.0f);
    c2 = breath_ellipse_view_eval_color(sequence, 3, 2.0f / 3.0f);
    c3 = breath_ellipse_view_eval_color(sequence, 3, 5.0f / 6.0f);

    assert(c0.rgba.r == 255 && c0.rgba.g == 0 && c0.rgba.b == 0);
    assert(c1.rgba.r == 0 && c1.rgba.g == 255 && c1.rgba.b == 0);
    assert(c2.rgba.r == 0 && c2.rgba.g == 0 && c2.rgba.b == 255);
    assert(c3.rgba.r == 128 && c3.rgba.g == 0 && c3.rgba.b == 128);
  }

  {
    color_t sequence[3];
    color_t c_hold;
    color_t c_blend;
    color_t c_next;
    sequence[0] = make_color(255, 0, 0, 255);
    sequence[1] = make_color(0, 255, 0, 255);
    sequence[2] = make_color(0, 0, 255, 255);

    c_hold = breath_ellipse_view_eval_color_near_min(sequence, 3, 0, 0.4f, 0.2f);
    c_blend = breath_ellipse_view_eval_color_near_min(sequence, 3, 0, 0.9f, 0.2f);
    c_next = breath_ellipse_view_eval_color_near_min(sequence, 3, 1, 0.1f, 0.2f);

    assert(c_hold.rgba.r == 255 && c_hold.rgba.g == 0 && c_hold.rgba.b == 0);
    assert(c_blend.rgba.r + c_blend.rgba.g == 255 && c_blend.rgba.b == 0);
    assert(c_next.rgba.r == 0 && c_next.rgba.g == 255 && c_next.rgba.b == 0);
  }

  {
    uint32_t i = 0;
    float total = 0.0f;
    for (i = 0; i < 100000; i++) {
      total += breath_ellipse_view_eval_scale(0.8f, 1.2f, (float)i / 1000.0f);
    }
    assert(total > 0.0f);
  }

  printf("breath_ellipse_view_test passed\n");
  return 0;
}
