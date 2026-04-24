#define _GNU_SOURCE
#include "nes_runtime.h"
#include <stdlib.h>
#include <string.h>

/* Singleton claim */
static _Atomic(nes_runtime_t*) g_active_runtime = NULL;

nes_runtime_t* nes_runtime_create(void) {
  nes_runtime_t* rt = (nes_runtime_t*)calloc(1, sizeof(*rt));
  if (!rt) return NULL;

  pthread_mutex_init(&rt->state_lock, NULL);
  pthread_cond_init(&rt->state_cond, NULL);
  rt->state = NES_STATE_STOPPED;
  atomic_init(&rt->volume, 80);
  atomic_init(&rt->fps, 0.0f);
  atomic_init(&rt->last_frame_us, 0);
  atomic_init(&rt->front_idx, 0);
  atomic_init(&rt->pad1, 0);
  atomic_init(&rt->sys_req, 0);
  rt->sample_rate = 22050;
  rt->sound_enable = true;

  rt->buffers[0] = (uint16_t*)calloc(NES_FB_W * NES_FB_H, sizeof(uint16_t));
  rt->buffers[1] = (uint16_t*)calloc(NES_FB_W * NES_FB_H, sizeof(uint16_t));
  if (!rt->buffers[0] || !rt->buffers[1]) {
    free(rt->buffers[0]); free(rt->buffers[1]);
    pthread_mutex_destroy(&rt->state_lock);
    pthread_cond_destroy(&rt->state_cond);
    free(rt);
    return NULL;
  }
  return rt;
}

void nes_runtime_destroy(nes_runtime_t* rt) {
  if (!rt) return;
  if (rt->state != NES_STATE_STOPPED) {
    /* Caller should have stopped us first. Best effort. */
    nes_runtime_stop(rt);
  }
  free(rt->buffers[0]);
  free(rt->buffers[1]);
  pthread_mutex_destroy(&rt->state_lock);
  pthread_cond_destroy(&rt->state_cond);
  free(rt);
}

void nes_runtime_set_rom(nes_runtime_t* rt, const char* p) {
  if (!rt) return;
  strncpy(rt->rom_path, p ? p : "", sizeof(rt->rom_path) - 1);
  rt->rom_path[sizeof(rt->rom_path) - 1] = '\0';
}
void nes_runtime_set_sram_path(nes_runtime_t* rt, const char* p) {
  if (!rt) return;
  strncpy(rt->sram_path, p ? p : "", sizeof(rt->sram_path) - 1);
  rt->sram_path[sizeof(rt->sram_path) - 1] = '\0';
}
void nes_runtime_set_sound(nes_runtime_t* rt, bool e, uint32_t r) {
  if (!rt) return; rt->sound_enable = e; rt->sample_rate = r;
}
void nes_runtime_set_volume(nes_runtime_t* rt, uint8_t v) {
  if (!rt) return; atomic_store(&rt->volume, v);
}
void nes_runtime_set_on_frame(nes_runtime_t* rt, void (*cb)(void*), void* ctx) {
  if (!rt) return; rt->on_frame = cb; rt->on_frame_ctx = ctx;
}

uint16_t* nes_runtime_front_buffer(nes_runtime_t* rt) {
  if (!rt) return NULL;
  int idx = atomic_load(&rt->front_idx);
  return rt->buffers[idx];
}

void nes_runtime_pad_set_bit(nes_runtime_t* rt, uint32_t bit, bool pressed) {
  if (!rt) return;
  uint32_t mask = 1u << bit;
  uint32_t cur = atomic_load(&rt->pad1);
  uint32_t next = pressed ? (cur | mask) : (cur & ~mask);
  atomic_store(&rt->pad1, next);
}

float nes_runtime_fps(nes_runtime_t* rt) {
  if (!rt) return 0.0f;
  return atomic_load(&rt->fps);
}

nes_state_t nes_runtime_state(nes_runtime_t* rt) {
  if (!rt) return NES_STATE_STOPPED;
  pthread_mutex_lock(&rt->state_lock);
  nes_state_t s = rt->state;
  pthread_mutex_unlock(&rt->state_lock);
  return s;
}

bool nes_runtime_claim_singleton(nes_runtime_t* rt) {
  nes_runtime_t* expected = NULL;
  return atomic_compare_exchange_strong(&g_active_runtime, &expected, rt);
}
void nes_runtime_release_singleton(nes_runtime_t* rt) {
  nes_runtime_t* expected = rt;
  atomic_compare_exchange_strong(&g_active_runtime, &expected, NULL);
}

/* Stubs — real impl in Task 9. */
int nes_runtime_start(nes_runtime_t* rt)  { (void)rt; return -1; }
int nes_runtime_pause(nes_runtime_t* rt)  { (void)rt; return -1; }
int nes_runtime_resume(nes_runtime_t* rt) { (void)rt; return -1; }
int nes_runtime_stop(nes_runtime_t* rt)   { (void)rt; return 0; }
