/* Placeholder — full ALSA impl arrives in Task 6 */
#include "nes_audio.h"

nes_audio_t* nes_audio_open(uint32_t sample_rate) {
  (void)sample_rate;
  return (void*)0;
}

int nes_audio_write(nes_audio_t* a, const uint8_t* samples, size_t n, uint8_t volume) {
  (void)a; (void)samples; (void)n; (void)volume;
  return -1;
}

void nes_audio_close(nes_audio_t* a) {
  (void)a;
}
