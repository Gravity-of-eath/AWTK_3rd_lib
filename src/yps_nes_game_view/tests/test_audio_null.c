#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../src/nes_audio.h"

int main(void) {
  nes_audio_t* a = nes_audio_open(22050);
  assert(a != NULL);

  uint8_t buf[1024];
  memset(buf, 128, sizeof(buf));
  int rc = nes_audio_write(a, buf, sizeof(buf), 100);
  assert(rc == (int)sizeof(buf));  /* null impl returns sample count */

  nes_audio_close(a);
  printf("test_audio_null OK\n");
  return 0;
}
