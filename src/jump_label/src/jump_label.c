#include "jump_label.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "tkc/mem.h"
#include "tkc/time_now.h"
#include "tkc/utils.h"

/* ========== 内部工具 ========== */

static void jump_label_free_chars(jump_label_t* label) {
  if (label->chars != NULL) {
    TKMEM_FREE(label->chars);
    label->chars = NULL;
  }
  label->char_count = 0;
}

static void jump_label_stop_timer(jump_label_t* label) {
  if (label->timer_id != 0) {
    timer_remove(label->timer_id);
    label->timer_id = 0;
  }
}

/* 计算动画总时长 */
static uint32_t jump_label_total_anim_ms(jump_label_t* label) {
  if (label->char_count == 0) return 0;
  return label->char_delay_ms * (label->char_count - 1) + label->char_anim_ms;
}

/* ========== 定时器回调 ========== */

static ret_t jump_label_on_timer(const timer_info_t* timer) {
  widget_t* widget = WIDGET(timer->ctx);
  jump_label_t* label = JUMP_LABEL(widget);
  uint64_t now = time_now_ms();
  uint32_t total = jump_label_total_anim_ms(label);
  uint64_t elapsed = now - label->anim_start_ms;

  if (elapsed >= total) {
    if (label->anim_state == JUMP_LABEL_STATE_SHOWING) {
      label->anim_state = JUMP_LABEL_STATE_SHOWN;
    } else if (label->anim_state == JUMP_LABEL_STATE_CLEARING) {
      label->anim_state = JUMP_LABEL_STATE_IDLE;
      jump_label_free_chars(label);
      widget_set_text(widget, L"");
    }
    jump_label_stop_timer(label);
    widget_invalidate_force(widget, NULL);
    return RET_REMOVE;
  }

  widget_invalidate_force(widget, NULL);
  return RET_REPEAT;
}

static void jump_label_start_timer(jump_label_t* label, widget_t* widget) {
  label->anim_start_ms = time_now_ms();
  if (label->timer_id == 0) {
    label->timer_id = widget_add_timer(widget, jump_label_on_timer, label->frame_interval_ms);
  }
}

/* ========== 绘制 ========== */

/*
 * 逐字符绘制。每个字符根据动画进度有独立的 y 偏移和 alpha。
 * SHOWING  : 第 i 个字符的进度 = clamp((elapsed - i*delay) / anim_ms, 0, 1)
 * CLEARING : FILO，最后一个字符先退出 => 第 i 个字符对应倒序索引
 */
static ret_t jump_label_paint_chars(widget_t* widget, canvas_t* c) {
  jump_label_t* label = JUMP_LABEL(widget);
  uint32_t i;
  int32_t margin = 0;
  int32_t margin_left = 0;
  int32_t margin_top = 0;
  int32_t margin_right = 0;
  int32_t margin_bottom = 0;
  int32_t spacer = 0;
  uint16_t font_size = 0;
  style_t* style = widget->astyle;
  int32_t line_height;
  int32_t content_w;
  int32_t x_cursor, y_cursor;
  uint64_t now;
  uint64_t elapsed;
  color_t text_color;

  return_value_if_fail(label != NULL && label->chars != NULL && label->char_count > 0, RET_OK);
  return_value_if_fail(style_is_valid(style), RET_OK);

  widget_prepare_text_style(widget, c);
  font_size = c->font_size;

  margin = style_get_int(style, STYLE_ID_MARGIN, 0);
  margin_left = style_get_int(style, STYLE_ID_MARGIN_LEFT, margin);
  margin_top = style_get_int(style, STYLE_ID_MARGIN_TOP, margin);
  margin_right = style_get_int(style, STYLE_ID_MARGIN_RIGHT, margin);
  margin_bottom = style_get_int(style, STYLE_ID_MARGIN_BOTTOM, margin);
  spacer = style_get_int(style, STYLE_ID_SPACER, 2);
  line_height = font_size + spacer;
  content_w = widget->w - margin_left - margin_right;

  text_color = c->lcd->text_color;
  now = time_now_ms();
  elapsed = (label->anim_state == JUMP_LABEL_STATE_SHOWING ||
             label->anim_state == JUMP_LABEL_STATE_CLEARING)
                ? (now - label->anim_start_ms)
                : 0xFFFFFFFF; /* 已完成，progress = 1.0 */

  x_cursor = margin_left;
  y_cursor = margin_top;

  for (i = 0; i < label->char_count; i++) {
    wchar_t ch = label->chars[i];
    float_t char_w;
    float_t progress;
    float_t ease_val;
    int32_t y_offset;
    uint8_t alpha;
    uint32_t anim_index;

    /* 处理换行符 */
    if (ch == L'\n' || ch == L'\r') {
      if (ch == L'\r' && i + 1 < label->char_count && label->chars[i + 1] == L'\n') {
        i++; /* 跳过 \r\n 中的 \n */
      }
      x_cursor = margin_left;
      y_cursor += line_height;
      continue;
    }

    char_w = canvas_measure_text(c, &ch, 1);

    /* 自动换行 */
    if (label->line_wrap && x_cursor + (int32_t)char_w > widget->w - margin_right &&
        x_cursor > margin_left) {
      x_cursor = margin_left;
      y_cursor += line_height;
    }

    /* 超出底部不绘制 */
    if (y_cursor + font_size > widget->h - margin_bottom) {
      break;
    }

    /* 计算该字符的动画进度 */
    if (label->anim_state == JUMP_LABEL_STATE_SHOWING) {
      uint32_t start_ms = i * label->char_delay_ms;
      if (elapsed <= start_ms) {
        progress = 0.0f;
      } else {
        progress = (float_t)(elapsed - start_ms) / (float_t)label->char_anim_ms;
        if (progress > 1.0f) progress = 1.0f;
      }
    } else if (label->anim_state == JUMP_LABEL_STATE_CLEARING) {
      /* FILO: 最后一个字符最先退出 */
      anim_index = (label->char_count - 1) - i;
      uint32_t start_ms = anim_index * label->char_delay_ms;
      if (elapsed <= start_ms) {
        progress = 0.0f;
      } else {
        progress = (float_t)(elapsed - start_ms) / (float_t)label->char_anim_ms;
        if (progress > 1.0f) progress = 1.0f;
      }
      /* clearing 时进度反向：1->0 表示从就位到跳出 */
      progress = 1.0f - progress;
    } else {
      /* SHOWN 或 IDLE */
      progress = (label->anim_state == JUMP_LABEL_STATE_SHOWN) ? 1.0f : 0.0f;
    }

    ease_val = jump_label_ease_out_bounce(progress);
    y_offset = (int32_t)((1.0f - ease_val) * label->jump_height);
    alpha = (uint8_t)(ease_val * text_color.rgba.a);

    if (alpha > 0) {
      color_t draw_color = text_color;
      draw_color.rgba.a = alpha;
      canvas_set_text_color(c, draw_color);
      canvas_draw_text(c, &ch, 1, x_cursor, y_cursor + y_offset);
    }

    x_cursor += (int32_t)char_w;
  }

  /* 恢复原始颜色 */
  canvas_set_text_color(c, text_color);
  return RET_OK;
}

/* ========== VTABLE 回调 ========== */

static ret_t jump_label_on_paint_self(widget_t* widget, canvas_t* c) {
  jump_label_t* label = JUMP_LABEL(widget);
  if (label->chars != NULL && label->char_count > 0) {
    return jump_label_paint_chars(widget, c);
  }
  return RET_OK;
}

static ret_t jump_label_on_event(widget_t* widget, event_t* e) {
  (void)widget;
  (void)e;
  return RET_OK;
}

static ret_t jump_label_on_destroy(widget_t* widget) {
  jump_label_t* label = JUMP_LABEL(widget);
  return_value_if_fail(label != NULL, RET_BAD_PARAMS);
  jump_label_stop_timer(label);
  jump_label_free_chars(label);
  return RET_OK;
}

static ret_t jump_label_get_prop(widget_t* widget, const char* name, value_t* v) {
  jump_label_t* label = JUMP_LABEL(widget);
  return_value_if_fail(label != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(JUMP_LABEL_PROP_JUMP_HEIGHT, name)) {
    value_set_int32(v, label->jump_height);
    return RET_OK;
  } else if (tk_str_eq(JUMP_LABEL_PROP_CHAR_DELAY_MS, name)) {
    value_set_uint32(v, label->char_delay_ms);
    return RET_OK;
  } else if (tk_str_eq(JUMP_LABEL_PROP_CHAR_ANIM_MS, name)) {
    value_set_uint32(v, label->char_anim_ms);
    return RET_OK;
  } else if (tk_str_eq(JUMP_LABEL_PROP_FRAME_INTERVAL_MS, name)) {
    value_set_uint32(v, label->frame_interval_ms);
    return RET_OK;
  } else if (tk_str_eq(JUMP_LABEL_PROP_LINE_WRAP, name)) {
    value_set_bool(v, label->line_wrap);
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

static ret_t jump_label_set_prop(widget_t* widget, const char* name, const value_t* v) {
  return_value_if_fail(widget != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(JUMP_LABEL_PROP_JUMP_HEIGHT, name)) {
    return jump_label_set_jump_height(widget, value_int32(v));
  } else if (tk_str_eq(JUMP_LABEL_PROP_CHAR_DELAY_MS, name)) {
    return jump_label_set_char_delay_ms(widget, value_uint32(v));
  } else if (tk_str_eq(JUMP_LABEL_PROP_CHAR_ANIM_MS, name)) {
    return jump_label_set_char_anim_ms(widget, value_uint32(v));
  } else if (tk_str_eq(JUMP_LABEL_PROP_FRAME_INTERVAL_MS, name)) {
    return jump_label_set_frame_interval_ms(widget, value_uint32(v));
  } else if (tk_str_eq(JUMP_LABEL_PROP_LINE_WRAP, name)) {
    return jump_label_set_line_wrap(widget, value_bool(v));
  } else if (tk_str_eq(WIDGET_PROP_TEXT, name)) {
    /* 拦截 set_text，触发显示动画 */
    const char* text_utf8 = value_str(v);
    if (text_utf8 != NULL && *text_utf8 != '\0') {
      wstr_t tmp;
      wstr_init(&tmp, strlen(text_utf8) + 1);
      wstr_set_utf8(&tmp, text_utf8);

      jump_label_stop_timer(JUMP_LABEL(widget));
      jump_label_free_chars(JUMP_LABEL(widget));

      JUMP_LABEL(widget)->chars = tmp.str;
      JUMP_LABEL(widget)->char_count = tmp.size;
      /* 不要 wstr_reset，我们接管了 tmp.str 的所有权 */

      /* 同步到 widget->text 供框架使用 */
      wstr_set(&(widget->text), tmp.str);

      JUMP_LABEL(widget)->anim_state = JUMP_LABEL_STATE_SHOWING;
      jump_label_start_timer(JUMP_LABEL(widget), widget);
    } else {
      jump_label_clear(widget);
    }
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

/* ========== 公开 API ========== */

ret_t jump_label_set_jump_height(widget_t* widget, int32_t jump_height) {
  jump_label_t* label = JUMP_LABEL(widget);
  return_value_if_fail(label != NULL, RET_BAD_PARAMS);
  label->jump_height = tk_max(jump_height, 1);
  return RET_OK;
}

ret_t jump_label_set_char_delay_ms(widget_t* widget, uint32_t char_delay_ms) {
  jump_label_t* label = JUMP_LABEL(widget);
  return_value_if_fail(label != NULL, RET_BAD_PARAMS);
  label->char_delay_ms = tk_max(char_delay_ms, 1);
  return RET_OK;
}

ret_t jump_label_set_char_anim_ms(widget_t* widget, uint32_t char_anim_ms) {
  jump_label_t* label = JUMP_LABEL(widget);
  return_value_if_fail(label != NULL, RET_BAD_PARAMS);
  label->char_anim_ms = tk_max(char_anim_ms, 50);
  return RET_OK;
}

ret_t jump_label_set_frame_interval_ms(widget_t* widget, uint32_t frame_interval_ms) {
  jump_label_t* label = JUMP_LABEL(widget);
  return_value_if_fail(label != NULL, RET_BAD_PARAMS);
  label->frame_interval_ms = tk_clamp(frame_interval_ms, 8, 100);
  if (label->timer_id != 0) {
    timer_remove(label->timer_id);
    label->timer_id = widget_add_timer(widget, jump_label_on_timer, label->frame_interval_ms);
  }
  return RET_OK;
}

ret_t jump_label_set_line_wrap(widget_t* widget, bool_t line_wrap) {
  jump_label_t* label = JUMP_LABEL(widget);
  return_value_if_fail(label != NULL, RET_BAD_PARAMS);
  label->line_wrap = line_wrap;
  return RET_OK;
}

ret_t jump_label_clear(widget_t* widget) {
  jump_label_t* label = JUMP_LABEL(widget);
  return_value_if_fail(label != NULL, RET_BAD_PARAMS);

  if (label->char_count == 0) {
    return RET_OK;
  }

  jump_label_stop_timer(label);
  label->anim_state = JUMP_LABEL_STATE_CLEARING;
  jump_label_start_timer(label, widget);
  return RET_OK;
}

/* ========== 属性列表 / VTABLE / 构造 ========== */

static const char* s_jump_label_properties[] = {
    JUMP_LABEL_PROP_JUMP_HEIGHT,
    JUMP_LABEL_PROP_CHAR_DELAY_MS,
    JUMP_LABEL_PROP_CHAR_ANIM_MS,
    JUMP_LABEL_PROP_FRAME_INTERVAL_MS,
    JUMP_LABEL_PROP_LINE_WRAP,
    NULL};

TK_DECL_VTABLE(jump_label) = {
    .size = sizeof(jump_label_t),
    .type = WIDGET_TYPE_JUMP_LABEL,
    .clone_properties = s_jump_label_properties,
    .persistent_properties = s_jump_label_properties,
    .parent = TK_PARENT_VTABLE(widget),
    .create = jump_label_create,
    .on_paint_self = jump_label_on_paint_self,
    .set_prop = jump_label_set_prop,
    .get_prop = jump_label_get_prop,
    .on_event = jump_label_on_event,
    .on_destroy = jump_label_on_destroy};

widget_t* jump_label_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h) {
  widget_t* widget = widget_create(parent, TK_REF_VTABLE(jump_label), x, y, w, h);
  jump_label_t* label = JUMP_LABEL(widget);
  return_value_if_fail(label != NULL, NULL);

  label->jump_height = 20;
  label->char_delay_ms = 60;
  label->char_anim_ms = 300;
  label->frame_interval_ms = 16;
  label->line_wrap = TRUE;
  label->anim_state = JUMP_LABEL_STATE_IDLE;
  label->timer_id = 0;
  label->anim_start_ms = 0;
  label->chars = NULL;
  label->char_count = 0;

  return widget;
}

widget_t* jump_label_cast(widget_t* widget) {
  return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, jump_label), NULL);
  return widget;
}
