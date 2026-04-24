#include "yps_nes_game_view.h"
#include "tkc/mem.h"
#include "tkc/utils.h"

static ret_t yps_nes_game_view_on_paint_self(widget_t* widget, canvas_t* c) {
  (void)widget; (void)c;
  return RET_OK;
}
static ret_t yps_nes_game_view_on_destroy(widget_t* widget) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  if (v == NULL) return RET_BAD_PARAMS;
  TKMEM_FREE(v->rom);
  TKMEM_FREE(v->sram_dir);
  TKMEM_FREE(v->key_map);
  return RET_OK;
}

TK_DECL_VTABLE(yps_nes_game_view) = {
  .size = sizeof(yps_nes_game_view_t),
  .type = WIDGET_TYPE_YPS_NES_GAME_VIEW,
  .create = yps_nes_game_view_create,
  .on_paint_self = yps_nes_game_view_on_paint_self,
  .on_destroy = yps_nes_game_view_on_destroy,
};

widget_t* yps_nes_game_view_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h) {
  widget_t* widget = widget_create(parent, TK_REF_VTABLE(yps_nes_game_view), x, y, w, h);
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL, NULL);

  v->sram_dir = tk_strdup("/tmp");
  v->auto_play = TRUE;
  v->sound_enable = TRUE;
  v->sample_rate = 22050;
  v->volume = 80;
  return widget;
}

widget_t* yps_nes_game_view_cast(widget_t* widget) {
  return_value_if_fail(widget != NULL && widget->vt == TK_REF_VTABLE(yps_nes_game_view), NULL);
  return widget;
}

/* Method stubs — real impl comes in later tasks */
ret_t yps_nes_game_view_start(widget_t* w)  { (void)w; return RET_NOT_IMPL; }
ret_t yps_nes_game_view_pause(widget_t* w)  { (void)w; return RET_NOT_IMPL; }
ret_t yps_nes_game_view_stop(widget_t* w)   { (void)w; return RET_NOT_IMPL; }
ret_t yps_nes_game_view_set_rom(widget_t* w, const char* r) { (void)w; (void)r; return RET_NOT_IMPL; }
ret_t yps_nes_game_view_set_sram_dir(widget_t* w, const char* d){ (void)w; (void)d; return RET_NOT_IMPL; }
ret_t yps_nes_game_view_set_key_map(widget_t* w, const char* k) { (void)w; (void)k; return RET_NOT_IMPL; }
ret_t yps_nes_game_view_set_sound_enable(widget_t* w, bool_t e){ (void)w; (void)e; return RET_NOT_IMPL; }
ret_t yps_nes_game_view_set_sample_rate(widget_t* w, uint32_t r){ (void)w; (void)r; return RET_NOT_IMPL; }
ret_t yps_nes_game_view_set_volume(widget_t* w, uint8_t v)     { (void)w; (void)v; return RET_NOT_IMPL; }
float_t yps_nes_game_view_get_fps(widget_t* w)  { (void)w; return 0; }
