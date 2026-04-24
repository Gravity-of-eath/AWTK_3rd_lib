/*===================================================================*/
/*                                                                   */
/*  InfoNES_System_awtk.cpp : AWTK glue for InfoNES                  */
/*                                                                   */
/*  Implements every callback declared in InfoNES_System.h by        */
/*  forwarding to the per-thread nes_runtime_t set by runtime.       */
/*                                                                   */
/*===================================================================*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdint.h>

/* C11 _Atomic qualifier in nes_runtime.h is not recognised by C++ compilers,
 * but the underlying storage is layout-compatible on GCC/Clang for the small
 * types we use (int, uint8_t, uint32_t, float, unsigned long). Use GCC's
 * built-in atomics to access those fields from C++ — they accept a plain
 * pointer to the underlying type. */
extern "C" {
#include "../nes_runtime.h"
#include "../nes_audio.h"
}

/* Helpers: reinterpret a pointer-to-_Atomic-T as pointer-to-T, then use GCC
 * atomic builtins. Layout compatibility holds for all types we access here.
 * __atomic_load_n / __atomic_store_n only accept integer/pointer operands,
 * so float fields go through a uint32_t alias (see glue_atomic_*_float). */
#define GLUE_ATOMIC_LOAD(ptr, T) \
  __atomic_load_n(reinterpret_cast<T*>(const_cast<void*>(static_cast<const volatile void*>(ptr))), __ATOMIC_SEQ_CST)
#define GLUE_ATOMIC_STORE(ptr, T, val) \
  __atomic_store_n(reinterpret_cast<T*>(const_cast<void*>(static_cast<const volatile void*>(ptr))), (T)(val), __ATOMIC_SEQ_CST)

static inline float glue_atomic_load_float(float* p) {
  uint32_t u = __atomic_load_n(reinterpret_cast<uint32_t*>(p), __ATOMIC_SEQ_CST);
  float r; memcpy(&r, &u, sizeof r); return r;
}
static inline void glue_atomic_store_float(float* p, float v) {
  uint32_t u; memcpy(&u, &v, sizeof u);
  __atomic_store_n(reinterpret_cast<uint32_t*>(p), u, __ATOMIC_SEQ_CST);
}

#include "../../../../InfoNES/src/InfoNES.h"
#include "../../../../InfoNES/src/InfoNES_System.h"
#include "../../../../InfoNES/src/InfoNES_pAPU.h"

/* -------------------------------------------------------------------
 *  Global state required by InfoNES core (definitions).
 *  NesPalette[64] is declared extern in InfoNES_System.h but defined
 *  by each platform's system file (we don't link those), so we
 *  define it here. Linkage matches the declaration (no extern "C").
 * -----------------------------------------------------------------*/
WORD NesPalette[64] = {0};

/* NES base palette (RGB triplets, 0-255). Source: InfoNES reference ports. */
static const uint8_t kNesPaletteRGB[64][3] = {
  {112,112,112},{32,24,136},{0,0,168},{64,0,152},
  {136,0,112},{168,0,16},{160,0,0},{120,8,0},
  {64,40,0},{0,64,0},{0,80,0},{0,56,16},
  {24,56,88},{0,0,0},{0,0,0},{0,0,0},
  {184,184,184},{0,112,232},{32,56,232},{128,0,240},
  {184,0,184},{224,0,88},{216,40,0},{200,72,8},
  {136,112,0},{0,144,0},{0,168,0},{0,144,56},
  {0,128,136},{0,0,0},{0,0,0},{0,0,0},
  {248,248,248},{56,184,248},{88,144,248},{64,136,248},
  {240,120,248},{248,112,176},{248,112,96},{248,152,56},
  {240,184,56},{128,208,16},{72,216,72},{88,248,152},
  {0,232,216},{0,0,0},{0,0,0},{0,0,0},
  {248,248,248},{168,224,248},{192,208,248},{208,200,248},
  {248,192,248},{248,192,216},{248,184,176},{248,216,168},
  {248,224,160},{224,248,160},{168,240,184},{176,248,200},
  {152,248,240},{0,0,0},{0,0,0},{0,0,0},
};

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

/* Called by runtime before InfoNES_Main to populate the RGB565 palette table.
 * WorkFrame is a fixed array in InfoNES.cpp (not a pointer), so we cannot
 * retarget it to the back buffer — we copy it each frame in InfoNES_LoadFrame. */
extern "C" void infones_glue_init_palette(void) {
  for (int i = 0; i < 64; i++) {
    NesPalette[i] = rgb565(kNesPaletteRGB[i][0],
                           kNesPaletteRGB[i][1],
                           kNesPaletteRGB[i][2]);
  }
}

static uint64_t now_us(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

/* -------------------------------------------------------------------
 *  InfoNES_System.h callbacks. These are declared WITHOUT extern "C"
 *  in InfoNES_System.h, so they carry C++ linkage — match that.
 * -----------------------------------------------------------------*/

int InfoNES_Menu(void) {
  nes_runtime_t* rt = nes_runtime_current();
  if (!rt) return -1;
  pthread_mutex_lock(&rt->state_lock);
  nes_state_t s = rt->state;
  pthread_mutex_unlock(&rt->state_lock);
  return (s == NES_STATE_STOPPING) ? -1 : 0;
}

int InfoNES_ReadRom(const char* pszFileName) {
  FILE* fp = fopen(pszFileName, "rb");
  if (!fp) return -1;
  if (fread(&NesHeader, sizeof NesHeader, 1, fp) != 1 ||
      memcmp(NesHeader.byID, "NES\x1a", 4) != 0) {
    fclose(fp);
    return -1;
  }
  memset(SRAM, 0, SRAM_SIZE);
  if (NesHeader.byInfo1 & 4) {
    /* trainer: 512 bytes into SRAM at 0x1000 */
    if (fread(&SRAM[0x1000], 512, 1, fp) != 1) {
      fclose(fp);
      return -1;
    }
  }
  ROM = (BYTE*)malloc((size_t)NesHeader.byRomSize * 0x4000);
  if (!ROM) { fclose(fp); return -1; }
  if (fread(ROM, 0x4000, NesHeader.byRomSize, fp) != NesHeader.byRomSize) {
    free(ROM); ROM = NULL;
    fclose(fp);
    return -1;
  }
  if (NesHeader.byVRomSize > 0) {
    VROM = (BYTE*)malloc((size_t)NesHeader.byVRomSize * 0x2000);
    if (!VROM) { free(ROM); ROM = NULL; fclose(fp); return -1; }
    if (fread(VROM, 0x2000, NesHeader.byVRomSize, fp) != NesHeader.byVRomSize) {
      free(ROM); ROM = NULL;
      free(VROM); VROM = NULL;
      fclose(fp);
      return -1;
    }
  }
  fclose(fp);
  return 0;
}

void InfoNES_ReleaseRom(void) {
  if (ROM)  { free(ROM);  ROM = NULL; }
  if (VROM) { free(VROM); VROM = NULL; }
}

void InfoNES_LoadFrame(void) {
  nes_runtime_t* rt = nes_runtime_current();
  if (!rt) return;

  /* FPS EMA */
  uint64_t t = now_us();
  unsigned long last = GLUE_ATOMIC_LOAD(&rt->last_frame_us, unsigned long);
  if (last != 0) {
    uint64_t dt = t - (uint64_t)last;
    if (dt > 0) {
      float inst = 1000000.0f / (float)dt;
      float cur  = glue_atomic_load_float(&rt->fps);
      float next = (cur == 0.0f) ? inst : (0.9f * cur + 0.1f * inst);
      glue_atomic_store_float(&rt->fps, next);
    }
  }
  GLUE_ATOMIC_STORE(&rt->last_frame_us, unsigned long, t);

  /* WorkFrame is a fixed array — copy into the back buffer, then publish. */
  int old_front = GLUE_ATOMIC_LOAD(&rt->front_idx, int);
  int back      = 1 - old_front;
  uint16_t* dst = rt->buffers[back];
  if (dst) {
    memcpy(dst, WorkFrame,
           (size_t)NES_FB_W * (size_t)NES_FB_H * sizeof(uint16_t));
  }
  GLUE_ATOMIC_STORE(&rt->front_idx, int, back);

  if (rt->on_frame) rt->on_frame(rt->on_frame_ctx);
}

void InfoNES_PadState(DWORD* pad1, DWORD* pad2, DWORD* sys) {
  nes_runtime_t* rt = nes_runtime_current();
  if (!rt) { *pad1 = 0; *pad2 = 0; *sys = 0; return; }
  *pad1 = (DWORD)GLUE_ATOMIC_LOAD(&rt->pad1, uint32_t);
  *pad2 = 0;
  *sys  = (DWORD)GLUE_ATOMIC_LOAD(&rt->sys_req, uint32_t);
}

void* InfoNES_MemoryCopy(void* d, const void* s, int n) {
  return memcpy(d, s, (size_t)n);
}

void* InfoNES_MemorySet(void* d, int c, int n) {
  return memset(d, c, (size_t)n);
}

void InfoNES_DebugPrint(char* msg) {
  fprintf(stderr, "%s\n", msg);
}

void InfoNES_Wait(void) {
  nes_runtime_t* rt = nes_runtime_current();
  if (!rt) return;

  /* Handle pause */
  pthread_mutex_lock(&rt->state_lock);
  while (rt->state == NES_STATE_PAUSED) {
    pthread_cond_wait(&rt->state_cond, &rt->state_lock);
  }
  pthread_mutex_unlock(&rt->state_lock);

  /* Wall-clock frame pacing ONLY when sound is disabled. With sound,
   * SoundOutput blocks on audio backend for pacing. InfoNES_Wait is
   * called every scanline (~262/frame) — only pace once per frame. */
  if (!rt->sound_enable) {
    static __thread int scanline_counter = 0;
    static __thread uint64_t last_pace_us = 0;
    if (++scanline_counter >= 262) {
      scanline_counter = 0;
      uint64_t now = now_us();
      if (last_pace_us != 0) {
        uint64_t target = last_pace_us + (1000000ULL / 60ULL);
        if (now < target) usleep((useconds_t)(target - now));
      }
      last_pace_us = now_us();
    }
  }
}

void InfoNES_SoundInit(void) {
  /* nothing */
}

int InfoNES_SoundOpen(int samples_per_sync, int sample_rate) {
  (void)samples_per_sync;
  nes_runtime_t* rt = nes_runtime_current();
  if (!rt || !rt->sound_enable) return 0;
  if (rt->audio) return 1;
  rt->audio = nes_audio_open((uint32_t)sample_rate);
  if (!rt->audio) {
    fprintf(stderr,
            "yps_nes_game_view: audio open failed; running silent with frame pacing\n");
    rt->sound_enable = false;   /* fallback, wall-clock pacing kicks in */
    return 0;
  }
  return 1;
}

void InfoNES_SoundClose(void) {
  nes_runtime_t* rt = nes_runtime_current();
  if (!rt) return;
  if (rt->audio) {
    nes_audio_close(rt->audio);
    rt->audio = NULL;
  }
}

void InfoNES_SoundOutput(int samples, BYTE* w1, BYTE* w2, BYTE* w3, BYTE* w4, BYTE* w5) {
  nes_runtime_t* rt = nes_runtime_current();
  if (!rt || !rt->audio) return;

  /* Mix five u8 waves (mean) into a local buffer; backend handles volume/pacing. */
  static uint8_t mix[4096];
  int n = samples > (int)sizeof(mix) ? (int)sizeof(mix) : samples;
  for (int i = 0; i < n; i++) {
    mix[i] = (uint8_t)(((int)w1[i] + (int)w2[i] + (int)w3[i] + (int)w4[i] + (int)w5[i]) / 5);
  }
  (void)nes_audio_write(rt->audio, mix, (size_t)n, GLUE_ATOMIC_LOAD(&rt->volume, uint8_t));
}

void InfoNES_MessageBox(char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "yps_nes_game_view: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}
