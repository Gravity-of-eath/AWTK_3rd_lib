#ifndef YPS_NES_RUNTIME_H
#define YPS_NES_RUNTIME_H
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <limits.h>
#include "nes_audio.h"

/* C11 <stdatomic.h> and the _Atomic qualifier aren't available in GCC's C++
 * mode (pre-C++23). From C++ we present the fields as plain layout-compatible
 * types; C++ code must access them via GCC atomic builtins (see glue). */
#ifdef __cplusplus
/* Strip C11 atomics to layout-compatible plain types. C++ code uses GCC
 * atomic builtins on the underlying fields — see InfoNES_System_awtk.cpp. */
#define _Atomic
typedef unsigned long atomic_ulong;
typedef int           atomic_int;
extern "C" {
#else
#include <stdatomic.h>
#endif

#define NES_FB_W 256
#define NES_FB_H 240

typedef enum {
  NES_STATE_STOPPED = 0,
  NES_STATE_RUNNING,
  NES_STATE_PAUSED,
  NES_STATE_STOPPING,
} nes_state_t;

typedef struct _nes_runtime_t {
  /* Thread + state */
  pthread_t tid;
  pthread_mutex_t state_lock;
  pthread_cond_t  state_cond;
  nes_state_t     state;
  bool            thread_started;

  /* Configuration (stable while thread running) */
  char rom_path[PATH_MAX];
  char sram_path[PATH_MAX];
  bool sound_enable;
  uint32_t sample_rate;

  /* Runtime-modifiable */
  _Atomic uint8_t volume;
  _Atomic float   fps;
  atomic_ulong    last_frame_us;  /* used by fps calc */

  /* Frame buffers */
  uint16_t* buffers[2];
  atomic_int front_idx;

  /* Input */
  _Atomic uint32_t pad1;
  _Atomic uint32_t sys_req;

  /* UI hook */
  void (*on_frame)(void* ctx);
  void* on_frame_ctx;

  /* Audio */
  nes_audio_t* audio;
} nes_runtime_t;

/* Create/destroy — not start/stop; allocates buffers and inits mutex/cond. */
nes_runtime_t* nes_runtime_create(void);
void nes_runtime_destroy(nes_runtime_t* rt);

/* Configuration — must be called while state == STOPPED. */
void nes_runtime_set_rom(nes_runtime_t* rt, const char* rom_path);
void nes_runtime_set_sram_path(nes_runtime_t* rt, const char* sram_path);
void nes_runtime_set_sound(nes_runtime_t* rt, bool enable, uint32_t sample_rate);
void nes_runtime_set_volume(nes_runtime_t* rt, uint8_t volume);
void nes_runtime_set_on_frame(nes_runtime_t* rt, void (*cb)(void*), void* ctx);

/* Lifecycle — implemented later tasks. */
int  nes_runtime_start(nes_runtime_t* rt);   /* 0 OK, <0 err */
int  nes_runtime_pause(nes_runtime_t* rt);
int  nes_runtime_resume(nes_runtime_t* rt);
int  nes_runtime_stop(nes_runtime_t* rt);    /* joins thread */
nes_state_t nes_runtime_state(nes_runtime_t* rt);

/* Accessors used by widget paint / events. */
uint16_t* nes_runtime_front_buffer(nes_runtime_t* rt);
void nes_runtime_pad_set_bit(nes_runtime_t* rt, uint32_t bit, bool pressed);
float nes_runtime_fps(nes_runtime_t* rt);

/* Single-instance guard (InfoNES global state constraint). Returns true if
 * this runtime successfully claimed the singleton; false if another is active. */
bool nes_runtime_claim_singleton(nes_runtime_t* rt);
void nes_runtime_release_singleton(nes_runtime_t* rt);

/* Internal: the runtime active for the calling thread (used by InfoNES glue).
 * Set by runtime thread_main just before calling InfoNES_Main. */
nes_runtime_t* nes_runtime_current(void);
void nes_runtime_set_current(nes_runtime_t* rt);

#ifdef __cplusplus
}
#endif
#endif
