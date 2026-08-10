#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "jump_label.h"

static int nearly_equal(float a, float b, float eps) {
  return fabsf(a - b) < eps;
}

static void test_ease_out_bounce(void) {
  /* t=0 => 0 */
  assert(nearly_equal(jump_label_ease_out_bounce(0.0f), 0.0f, 0.001f));
  /* t=1 => 1 */
  assert(nearly_equal(jump_label_ease_out_bounce(1.0f), 1.0f, 0.001f));
  /* 中间值在 (0,1) 之间 */
  float v1 = jump_label_ease_out_bounce(0.25f);
  float v2 = jump_label_ease_out_bounce(0.5f);
  float v3 = jump_label_ease_out_bounce(0.75f);
  assert(v1 > 0.0f && v1 < 1.0f);
  assert(v2 > 0.0f && v2 < 1.0f);
  assert(v3 > 0.0f && v3 < 1.0f);
  /* 边界夹持 */
  assert(nearly_equal(jump_label_ease_out_bounce(-1.0f), 0.0f, 0.001f));
  assert(nearly_equal(jump_label_ease_out_bounce(2.0f), 1.0f, 0.001f));
  printf("  [PASS] test_ease_out_bounce\n");
}

static void test_ease_out_bounce_stress(void) {
  int i;
  for (i = 0; i <= 10000; i++) {
    float t = (float)i / 10000.0f;
    float v = jump_label_ease_out_bounce(t);
    assert(v >= 0.0f && v <= 1.001f);
  }
  printf("  [PASS] test_ease_out_bounce_stress (10001 samples)\n");
}

int main(void) {
  printf("jump_label tests:\n");
  test_ease_out_bounce();
  test_ease_out_bounce_stress();
  printf("All tests passed.\n");
  return 0;
}
