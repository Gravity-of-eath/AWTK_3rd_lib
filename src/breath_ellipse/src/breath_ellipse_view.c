#include "breath_ellipse_view.h"

#include <ctype.h>
#include <stdio.h>

#include "base/vg_gradient.h"
#include "base/vgcanvas.h"
#include "tkc/color_parser.h"
#include "tkc/mem.h"
#include "tkc/time_now.h"
#include "tkc/timer_manager.h"
#include "tkc/utils.h"

#define BREATH_ELLIPSE_VIEW_COLOR_BLEND_WINDOW 0.33f

static ret_t breath_ellipse_view_normalize_scale_range(float_t* min_scale, float_t* max_scale) {
  float_t min_v = tk_clamp(*min_scale, 0.1f, 3.0f);
  float_t max_v = tk_clamp(*max_scale, 0.1f, 3.0f);
  if (max_v < min_v) {
    float_t tmp = min_v;
    min_v = max_v;
    max_v = tmp;
  }
  *min_scale = min_v;
  *max_scale = max_v;
  return RET_OK;
}

static bool_t breath_ellipse_view_is_color_separator(char c) {
  return c == ',' || c == ';' || c == '|';
}

static void breath_ellipse_view_clear_center_colors(breath_ellipse_view_t* view) {
  if (view->center_colors != NULL) {
    TKMEM_FREE(view->center_colors);
    view->center_colors = NULL;
  }
  view->center_color_count = 0;
}

static uint32_t breath_ellipse_view_count_color_tokens(const char* center_color) {
  const char* p = center_color;
  uint32_t count = 0;
  bool_t in_token = FALSE;

  while (*p != '\0') {
    if (breath_ellipse_view_is_color_separator(*p)) {
      if (in_token) {
        count++;
        in_token = FALSE;
      }
    } else if (!isspace((unsigned char)(*p))) {
      in_token = TRUE;
    }
    p++;
  }

  if (in_token) {
    count++;
  }

  return count;
}

static ret_t breath_ellipse_view_parse_center_colors(const char* center_color, color_t** colors,
                                                     uint32_t* color_count) {
  uint32_t capacity = 0;
  uint32_t index = 0;
  char* copy = NULL;
  char* cursor = NULL;
  color_t* parsed = NULL;
  return_value_if_fail(center_color != NULL && colors != NULL && color_count != NULL, RET_BAD_PARAMS);

  capacity = breath_ellipse_view_count_color_tokens(center_color);
  printf("breath_ellipse_view: parse center_color=\"%s\" token_capacity=%u\n", center_color, capacity);
  return_value_if_fail(capacity > 0, RET_BAD_PARAMS);

  copy = tk_strdup(center_color);
  return_value_if_fail(copy != NULL, RET_OOM);

  parsed = TKMEM_ZALLOCN(color_t, capacity);
  if (parsed == NULL) {
    TKMEM_FREE(copy);
    return RET_OOM;
  }

  cursor = copy;
  while (*cursor != '\0' && index < capacity) {
    char* start = NULL;
    char* end = NULL;
    while (*cursor != '\0' &&
           (breath_ellipse_view_is_color_separator(*cursor) || isspace((unsigned char)(*cursor)))) {
      cursor++;
    }
    if (*cursor == '\0') {
      break;
    }

    start = cursor;
    while (*cursor != '\0' && !breath_ellipse_view_is_color_separator(*cursor)) {
      cursor++;
    }
    end = cursor - 1;
    while (end >= start && isspace((unsigned char)(*end))) {
      *end = '\0';
      end--;
    }
    if (*cursor != '\0') {
      *cursor = '\0';
      cursor++;
    }
    if (*start == '\0') {
      continue;
    }
    parsed[index++] = color_parse(start);
    printf("breath_ellipse_view: token[%u]=\"%s\" rgba=(%u,%u,%u,%u)\n", index - 1, start,
              parsed[index - 1].rgba.r, parsed[index - 1].rgba.g, parsed[index - 1].rgba.b,
              parsed[index - 1].rgba.a);
  }

  TKMEM_FREE(copy);

  if (index == 0) {
    TKMEM_FREE(parsed);
    return RET_BAD_PARAMS;
  }

  *colors = parsed;
  *color_count = index;
  printf("breath_ellipse_view: parsed center_color_count=%u\n", index);
  return RET_OK;
}

static ret_t breath_ellipse_view_update_scale(breath_ellipse_view_t* view, uint64_t now) {
  uint32_t duration_ms = breath_ellipse_view_effective_duration_ms(view->frequency_bpm, view->duration_ms);
  uint64_t base = now - view->start_time_ms - view->paused_total_ms;
  view->current_cycle = base / duration_ms;
  float_t phase = (float_t)(base % duration_ms) / (float_t)duration_ms;
  view->current_phase = phase;
  view->current_scale = breath_ellipse_view_eval_scale(view->min_scale, view->max_scale, phase);
  return RET_OK;
}

static ret_t breath_ellipse_view_on_timer(const timer_info_t* timer) {
  widget_t* widget = WIDGET(timer->ctx);
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  uint64_t now = time_now_ms();

  if (!view->running) {
    view->timer_id = 0;
    return RET_REMOVE;
  }

  if (!view->paused) {
    breath_ellipse_view_update_scale(view, now);
    view->frame_count++;
    if (view->fps_sample_start_ms == 0) {
      view->fps_sample_start_ms = now;
      view->frame_count = 0;
    } else if (now - view->fps_sample_start_ms >= 1000) {
      view->fps = breath_ellipse_view_calc_fps(view->frame_count, (uint32_t)(now - view->fps_sample_start_ms));
      view->fps_sample_start_ms = now;
      view->frame_count = 0;
    }
    widget_invalidate_force(widget, NULL);
  }

  return RET_REPEAT;
}

static ret_t breath_ellipse_view_draw(widget_t* widget, canvas_t* c) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  vgcanvas_t* vg = canvas_get_vgcanvas(c);
  color_t inner_color = breath_ellipse_view_eval_color_near_min(
      view->center_colors, view->center_color_count, view->current_cycle, view->current_phase,
      BREATH_ELLIPSE_VIEW_COLOR_BLEND_WINDOW);
  color_t mid_color_1 = inner_color;
  color_t mid_color_2 = inner_color;
  color_t mid_color_3 = inner_color;
  color_t outer_color = inner_color;
  float_t cx = (float_t)widget->w * 0.5f;
  float_t cy = (float_t)widget->h * 0.5f;
  float_t fit_scale = tk_max(view->max_scale, 0.1f);
  float_t base_rx = ((float_t)widget->w * 0.5f) / fit_scale;
  float_t base_ry = ((float_t)widget->h * 0.5f) / fit_scale;
  float_t rx = tk_max(base_rx * view->current_scale, 1.0f);
  float_t ry = tk_max(base_ry * view->current_scale, 1.0f);

  mid_color_1.rgba.a = (uint8_t)(inner_color.rgba.a * 0.78f);
  mid_color_2.rgba.a = (uint8_t)(inner_color.rgba.a * 0.42f);
  mid_color_3.rgba.a = (uint8_t)(inner_color.rgba.a * 0.14f);
  outer_color.rgba.a = (uint8_t)(inner_color.rgba.a * 0.01f);

  if (vg == NULL) {
    canvas_set_fill_color(c, inner_color);
    canvas_fill_rect(c, 0, 0, widget->w, widget->h);
    return RET_OK;
  }

  {
    vg_gradient_t gradient;
    vg_gradient_init_radial(&gradient, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    vg_gradient_add_stop(&gradient, inner_color, 0.0f);
    vg_gradient_add_stop(&gradient, mid_color_1, 0.38f);
    vg_gradient_add_stop(&gradient, mid_color_2, 0.68f);
    vg_gradient_add_stop(&gradient, mid_color_3, 0.88f);
    vg_gradient_add_stop(&gradient, outer_color, 1.0f);

    vgcanvas_save(vg);
    vgcanvas_translate(vg, c->ox + cx, c->oy + cy);
    vgcanvas_scale(vg, rx, ry);
    vgcanvas_begin_path(vg);
    vgcanvas_ellipse(vg, 0.0f, 0.0f, 1.0f, 1.0f);
    vgcanvas_set_fill_gradient(vg, &gradient);
    vgcanvas_fill(vg);
    vgcanvas_restore(vg);
  }

  return RET_OK;
}

ret_t breath_ellipse_view_set_center_color(widget_t* widget, const char* center_color) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  color_t* parsed = NULL;
  uint32_t parsed_count = 0;
  char* copied = NULL;
  return_value_if_fail(view != NULL && center_color != NULL, RET_BAD_PARAMS);
  printf("breath_ellipse_view: set center_color=\"%s\"\n", center_color);

  return_value_if_fail(breath_ellipse_view_parse_center_colors(center_color, &parsed, &parsed_count) == RET_OK,
                       RET_BAD_PARAMS);

  copied = tk_strdup(center_color);
  if (copied == NULL) {
    TKMEM_FREE(parsed);
    return RET_OOM;
  }

  breath_ellipse_view_clear_center_colors(view);
  view->center_colors = parsed;
  view->center_color_count = parsed_count;
  TKMEM_FREE(view->center_color);
  view->center_color = copied;
  if (view->center_color_count <= 1) {
    printf("breath_ellipse_view: center_color_count=%u, 颜色轮换不会生效\n", view->center_color_count);
  } else {
    printf("breath_ellipse_view: center_color_count=%u, 颜色轮换已启用\n", view->center_color_count);
  }
  widget_invalidate_force(widget, NULL);
  return RET_OK;
}

ret_t breath_ellipse_view_set_frequency_bpm(widget_t* widget, float_t frequency_bpm) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  return_value_if_fail(view != NULL, RET_BAD_PARAMS);
  view->frequency_bpm = tk_max(frequency_bpm, 1.0f);
  return RET_OK;
}

ret_t breath_ellipse_view_set_scale_range(widget_t* widget, float_t min_scale, float_t max_scale) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  return_value_if_fail(view != NULL, RET_BAD_PARAMS);
  breath_ellipse_view_normalize_scale_range(&min_scale, &max_scale);
  view->min_scale = min_scale;
  view->max_scale = max_scale;
  view->current_scale = tk_clamp(view->current_scale, view->min_scale, view->max_scale);
  widget_invalidate_force(widget, NULL);
  return RET_OK;
}

ret_t breath_ellipse_view_set_min_scale(widget_t* widget, float_t min_scale) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  return_value_if_fail(view != NULL, RET_BAD_PARAMS);
  return breath_ellipse_view_set_scale_range(widget, min_scale, view->max_scale);
}

ret_t breath_ellipse_view_set_max_scale(widget_t* widget, float_t max_scale) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  return_value_if_fail(view != NULL, RET_BAD_PARAMS);
  return breath_ellipse_view_set_scale_range(widget, view->min_scale, max_scale);
}

ret_t breath_ellipse_view_set_duration_ms(widget_t* widget, uint32_t duration_ms) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  return_value_if_fail(view != NULL, RET_BAD_PARAMS);
  view->duration_ms = duration_ms == 0 ? 0 : tk_max(duration_ms, 200);
  return RET_OK;
}

ret_t breath_ellipse_view_set_frame_interval_ms(widget_t* widget, uint32_t frame_interval_ms) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  return_value_if_fail(view != NULL, RET_BAD_PARAMS);
  view->frame_interval_ms = tk_clamp(frame_interval_ms, 8, 100);
  if (view->timer_id != 0) {
    timer_remove(view->timer_id);
    view->timer_id = widget_add_timer(widget, breath_ellipse_view_on_timer, view->frame_interval_ms);
  }
  return RET_OK;
}

ret_t breath_ellipse_view_start(widget_t* widget) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  uint64_t now = time_now_ms();
  return_value_if_fail(view != NULL, RET_BAD_PARAMS);
  view->running = TRUE;
  view->paused = FALSE;
  view->start_time_ms = now;
  view->pause_time_ms = 0;
  view->paused_total_ms = 0;
  view->frame_count = 0;
  view->fps_sample_start_ms = now;
  view->fps = 0.0f;
  view->current_scale = view->min_scale;
  view->current_phase = 0.0f;
  view->current_cycle = 0;
  printf("breath_ellipse_view: start center_color_count=%u center_color=\"%s\"\n", view->center_color_count,
           view->center_color != NULL ? view->center_color : "");
  if (view->timer_id == 0) {
    view->timer_id = widget_add_timer(widget, breath_ellipse_view_on_timer, view->frame_interval_ms);
  }
  widget_invalidate_force(widget, NULL);
  return RET_OK;
}

ret_t breath_ellipse_view_pause(widget_t* widget) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  return_value_if_fail(view != NULL, RET_BAD_PARAMS);
  if (!view->running || view->paused) {
    return RET_OK;
  }
  view->paused = TRUE;
  view->pause_time_ms = time_now_ms();
  return RET_OK;
}

ret_t breath_ellipse_view_resume(widget_t* widget) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  uint64_t now = time_now_ms();
  return_value_if_fail(view != NULL, RET_BAD_PARAMS);
  if (!view->running || !view->paused) {
    return RET_OK;
  }
  view->paused = FALSE;
  if (now > view->pause_time_ms) {
    view->paused_total_ms += (now - view->pause_time_ms);
  }
  return RET_OK;
}

ret_t breath_ellipse_view_stop(widget_t* widget) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  return_value_if_fail(view != NULL, RET_BAD_PARAMS);
  view->running = FALSE;
  view->paused = FALSE;
  view->current_scale = view->min_scale;
  view->current_phase = 0.0f;
  view->current_cycle = 0;
  view->fps = 0.0f;
  if (view->timer_id != 0) {
    timer_remove(view->timer_id);
    view->timer_id = 0;
  }
  widget_invalidate_force(widget, NULL);
  return RET_OK;
}

float_t breath_ellipse_view_get_current_scale(widget_t* widget) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  return_value_if_fail(view != NULL, 0.0f);
  return view->current_scale;
}

float_t breath_ellipse_view_get_fps(widget_t* widget) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  return_value_if_fail(view != NULL, 0.0f);
  return view->fps;
}

bool_t breath_ellipse_view_is_running(widget_t* widget) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  return_value_if_fail(view != NULL, FALSE);
  return view->running;
}

bool_t breath_ellipse_view_is_paused(widget_t* widget) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  return_value_if_fail(view != NULL, FALSE);
  return view->paused;
}

static ret_t breath_ellipse_view_get_prop(widget_t* widget, const char* name, value_t* v) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  return_value_if_fail(view != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(BREATH_ELLIPSE_VIEW_PROP_CENTER_COLOR, name)) {
    value_set_str(v, view->center_color);
    return RET_OK;
  } else if (tk_str_eq(BREATH_ELLIPSE_VIEW_PROP_CENTER_COLORS, name)) {
    value_set_str(v, view->center_color);
    return RET_OK;
  } else if (tk_str_eq(BREATH_ELLIPSE_VIEW_PROP_FREQUENCY_BPM, name)) {
    value_set_float32(v, view->frequency_bpm);
    return RET_OK;
  } else if (tk_str_eq(BREATH_ELLIPSE_VIEW_PROP_MIN_SCALE, name)) {
    value_set_float32(v, view->min_scale);
    return RET_OK;
  } else if (tk_str_eq(BREATH_ELLIPSE_VIEW_PROP_MAX_SCALE, name)) {
    value_set_float32(v, view->max_scale);
    return RET_OK;
  } else if (tk_str_eq(BREATH_ELLIPSE_VIEW_PROP_DURATION_MS, name)) {
    value_set_uint32(v, view->duration_ms);
    return RET_OK;
  } else if (tk_str_eq(BREATH_ELLIPSE_VIEW_PROP_FRAME_INTERVAL_MS, name)) {
    value_set_uint32(v, view->frame_interval_ms);
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

static ret_t breath_ellipse_view_set_prop(widget_t* widget, const char* name, const value_t* v) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  return_value_if_fail(view != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(BREATH_ELLIPSE_VIEW_PROP_CENTER_COLOR, name)) {
    return breath_ellipse_view_set_center_color(widget, value_str(v));
  } else if (tk_str_eq(BREATH_ELLIPSE_VIEW_PROP_CENTER_COLORS, name)) {
    return breath_ellipse_view_set_center_color(widget, value_str(v));
  } else if (tk_str_eq(BREATH_ELLIPSE_VIEW_PROP_FREQUENCY_BPM, name)) {
    return breath_ellipse_view_set_frequency_bpm(widget, value_float32(v));
  } else if (tk_str_eq(BREATH_ELLIPSE_VIEW_PROP_MIN_SCALE, name)) {
    return breath_ellipse_view_set_min_scale(widget, value_float32(v));
  } else if (tk_str_eq(BREATH_ELLIPSE_VIEW_PROP_MAX_SCALE, name)) {
    return breath_ellipse_view_set_max_scale(widget, value_float32(v));
  } else if (tk_str_eq(BREATH_ELLIPSE_VIEW_PROP_DURATION_MS, name)) {
    return breath_ellipse_view_set_duration_ms(widget, value_uint32(v));
  } else if (tk_str_eq(BREATH_ELLIPSE_VIEW_PROP_FRAME_INTERVAL_MS, name)) {
    return breath_ellipse_view_set_frame_interval_ms(widget, value_uint32(v));
  }

  return RET_NOT_FOUND;
}

static ret_t breath_ellipse_view_on_destroy(widget_t* widget) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  return_value_if_fail(view != NULL, RET_BAD_PARAMS);
  breath_ellipse_view_stop(widget);
  breath_ellipse_view_clear_center_colors(view);
  TKMEM_FREE(view->center_color);
  view->center_color = NULL;
  return RET_OK;
}

static ret_t breath_ellipse_view_on_event(widget_t* widget, event_t* e) {
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  return_value_if_fail(view != NULL && e != NULL, RET_BAD_PARAMS);

  if (e->type == EVT_WIDGET_LOAD && !view->running) {
    breath_ellipse_view_start(widget);
  }

  return RET_OK;
}

static ret_t breath_ellipse_view_on_paint_self(widget_t* widget, canvas_t* c) {
  return breath_ellipse_view_draw(widget, c);
}

const char* s_breath_ellipse_view_properties[] = {
    BREATH_ELLIPSE_VIEW_PROP_CENTER_COLOR,
    BREATH_ELLIPSE_VIEW_PROP_CENTER_COLORS,
    BREATH_ELLIPSE_VIEW_PROP_FREQUENCY_BPM,
    BREATH_ELLIPSE_VIEW_PROP_MIN_SCALE,
    BREATH_ELLIPSE_VIEW_PROP_MAX_SCALE,
    BREATH_ELLIPSE_VIEW_PROP_DURATION_MS,
    BREATH_ELLIPSE_VIEW_PROP_FRAME_INTERVAL_MS,
    NULL};

TK_DECL_VTABLE(breath_ellipse_view) = {.size = sizeof(breath_ellipse_view_t),
                                       .type = WIDGET_TYPE_BREATH_ELLIPSE_VIEW,
                                       .clone_properties = s_breath_ellipse_view_properties,
                                       .persistent_properties = s_breath_ellipse_view_properties,
                                       .parent = TK_PARENT_VTABLE(widget),
                                       .create = breath_ellipse_view_create,
                                       .on_paint_self = breath_ellipse_view_on_paint_self,
                                       .on_event = breath_ellipse_view_on_event,
                                       .set_prop = breath_ellipse_view_set_prop,
                                       .get_prop = breath_ellipse_view_get_prop,
                                       .on_destroy = breath_ellipse_view_on_destroy};

widget_t* breath_ellipse_view_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h) {
  widget_t* widget = widget_create(parent, TK_REF_VTABLE(breath_ellipse_view), x, y, w, h);
  breath_ellipse_view_t* view = BREATH_ELLIPSE_VIEW(widget);
  return_value_if_fail(view != NULL, NULL);
  view->center_color = NULL;
  view->frequency_bpm = 12.0f;
  view->min_scale = 0.8f;
  view->max_scale = 1.2f;
  view->duration_ms = 0;
  view->frame_interval_ms = 16;
  view->current_scale = 0.8f;
  view->current_phase = 0.0f;
  view->current_cycle = 0;
  view->center_colors = NULL;
  view->center_color_count = 0;
  view->timer_id = 0;
  view->running = FALSE;
  view->paused = FALSE;
  view->start_time_ms = 0;
  view->pause_time_ms = 0;
  view->paused_total_ms = 0;
  view->frame_count = 0;
  view->fps_sample_start_ms = 0;
  view->fps = 0.0f;
  breath_ellipse_view_set_center_color(widget, "#00B511");
  return widget;
}

widget_t* breath_ellipse_view_cast(widget_t* widget) {
  return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, breath_ellipse_view), NULL);
  return widget;
}
