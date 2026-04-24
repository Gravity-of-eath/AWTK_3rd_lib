#include <assert.h>
#include <stdio.h>
#include "../src/nes_letterbox.h"

static void test_exact_2x(void) {
  nes_rect_t r = nes_letterbox_fit(512, 480);
  assert(r.w == 512 && r.h == 480);
  assert(r.x == 0 && r.y == 0);
  printf("test_exact_2x OK\n");
}

static void test_wider_than_nes(void) {
  /* 800x240 — height is the constraint, 256x240, horizontal letterbox */
  nes_rect_t r = nes_letterbox_fit(800, 240);
  assert(r.w == 256 && r.h == 240);
  assert(r.x == (800 - 256) / 2);
  assert(r.y == 0);
  printf("test_wider_than_nes OK\n");
}

static void test_taller_than_nes(void) {
  /* 256x800 — width is the constraint; vertical letterbox */
  nes_rect_t r = nes_letterbox_fit(256, 800);
  assert(r.w == 256 && r.h == 240);
  assert(r.x == 0);
  assert(r.y == (800 - 240) / 2);
  printf("test_taller_than_nes OK\n");
}

static void test_non_integer_scale(void) {
  /* 300x300 — min(300/256,300/240)=1.17..; w=~301, h=~281. Use x/y centered. */
  nes_rect_t r = nes_letterbox_fit(300, 300);
  /* scale = min(1.171, 1.25) = 1.171; dst_w = 256*1.171 = 299 */
  assert(r.w >= 298 && r.w <= 300);
  assert(r.h >= 279 && r.h <= 282);
  assert(r.x + r.w <= 300);
  assert(r.y + r.h <= 300);
  printf("test_non_integer_scale OK (w=%d h=%d x=%d y=%d)\n", r.w, r.h, r.x, r.y);
}

static void test_too_small(void) {
  nes_rect_t r = nes_letterbox_fit(0, 0);
  assert(r.w == 0 || r.h == 0); /* degenerate */
  printf("test_too_small OK\n");
}

int main(void) {
  test_exact_2x();
  test_wider_than_nes();
  test_taller_than_nes();
  test_non_integer_scale();
  test_too_small();
  return 0;
}
