#include "nes_audio.h"
#include <stdlib.h>
#include <unistd.h>

struct _nes_audio_t {
  uint32_t sample_rate;
};

nes_audio_t* nes_audio_open(uint32_t sample_rate) {
  nes_audio_t* a = (nes_audio_t*)calloc(1, sizeof(nes_audio_t));
  if (!a) return NULL;
  a->sample_rate = sample_rate;
  return a;
}

int nes_audio_write(nes_audio_t* a, const uint8_t* samples, size_t n, uint8_t volume) {
  (void)samples; (void)volume;
  if (!a) return -1;
  /* Pace by sleeping for sample time to avoid flat-out spin when caller relies on blocking.
   * 1e6 / sample_rate microseconds per sample × n samples */
  if (a->sample_rate > 0 && n > 0) {
    useconds_t us = (useconds_t)(((uint64_t)n * 1000000ULL) / a->sample_rate);
    if (us > 0) usleep(us);
  }
  return (int)n;
}

void nes_audio_close(nes_audio_t* a) {
  free(a);
}
