#include "yps_nes_game_view.h"
#include "nes_runtime.h"
#include "nes_keymap.h"
#include "nes_letterbox.h"
#include "nes_srampath.h"

#include "base/keys.h"
#include "base/events.h"
#include "base/idle.h"
#include "base/bitmap.h"
#include "base/widget.h"
#include "tkc/mem.h"
#include "tkc/utils.h"

#include <string.h>
#include <limits.h>

static const uint32_t kDefaultKeyMap[8] = {
  TK_KEY_UP, TK_KEY_DOWN, TK_KEY_LEFT, TK_KEY_RIGHT,
  TK_KEY_x, TK_KEY_z, TK_KEY_s, TK_KEY_a
};

/* Slot -> InfoNES pad bit (SDL port convention):
 * 0:UP=4  1:DOWN=5  2:LEFT=6  3:RIGHT=7
 * 4:A=0   5:B=1     6:START=3 7:SELECT=2 */
static const uint32_t kSlotToPadBit[8] = {4, 5, 6, 7, 0, 1, 3, 2};

static void apply_key_map(yps_nes_game_view_t* v) {
  nes_keymap_parse(v->key_map ? v->key_map : "", kDefaultKeyMap, v->keymap);
}

static int8_t keymap_slot_for_code(yps_nes_game_view_t* v, uint32_t code) {
  for (int8_t i = 0; i < 8; i++) if (v->keymap[i] == code) return i;
  return -1;
}

/* UI-thread idle: invalidate. Queued from emulator thread via idle_queue. */
static ret_t frame_invalidate_idle(const idle_info_t* info) {
  widget_t* w = (widget_t*)info->ctx;
  if (w) widget_invalidate_force(w, NULL);
  return RET_REMOVE;
}

/* Called from emulator thread each frame. */
static void on_frame_thread(void* ctx) {
  idle_queue(frame_invalidate_idle, ctx);
}

static ret_t yps_nes_game_view_on_paint_self(widget_t* widget, canvas_t* c) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  if (!v || !v->rt || !v->front_bmp) return RET_OK;

  uint16_t* src = nes_runtime_front_buffer(v->rt);
  if (!src) return RET_OK;

  uint8_t* dst = bitmap_lock_buffer_for_write(v->front_bmp);
  if (dst) {
    memcpy(dst, src, (size_t)NES_FB_W * (size_t)NES_FB_H * sizeof(uint16_t));
    bitmap_unlock_buffer(v->front_bmp);
  }

  nes_rect_t r = nes_letterbox_fit(widget->w, widget->h);
  rect_t src_rect = {.x = 0, .y = 0, .w = NES_FB_W, .h = NES_FB_H};
  rect_t dst_rect = {.x = r.x, .y = r.y, .w = r.w, .h = r.h};
  canvas_draw_image(c, v->front_bmp, &src_rect, &dst_rect);
  return RET_OK;
}

static ret_t yps_nes_game_view_on_event(widget_t* widget, event_t* e) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  if (!v || !v->rt) return RET_OK;

  if (e->type == EVT_KEY_DOWN || e->type == EVT_KEY_UP) {
    key_event_t* ke = (key_event_t*)e;
    int8_t slot = keymap_slot_for_code(v, ke->key);
    if (slot >= 0) {
      nes_runtime_pad_set_bit(v->rt, kSlotToPadBit[slot], e->type == EVT_KEY_DOWN);
      return RET_STOP;
    }
  }
  return RET_OK;
}

static ret_t yps_nes_game_view_on_destroy(widget_t* widget) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  if (!v) return RET_BAD_PARAMS;
  if (v->rt) {
    nes_runtime_stop(v->rt);
    nes_runtime_destroy(v->rt);
    v->rt = NULL;
  }
  if (v->front_bmp) {
    bitmap_destroy(v->front_bmp);
    v->front_bmp = NULL;
  }
  TKMEM_FREE(v->rom);
  TKMEM_FREE(v->sram_dir);
  TKMEM_FREE(v->key_map);
  return RET_OK;
}

static ret_t yps_nes_game_view_set_prop(widget_t* widget, const char* name, const value_t* val) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  if (!v) return RET_BAD_PARAMS;

  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_ROM)) {
    TKMEM_FREE(v->rom);
    v->rom = tk_strdup(value_str(val));
    return RET_OK;
  } else if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_SRAM_DIR)) {
    TKMEM_FREE(v->sram_dir);
    v->sram_dir = tk_strdup(value_str(val));
    return RET_OK;
  } else if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_KEY_MAP)) {
    TKMEM_FREE(v->key_map);
    v->key_map = tk_strdup(value_str(val));
    apply_key_map(v);
    return RET_OK;
  } else if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_AUTO_PLAY)) {
    v->auto_play = value_bool(val);
    return RET_OK;
  } else if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_SOUND_ENABLE)) {
    v->sound_enable = value_bool(val);
    return RET_OK;
  } else if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_SAMPLE_RATE)) {
    v->sample_rate = (uint32_t)value_uint32(val);
    return RET_OK;
  } else if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_VOLUME)) {
    v->volume = (uint8_t)value_uint32(val);
    if (v->rt) nes_runtime_set_volume(v->rt, v->volume);
    return RET_OK;
  }
  return RET_NOT_FOUND;
}

static ret_t yps_nes_game_view_get_prop(widget_t* widget, const char* name, value_t* val) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  if (!v) return RET_BAD_PARAMS;

  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_ROM))          { value_set_str(val, v->rom ? v->rom : ""); return RET_OK; }
  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_SRAM_DIR))     { value_set_str(val, v->sram_dir ? v->sram_dir : ""); return RET_OK; }
  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_KEY_MAP))      { value_set_str(val, v->key_map ? v->key_map : ""); return RET_OK; }
  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_AUTO_PLAY))    { value_set_bool(val, v->auto_play); return RET_OK; }
  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_SOUND_ENABLE)) { value_set_bool(val, v->sound_enable); return RET_OK; }
  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_SAMPLE_RATE))  { value_set_uint32(val, v->sample_rate); return RET_OK; }
  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_VOLUME))       { value_set_uint32(val, v->volume); return RET_OK; }
  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_FPS)) {
    value_set_float(val, v->rt ? nes_runtime_fps(v->rt) : 0.0f);
    return RET_OK;
  }
  return RET_NOT_FOUND;
}

TK_DECL_VTABLE(yps_nes_game_view) = {
  .size = sizeof(yps_nes_game_view_t),
  .type = WIDGET_TYPE_YPS_NES_GAME_VIEW,
  .create = yps_nes_game_view_create,
  .set_prop = yps_nes_game_view_set_prop,
  .get_prop = yps_nes_game_view_get_prop,
  .on_paint_self = yps_nes_game_view_on_paint_self,
  .on_event = yps_nes_game_view_on_event,
  .on_destroy = yps_nes_game_view_on_destroy,
};

/* Deferred start: XML attrs are applied after widget_create returns, so we
 * queue an idle to kick things off once the loader has populated props. */
static ret_t yps_nes_game_view_start_if_auto(const idle_info_t* info) {
  widget_t* w = (widget_t*)info->ctx;
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(w);
  if (v && v->auto_play && v->rom && *v->rom) {
    yps_nes_game_view_start(w);
  }
  return RET_REMOVE;
}

widget_t* yps_nes_game_view_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h) {
  widget_t* widget = widget_create(parent, TK_REF_VTABLE(yps_nes_game_view), x, y, w, h);
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL, NULL);

  v->sram_dir = tk_strdup("/tmp");
  v->auto_play = TRUE;
  v->sound_enable = TRUE;
  v->sample_rate = 22050;
  v->volume = 80;
  memcpy(v->keymap, kDefaultKeyMap, sizeof(kDefaultKeyMap));

  v->rt = nes_runtime_create();
  v->front_bmp = bitmap_create_ex(NES_FB_W, NES_FB_H, 0, BITMAP_FMT_RGB565);
  if (v->rt) {
    nes_runtime_set_on_frame(v->rt, on_frame_thread, widget);
  }

  widget_set_focusable(widget, TRUE);

  idle_queue(yps_nes_game_view_start_if_auto, widget);
  return widget;
}

widget_t* yps_nes_game_view_cast(widget_t* widget) {
  return_value_if_fail(widget != NULL && widget->vt == TK_REF_VTABLE(yps_nes_game_view), NULL);
  return widget;
}

ret_t yps_nes_game_view_start(widget_t* widget) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL && v->rt != NULL, RET_BAD_PARAMS);
  if (!v->rom || !*v->rom) return RET_FAIL;

  nes_runtime_set_rom(v->rt, v->rom);
  char sram_path[PATH_MAX];
  nes_srampath_build(v->sram_dir ? v->sram_dir : "/tmp", v->rom, sram_path, sizeof(sram_path));
  nes_runtime_set_sram_path(v->rt, sram_path);
  nes_runtime_set_sound(v->rt, v->sound_enable, v->sample_rate);
  nes_runtime_set_volume(v->rt, v->volume);

  return nes_runtime_start(v->rt) == 0 ? RET_OK : RET_FAIL;
}

ret_t yps_nes_game_view_pause(widget_t* widget) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL && v->rt != NULL, RET_BAD_PARAMS);
  return nes_runtime_pause(v->rt) == 0 ? RET_OK : RET_FAIL;
}

ret_t yps_nes_game_view_stop(widget_t* widget) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL && v->rt != NULL, RET_BAD_PARAMS);
  return nes_runtime_stop(v->rt) == 0 ? RET_OK : RET_FAIL;
}

ret_t yps_nes_game_view_set_rom(widget_t* widget, const char* rom) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL, RET_BAD_PARAMS);
  if (v->rt) nes_runtime_stop(v->rt);
  TKMEM_FREE(v->rom);
  v->rom = tk_strdup(rom ? rom : "");
  return yps_nes_game_view_start(widget);
}

ret_t yps_nes_game_view_set_sram_dir(widget_t* widget, const char* dir) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL, RET_BAD_PARAMS);
  TKMEM_FREE(v->sram_dir);
  v->sram_dir = tk_strdup(dir ? dir : "/tmp");
  return RET_OK;
}

ret_t yps_nes_game_view_set_key_map(widget_t* widget, const char* key_map) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL, RET_BAD_PARAMS);
  TKMEM_FREE(v->key_map);
  v->key_map = tk_strdup(key_map ? key_map : "");
  apply_key_map(v);
  return RET_OK;
}

ret_t yps_nes_game_view_set_sound_enable(widget_t* widget, bool_t enable) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL, RET_BAD_PARAMS);
  v->sound_enable = enable;
  return RET_OK;
}

ret_t yps_nes_game_view_set_sample_rate(widget_t* widget, uint32_t rate) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL, RET_BAD_PARAMS);
  v->sample_rate = rate;
  return RET_OK;
}

ret_t yps_nes_game_view_set_volume(widget_t* widget, uint8_t volume) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL, RET_BAD_PARAMS);
  v->volume = volume;
  if (v->rt) nes_runtime_set_volume(v->rt, volume);
  return RET_OK;
}

float_t yps_nes_game_view_get_fps(widget_t* widget) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  if (!v || !v->rt) return 0.0f;
  return nes_runtime_fps(v->rt);
}
