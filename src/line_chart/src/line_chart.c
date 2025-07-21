/**
 * File:   line_chart.c
 * Author:
 * Brief:
 *
 * Copyright (c) 2025 - 2025
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2025-7-17  created
 *
 */

#include "tkc/mem.h"
#include "tkc/utils.h"
#include "line_chart.h"
#include "tkc/color_parser.h"
#include "base/canvas_offline.h"
#include "stdio.h"

ret_t line_chart_set_fg_color(widget_t *widget, const char *fg_color)
{
  line_chart_t *line_chart = LINE_CHART(widget);
  return_value_if_fail(line_chart != NULL, RET_BAD_PARAMS);

  line_chart->fg_color = tk_str_copy(line_chart->fg_color, fg_color);

  return RET_OK;
}

ret_t line_chart_set_secd_color(widget_t *widget, const char *secd_color)
{
  line_chart_t *line_chart = LINE_CHART(widget);
  return_value_if_fail(line_chart != NULL, RET_BAD_PARAMS);

  line_chart->secd_color = tk_str_copy(line_chart->secd_color, secd_color);

  return RET_OK;
}

ret_t line_chart_set_guide_line_color(widget_t *widget, const char *guide_line_color)
{
  line_chart_t *line_chart = LINE_CHART(widget);
  return_value_if_fail(line_chart != NULL, RET_BAD_PARAMS);

  line_chart->guide_line_color = tk_str_copy(line_chart->guide_line_color, guide_line_color);

  return RET_OK;
}

ret_t line_chart_set_mode(widget_t *widget, int32_t mode)
{
  line_chart_t *line_chart = LINE_CHART(widget);
  return_value_if_fail(line_chart != NULL, RET_BAD_PARAMS);

  line_chart->mode = mode;

  return RET_OK;
}

ret_t line_chart_set_max_point(widget_t *widget, int32_t max_point)
{
  line_chart_t *line_chart = LINE_CHART(widget);
  return_value_if_fail(line_chart != NULL, RET_BAD_PARAMS);
  line_chart->max_point = max_point;
  float_queue_reinit(line_chart->queue, max_point);
  return RET_OK;
}

ret_t line_chart_set_divide_value(widget_t *widget, float_t divide_value)
{
  line_chart_t *line_chart = LINE_CHART(widget);
  return_value_if_fail(line_chart != NULL, RET_BAD_PARAMS);

  line_chart->divide_value = divide_value;

  return RET_OK;
}

ret_t line_chart_set_align(widget_t *widget, int32_t align)
{
  line_chart_t *line_chart = LINE_CHART(widget);
  return_value_if_fail(line_chart != NULL, RET_BAD_PARAMS);

  line_chart->align = align;

  return RET_OK;
}

ret_t line_chart_set_line_width(widget_t *widget, int32_t line_width)
{
  line_chart_t *line_chart = LINE_CHART(widget);
  return_value_if_fail(line_chart != NULL, RET_BAD_PARAMS);

  line_chart->line_width = line_width;

  return RET_OK;
}

ret_t line_chart_add_point(widget_t *widget, float_t point)
{
  line_chart_t *line_chart = LINE_CHART(widget);
  return_value_if_fail(line_chart != NULL, RET_BAD_PARAMS);
  float_queue_push_n(line_chart->queue, &point, 1);
  return RET_OK;
}

static ret_t line_chart_get_prop(widget_t *widget, const char *name, value_t *v)
{
  line_chart_t *line_chart = LINE_CHART(widget);
  return_value_if_fail(line_chart != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(LINE_CHART_PROP_FG_COLOR, name))
  {
    value_set_str(v, line_chart->fg_color);
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_SECD_COLOR, name))
  {
    value_set_str(v, line_chart->secd_color);
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_MODE, name))
  {
    value_set_int32(v, line_chart->mode);
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_MAX_POINT, name))
  {
    value_set_int32(v, line_chart->max_point);
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_DIVIDE_VALUE, name))
  {
    value_set_float(v, line_chart->divide_value);
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_ALIGN, name))
  {
    value_set_int32(v, line_chart->align);
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_LINE_WIDTH, name))
  {
    value_set_int32(v, line_chart->line_width);
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_GUIDE_LINE_OFFSET, name))
  {
    value_set_float32(v, line_chart->guide_line_offset);
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_SECD_PERCENT, name))
  {
    value_set_float32(v, line_chart->secd_percent);
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_GUIDE_LINE_PERCENT, name))
  {
    value_set_float32(v, line_chart->guide_line_percent);
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

static ret_t line_chart_set_prop(widget_t *widget, const char *name, const value_t *v)
{
  line_chart_t *line_chart = LINE_CHART(widget);
  return_value_if_fail(widget != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(LINE_CHART_PROP_FG_COLOR, name))
  {
    line_chart_set_fg_color(widget, value_str(v));
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_SECD_COLOR, name))
  {
    line_chart_set_secd_color(widget, value_str(v));
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_GUIDE_LINE_COLOR, name))
  {
    line_chart_set_guide_line_color(widget, value_str(v));
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_MODE, name))
  {
    line_chart_set_mode(widget, value_int32(v));
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_MAX_POINT, name))
  {
    line_chart_set_max_point(widget, value_int32(v));
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_DIVIDE_VALUE, name))
  {
    line_chart_set_divide_value(widget, value_float(v));
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_ALIGN, name))
  {
    line_chart_set_align(widget, value_int32(v));
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_LINE_WIDTH, name))
  {
    line_chart_set_line_width(widget, value_int32(v));
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_GUIDE_LINE_OFFSET, name))
  {
    line_chart->guide_line_offset = value_float32(v);
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_SECD_PERCENT, name))
  {
    line_chart->secd_percent = value_float32(v);
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_GUIDE_LINE_PERCENT, name))
  {
    line_chart->guide_line_percent = value_float32(v);
    return RET_OK;
  }
  else if (tk_str_eq(LINE_CHART_PROP_DEBUG, name))
  {
    line_chart->DEBUG = value_bool(v);
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

static ret_t line_chart_on_destroy(widget_t *widget)
{
  line_chart_t *line_chart = LINE_CHART(widget);
  return_value_if_fail(widget != NULL && line_chart != NULL, RET_BAD_PARAMS);

  TKMEM_FREE(line_chart->fg_color);
  TKMEM_FREE(line_chart->secd_color);

  return RET_OK;
}

static void draw_guide_line(line_chart_t *line_chart, vgcanvas_t *vgc)
{

  vgcanvas_set_stroke_color(vgc, color_parse(line_chart->guide_line_color));
  int32_t width_dest = line_chart->guide_line_percent * line_chart->widget.w;
  vgcanvas_move_to(vgc, 0, line_chart->widget.h * line_chart->guide_line_offset);
  vgcanvas_line_to(vgc, width_dest, line_chart->widget.h * line_chart->guide_line_offset);

  vgcanvas_move_to(vgc, 0, line_chart->widget.h * (1 - line_chart->guide_line_offset));
  vgcanvas_line_to(vgc, width_dest, line_chart->widget.h * (1 - line_chart->guide_line_offset));
  float_t x = 0;
  for (x = 0; x < width_dest; x += 5)
  {
    vgcanvas_move_to(vgc, x, line_chart->widget.h * 0.5f);
    x += 5;
    vgcanvas_line_to(vgc, x, line_chart->widget.h * 0.5f);
  }
  vgcanvas_stroke(vgc);
}

static void draw_chart_line(line_chart_t *line_chart, vgcanvas_t *wvgc)
{
  if (float_queue_empty(line_chart->queue))
  {
    printf("error  draw_chart_line float_queue_size=%d\n", float_queue_size(line_chart->queue));
    return;
  }

  canvas_t *oc = canvas_offline_create(line_chart->widget.w, line_chart->widget.h, BITMAP_FMT_RGBA8888);
  vgcanvas_t *vgc = canvas_get_vgcanvas(oc);
  vgcanvas_set_line_width(vgc, line_chart->line_width);
  if (line_chart->DEBUG)
  {

    vgcanvas_set_stroke_color_str(vgc, "#0000FFFF");
    vgcanvas_rect(vgc, 0, 0, line_chart->widget.w, line_chart->widget.h);
    vgcanvas_stroke(vgc);
  }
  color_t fgColor = color_parse(line_chart->fg_color);
  vgcanvas_set_stroke_color(vgc, fgColor);
  vgcanvas_set_line_cap(vgc, "round");
  float_t min = float_queue_at(line_chart->queue, 0);
  float_t max = float_queue_at(line_chart->queue, 0);
  float_t step = line_chart->widget.w / (line_chart->max_point * 1.0f);
  for (int32_t i = 1; i < float_queue_size(line_chart->queue); i++)
  {
    float_t value = float_queue_at(line_chart->queue, i);
    if (value < min)
    {
      min = value;
    }
    if (value > max)
    {
      max = value;
    }
  }
  // printf("line_chart -->> max=%f  min =%f\n", max, min);
  float_t scale_guild = max - min;

  float_t value = float_queue_at(line_chart->queue, 0);
  float_t ratio = (value - min) * 1.0f / scale_guild * 1.0f;
  int32_t useable_h = line_chart->widget.h * (1.0f - line_chart->guide_line_offset);
  int32_t y = (ratio * useable_h) + (line_chart->widget.h * line_chart->guide_line_offset / 2);
  vgcanvas_begin_path(vgc);
  vgcanvas_move_to(vgc, 0, y);
  int32_t secd_percent_widget = line_chart->secd_percent * line_chart->widget.w;
  for (int32_t i = 1; i < float_queue_size(line_chart->queue); i++)
  {
    value = float_queue_at(line_chart->queue, i);
    int32_t x = step * i;
    ratio = (value - min) * 1.0f / scale_guild * 1.0f;
    useable_h = line_chart->widget.h * (1.0f - line_chart->guide_line_offset);
    y = (ratio * useable_h) + (line_chart->widget.h * line_chart->guide_line_offset / 2);

    if (x > secd_percent_widget && x - step < secd_percent_widget)
    {
      vgcanvas_line_to(vgc, secd_percent_widget, y);
      vgcanvas_stroke(vgc);
      vgcanvas_set_stroke_color_str(vgc, line_chart->secd_color);

      vgcanvas_begin_path(vgc);
      vgcanvas_move_to(vgc, secd_percent_widget, y);
      // vgcanvas_line_to(vgc, x, y);
      // continue;
      printf("secd_percent_widget = %d x=%d\n", secd_percent_widget, x);
    }
    vgcanvas_line_to(vgc, x, y);

    // printf("line_chart -->>i=%d useable_h = %d value =%f ratio =%f x=%d  y =%d\n", i, useable_h, value, ratio, x, y);
  }
  vgcanvas_stroke(vgc);
  // printf("line_chart->secd_color =%s\n", line_chart->secd_color);

  bitmap_t *bmp = canvas_offline_get_bitmap(oc);
  uint8_t *src_data = bitmap_lock_buffer_for_write(bmp);

  // color_t fgColor = color_parse(line_chart->fg_color);
  int32_t old_color = color_get_color(&fgColor);
  // printf("fgColor  =%d str=%s\n", old_color, line_chart->fg_color);
  uint8_t old_a = (old_color >> 24) & 0xFF;
  uint8_t old_b = (old_color >> 16) & 0xFF;
  uint8_t old_g = (old_color >> 8) & 0xFF;
  uint8_t old_r = old_color & 0xFF;

  // printf("fgColor   rgb= %d-%d-%d \n", old_r, old_g, old_b);
  color_t secdColor = color_parse(line_chart->secd_color);
  int32_t new_color = color_get_color(&secdColor);
  // 解包新颜色（RGBA）
  uint8_t new_a = (new_color >> 24) & 0xFF;
  uint8_t new_b = (new_color >> 16) & 0xFF;
  uint8_t new_g = (new_color >> 8) & 0xFF;
  uint8_t new_r = new_color & 0xFF;

  // 每行字节数（RGBA8888格式）
  int32_t stride = line_chart->widget.w * 4;
  int32_t start_row = line_chart->divide_value > 1 ? line_chart->divide_value : (line_chart->divide_value * line_chart->widget.h);
  // 从指定行开始处理
  for (int32_t y = start_row; y < line_chart->widget.h; y++)
  {
    uint8_t *row_start = src_data + y * stride;

    for (int32_t x = 0; x < line_chart->widget.w; x++)
    {
      uint8_t *pixel = row_start + x * 4;

      // printf("XX pixel=%d-%d-%d  old= %d-%d-%d \n", pixel[0], pixel[1], pixel[2], old_r, old_g, old_b);
      // 如果像素不透明（alpha != 0），则修改颜色
      if (old_r == pixel[0] && old_g == pixel[1] && old_b == pixel[2])
      {
        pixel[0] = new_r; // R
        pixel[1] = new_g; // G
        pixel[2] = new_b; // B
        // pixel[3] = old_a; // A
      }
    }
  }

  vgcanvas_draw_image(wvgc, bmp, 0, 0, bmp->w, bmp->h, 0, 0, line_chart->widget.w, line_chart->widget.h);
  // printf("canvas_draw_image bmp->w=%d, bmp->h=%d  line_chart->widget.w=%d, line_chart->widget.h=%d \n", bmp->w, bmp->h, line_chart->widget.w, line_chart->widget.h);
  canvas_offline_destroy(oc);
}

static ret_t line_chart_on_paint_self(widget_t *widget, canvas_t *c)
{
  line_chart_t *line_chart = LINE_CHART(widget);
  vgcanvas_t *vgc = canvas_get_vgcanvas(c);
  // printf("line_chart_on_paint_self55555555552 -->> x=%d  y =%d w=%d  h =%d\n", widget->x, widget->y, widget->w, widget->h);

  vgcanvas_save(vgc);
  vgcanvas_translate(vgc, c->ox, c->oy);
  vgcanvas_set_antialias(vgc, TRUE);
  draw_guide_line(line_chart, vgc);
  draw_chart_line(line_chart, vgc);
  vgcanvas_restore(vgc);
  return RET_OK;
}

static ret_t line_chart_on_event(widget_t *widget, event_t *e)
{
  line_chart_t *line_chart = LINE_CHART(widget);
  return_value_if_fail(widget != NULL && line_chart != NULL, RET_BAD_PARAMS);

  (void)line_chart;

  return RET_OK;
}

const char *s_line_chart_properties[] = {
    LINE_CHART_PROP_FG_COLOR,
    LINE_CHART_PROP_SECD_COLOR,
    LINE_CHART_PROP_MODE,
    LINE_CHART_PROP_MAX_POINT,
    LINE_CHART_PROP_DIVIDE_VALUE,
    LINE_CHART_PROP_ALIGN,
    LINE_CHART_PROP_LINE_WIDTH,
    LINE_CHART_PROP_SECD_PERCENT,
    LINE_CHART_PROP_GUIDE_LINE_PERCENT,
    LINE_CHART_PROP_DEBUG,
    NULL};

TK_DECL_VTABLE(line_chart) = {.size = sizeof(line_chart_t),
                              .type = WIDGET_TYPE_LINE_CHART,
                              .clone_properties = s_line_chart_properties,
                              .persistent_properties = s_line_chart_properties,
                              .parent = TK_PARENT_VTABLE(widget),
                              .create = line_chart_create,
                              .on_paint_self = line_chart_on_paint_self,
                              .set_prop = line_chart_set_prop,
                              .get_prop = line_chart_get_prop,
                              .on_event = line_chart_on_event,
                              .on_destroy = line_chart_on_destroy};

widget_t *line_chart_create(widget_t *parent, xy_t x, xy_t y, wh_t w, wh_t h)
{
  widget_t *widget = widget_create(parent, TK_REF_VTABLE(line_chart), x, y, w, h);
  line_chart_t *line_chart = LINE_CHART(widget);
  return_value_if_fail(line_chart != NULL, NULL);

  line_chart->fg_color = tk_strdup("#5555FFFF");
  line_chart->secd_color = tk_strdup("#999999FF");
  line_chart->guide_line_color = tk_strdup("#333333FF");

  line_chart->mode = 0;
  line_chart->max_point = 20;
  line_chart->divide_value = 0.5;
  line_chart->align = 0;
  line_chart->line_width = 5;
  line_chart->queue = TKMEM_ZALLOC(FloatQueue);
  float_queue_init(line_chart->queue, line_chart->max_point);
  line_chart->guide_line_offset = 0.1f;
  line_chart->guide_line_percent = 1.0f;
  line_chart->secd_percent = 1.0f;

  return widget;
}

widget_t *line_chart_cast(widget_t *widget)
{
  return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, line_chart), NULL);

  return widget;
}
