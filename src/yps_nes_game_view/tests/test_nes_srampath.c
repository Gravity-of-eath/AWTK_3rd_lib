#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/nes_srampath.h"

static void test_simple(void) {
  char out[256];
  int n = nes_srampath_build("/tmp", "/home/u/mario.nes", out, sizeof(out));
  assert(n > 0);
  assert(strcmp(out, "/tmp/mario.srm") == 0);
  printf("test_simple OK\n");
}

static void test_no_ext(void) {
  char out[256];
  nes_srampath_build("/tmp", "noext", out, sizeof(out));
  assert(strcmp(out, "/tmp/noext.srm") == 0);
  printf("test_no_ext OK\n");
}

static void test_trailing_slash(void) {
  char out[256];
  nes_srampath_build("/tmp/", "game.nes", out, sizeof(out));
  assert(strcmp(out, "/tmp/game.srm") == 0);
  printf("test_trailing_slash OK\n");
}

static void test_dots_in_name(void) {
  char out[256];
  nes_srampath_build("/d", "path/a.b.c.nes", out, sizeof(out));
  /* strip only the last .ext */
  assert(strcmp(out, "/d/a.b.c.srm") == 0);
  printf("test_dots_in_name OK\n");
}

static void test_buffer_too_small(void) {
  char out[8];
  int n = nes_srampath_build("/tmp", "/a/b/c/long_rom_name.nes", out, sizeof(out));
  assert(n < 0);  /* overflow */
  printf("test_buffer_too_small OK\n");
}

int main(void) {
  test_simple();
  test_no_ext();
  test_trailing_slash();
  test_dots_in_name();
  test_buffer_too_small();
  return 0;
}
