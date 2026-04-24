#ifndef YPS_NES_AUDIO_H
#define YPS_NES_AUDIO_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _nes_audio_t nes_audio_t;

/* Open audio backend at given sample rate (Hz). Mono, unsigned 8-bit samples.
 * Returns NULL on failure. */
nes_audio_t* nes_audio_open(uint32_t sample_rate);

/* Write `n` samples. `volume` 0..100 — caller may pass current volume for per-write attenuation.
 * This call may block until the device can accept the data (used as pacing).
 * Returns number of samples written on success, negative on error. */
int nes_audio_write(nes_audio_t* a, const uint8_t* samples, size_t n, uint8_t volume);

void nes_audio_close(nes_audio_t* a);

#ifdef __cplusplus
}
#endif
#endif
