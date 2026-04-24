#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "base/keys.h"
#include "../src/nes_keymap.h"

static void test_defaults_when_empty(void) {
  uint32_t defaults[8] = {TK_KEY_UP, TK_KEY_DOWN, TK_KEY_LEFT, TK_KEY_RIGHT,
                          TK_KEY_x, TK_KEY_z, TK_KEY_s, TK_KEY_a};
  uint32_t out[8] = {0};
  int n = nes_keymap_parse("", defaults, out);
  assert(n == 0);
  for (int i = 0; i < 8; i++) assert(out[i] == defaults[i]);
  printf("test_defaults_when_empty OK\n");
}

static void test_partial_override(void) {
  uint32_t defaults[8] = {TK_KEY_UP, TK_KEY_DOWN, TK_KEY_LEFT, TK_KEY_RIGHT,
                          TK_KEY_x, TK_KEY_z, TK_KEY_s, TK_KEY_a};
  uint32_t out[8] = {0};
  int n = nes_keymap_parse("a=SPACE,start=RETURN", defaults, out);
  assert(n == 2);
  assert(out[0] == TK_KEY_UP);      /* unchanged */
  assert(out[1] == TK_KEY_DOWN);
  assert(out[2] == TK_KEY_LEFT);
  assert(out[3] == TK_KEY_RIGHT);
  assert(out[4] == TK_KEY_SPACE);   /* a remapped */
  assert(out[5] == TK_KEY_z);
  assert(out[6] == TK_KEY_RETURN);  /* start remapped */
  assert(out[7] == TK_KEY_a);
  printf("test_partial_override OK\n");
}

static void test_all_slots(void) {
  uint32_t defaults[8] = {0,0,0,0,0,0,0,0};
  uint32_t out[8] = {0};
  int n = nes_keymap_parse("up=w,down=s,left=a,right=d,a=j,b=k,start=RETURN,select=ESCAPE",
                           defaults, out);
  assert(n == 8);
  assert(out[0] == TK_KEY_w);
  assert(out[1] == TK_KEY_s);
  assert(out[2] == TK_KEY_a);
  assert(out[3] == TK_KEY_d);
  assert(out[4] == TK_KEY_j);
  assert(out[5] == TK_KEY_k);
  assert(out[6] == TK_KEY_RETURN);
  assert(out[7] == TK_KEY_ESCAPE);
  printf("test_all_slots OK\n");
}

static void test_whitespace_and_case(void) {
  uint32_t defaults[8] = {0,0,0,0,0,0,0,0};
  uint32_t out[8] = {0};
  int n = nes_keymap_parse("  UP = up ,  A = X  ", defaults, out);
  assert(n == 2);
  assert(out[0] == TK_KEY_UP);
  assert(out[4] == TK_KEY_x);  /* TK_KEY_x lowercase */
  printf("test_whitespace_and_case OK\n");
}

static void test_unknown_key_ignored(void) {
  uint32_t defaults[8] = {TK_KEY_UP};
  uint32_t out[8] = {0};
  int n = nes_keymap_parse("up=UP,nope=X,b=Z", defaults, out);
  /* nope=X silently skipped (unknown slot name); up and b are fine */
  assert(n == 2);
  assert(out[0] == TK_KEY_UP);
  assert(out[5] == TK_KEY_z);
  printf("test_unknown_key_ignored OK\n");
}

int main(void) {
  test_defaults_when_empty();
  test_partial_override();
  test_all_slots();
  test_whitespace_and_case();
  test_unknown_key_ignored();
  return 0;
}
