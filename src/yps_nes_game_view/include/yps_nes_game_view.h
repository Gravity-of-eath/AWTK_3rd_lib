#ifndef TK_YPS_NES_GAME_VIEW_H
#define TK_YPS_NES_GAME_VIEW_H

#include "base/widget.h"

BEGIN_C_DECLS

#define WIDGET_TYPE_YPS_NES_GAME_VIEW "yps_nes_game_view"

/* property names — used in XML attrs */
#define YPS_NES_GAME_VIEW_PROP_ROM           "rom"
#define YPS_NES_GAME_VIEW_PROP_SRAM_DIR      "sram_dir"
#define YPS_NES_GAME_VIEW_PROP_KEY_MAP       "key_map"
#define YPS_NES_GAME_VIEW_PROP_AUTO_PLAY     "auto_play"
#define YPS_NES_GAME_VIEW_PROP_SOUND_ENABLE  "sound_enable"
#define YPS_NES_GAME_VIEW_PROP_SAMPLE_RATE   "sample_rate"
#define YPS_NES_GAME_VIEW_PROP_VOLUME        "volume"
#define YPS_NES_GAME_VIEW_PROP_FPS           "fps"  /* read-only */

struct _nes_runtime_t;
typedef struct _nes_runtime_t nes_runtime_t;

typedef struct _yps_nes_game_view_t {
  widget_t widget;
  char* rom;
  char* sram_dir;
  char* key_map;
  bool_t auto_play;
  bool_t sound_enable;
  uint32_t sample_rate;
  uint8_t volume;

  nes_runtime_t* rt;       /* opaque; owned by widget */
  bitmap_t* front_bmp;     /* bitmap that wraps front buffer; allocated in on_create */
  uint32_t keymap[8];      /* 0=UP 1=DOWN 2=LEFT 3=RIGHT 4=A 5=B 6=START 7=SELECT -> TK_KEY_* */
} yps_nes_game_view_t;

widget_t* yps_nes_game_view_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h);
widget_t* yps_nes_game_view_cast(widget_t* widget);

ret_t yps_nes_game_view_start  (widget_t* widget);
ret_t yps_nes_game_view_pause  (widget_t* widget);
ret_t yps_nes_game_view_stop   (widget_t* widget);
ret_t yps_nes_game_view_set_rom(widget_t* widget, const char* rom);
ret_t yps_nes_game_view_set_sram_dir   (widget_t* widget, const char* dir);
ret_t yps_nes_game_view_set_key_map    (widget_t* widget, const char* key_map);
ret_t yps_nes_game_view_set_sound_enable(widget_t* widget, bool_t enable);
ret_t yps_nes_game_view_set_sample_rate (widget_t* widget, uint32_t rate);
ret_t yps_nes_game_view_set_volume      (widget_t* widget, uint8_t volume);
float_t yps_nes_game_view_get_fps       (widget_t* widget);

#define YPS_NES_GAME_VIEW(widget) \
  ((yps_nes_game_view_t*)(yps_nes_game_view_cast(WIDGET(widget))))

TK_EXTERN_VTABLE(yps_nes_game_view);

END_C_DECLS
#endif
