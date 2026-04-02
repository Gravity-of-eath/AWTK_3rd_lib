#ifndef TK_JUMP_LABEL_H
#define TK_JUMP_LABEL_H

#include "base/widget.h"

BEGIN_C_DECLS

/**
 * @enum jump_label_state_t
 * 动画状态
 */
typedef enum _jump_label_state_t {
  JUMP_LABEL_STATE_IDLE = 0,
  JUMP_LABEL_STATE_SHOWING,
  JUMP_LABEL_STATE_SHOWN,
  JUMP_LABEL_STATE_CLEARING
} jump_label_state_t;

/**
 * @class jump_label_t
 * @parent widget_t
 * @annotation ["scriptable","design","widget"]
 *
 * 带跳动效果的文本控件。
 * 设置文本时文字逐个从底部跳上来；清除时文字以 FILO 顺序逐个跳回消失。
 * 兼容 AWTK label 的 style 属性（字体、字号、颜色等）。
 *
 * 在 xml 中使用：
 * ```xml
 * <jump_label x="c" y="50" w="300" h="120"
 *   jump_height="20"
 *   char_delay_ms="60"
 *   char_anim_ms="300"
 *   frame_interval_ms="16"
 *   line_wrap="true" />
 * ```
 */
typedef struct _jump_label_t {
  widget_t widget;

  /**
   * @property {int32_t} jump_height
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 跳动高度（像素），字符从下方多远处跳入。默认 20。
   */
  int32_t jump_height;

  /**
   * @property {uint32_t} char_delay_ms
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 相邻字符开始动画的间隔（毫秒）。默认 60。
   */
  uint32_t char_delay_ms;

  /**
   * @property {uint32_t} char_anim_ms
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 单个字符跳动动画时长（毫秒）。默认 300。
   */
  uint32_t char_anim_ms;

  /**
   * @property {uint32_t} frame_interval_ms
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 帧刷新间隔（毫秒）。默认 16。
   */
  uint32_t frame_interval_ms;

  /**
   * @property {bool_t} line_wrap
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 是否自动换行（默认 TRUE）。
   */
  bool_t line_wrap;

  /* ---------- 内部状态，不序列化 ---------- */
  jump_label_state_t anim_state;
  uint32_t timer_id;
  uint64_t anim_start_ms;
  /* 内部存储的 wchar_t 文本副本及长度 */
  wchar_t* chars;
  uint32_t char_count;
} jump_label_t;

widget_t* jump_label_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h);
widget_t* jump_label_cast(widget_t* widget);

/**
 * 触发清除动画（FILO 顺序逐个跳出消失）。
 * 动画结束后文本被清空。
 */
ret_t jump_label_clear(widget_t* widget);

ret_t jump_label_set_jump_height(widget_t* widget, int32_t jump_height);
ret_t jump_label_set_char_delay_ms(widget_t* widget, uint32_t char_delay_ms);
ret_t jump_label_set_char_anim_ms(widget_t* widget, uint32_t char_anim_ms);
ret_t jump_label_set_frame_interval_ms(widget_t* widget, uint32_t frame_interval_ms);
ret_t jump_label_set_line_wrap(widget_t* widget, bool_t line_wrap);

/* 缓动函数：弹跳 ease-out */
float_t jump_label_ease_out_bounce(float_t t);

#define JUMP_LABEL_PROP_JUMP_HEIGHT      "jump_height"
#define JUMP_LABEL_PROP_CHAR_DELAY_MS    "char_delay_ms"
#define JUMP_LABEL_PROP_CHAR_ANIM_MS     "char_anim_ms"
#define JUMP_LABEL_PROP_FRAME_INTERVAL_MS "frame_interval_ms"
#define JUMP_LABEL_PROP_LINE_WRAP        "line_wrap"

#define WIDGET_TYPE_JUMP_LABEL "jump_label"

#define JUMP_LABEL(widget) ((jump_label_t*)(jump_label_cast(WIDGET(widget))))

TK_EXTERN_VTABLE(jump_label);

END_C_DECLS

#endif /*TK_JUMP_LABEL_H*/
