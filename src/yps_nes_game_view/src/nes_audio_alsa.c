#include "nes_audio.h"
#include <alsa/asoundlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct _nes_audio_t {
  snd_pcm_t* pcm;
  uint32_t sample_rate;
};

nes_audio_t* nes_audio_open(uint32_t sample_rate) {
  nes_audio_t* a = (nes_audio_t*)calloc(1, sizeof(nes_audio_t));
  if (!a) return NULL;
  a->sample_rate = sample_rate;

  int err = snd_pcm_open(&a->pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0) {
    fprintf(stderr, "yps_nes_game_view: snd_pcm_open failed: %s\n", snd_strerror(err));
    free(a);
    return NULL;
  }

  /* Mono, U8, sample_rate, ~100ms buffer */
  err = snd_pcm_set_params(a->pcm,
                           SND_PCM_FORMAT_U8,
                           SND_PCM_ACCESS_RW_INTERLEAVED,
                           1,            /* channels */
                           sample_rate,  /* rate */
                           1,            /* allow resampling */
                           100000);      /* latency: 100ms */
  if (err < 0) {
    fprintf(stderr, "yps_nes_game_view: snd_pcm_set_params failed: %s\n", snd_strerror(err));
    snd_pcm_close(a->pcm);
    free(a);
    return NULL;
  }
  return a;
}

int nes_audio_write(nes_audio_t* a, const uint8_t* samples, size_t n, uint8_t volume) {
  if (!a || !a->pcm) return -1;
  if (n == 0) return 0;

  /* Apply volume: U8 samples centered on 128 */
  uint8_t scratch[2048];
  const uint8_t* src = samples;
  if (volume != 100 && n <= sizeof(scratch)) {
    for (size_t i = 0; i < n; i++) {
      int s = (int)samples[i] - 128;
      s = s * (int)volume / 100;
      scratch[i] = (uint8_t)(s + 128);
    }
    src = scratch;
  }

  size_t offset = 0;
  while (offset < n) {
    snd_pcm_sframes_t wrote = snd_pcm_writei(a->pcm, src + offset, n - offset);
    if (wrote < 0) {
      wrote = snd_pcm_recover(a->pcm, (int)wrote, 1);
      if (wrote < 0) return -1;
      continue;
    }
    offset += (size_t)wrote;
  }
  return (int)n;
}

void nes_audio_close(nes_audio_t* a) {
  if (!a) return;
  if (a->pcm) {
    snd_pcm_drain(a->pcm);
    snd_pcm_close(a->pcm);
  }
  free(a);
}
