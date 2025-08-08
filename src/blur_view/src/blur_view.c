/**
 * File:   blur_view.c
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
#include "blur_view.h"
#include "view_ext.h"
#include "base/canvas.h"
#include "stackblur.h"
#include "stdio.h"

void apply_gaussian_blur_fast(bitmap_t* bitmap, uint32_t radius) {
  uint32_t* pixels = (uint32_t*)bitmap_lock_buffer_for_write(bitmap);
  int width = bitmap->w;
  int height = bitmap->h;
  
  // 简化版高斯模糊 - 实际实现可能需要更复杂的算法
  for (int y = radius; y < height - radius; y++) {
    for (int x = radius; x < width - radius; x++) {
      uint32_t r = 0, g = 0, b = 0, a = 0;
      int count = 0;
      
      // 采样周围像素
      for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
          uint32_t pixel = pixels[(y + dy) * width + (x + dx)];
          r += (pixel >> 16) & 0xFF;
          g += (pixel >> 8) & 0xFF;
          b += pixel & 0xFF;
          a += (pixel >> 24) & 0xFF;
          count++;
        }
      }
      
      // 计算平均值
      pixels[y * width + x] = ((a/count) << 24) | ((r/count) << 16) | 
                             ((g/count) << 8) | (b/count);
    }
  }
  
  bitmap_unlock_buffer(bitmap);
}

static void apply_gaussian_blur(bitmap_t *bitmap, uint32_t radius)
{
  uint32_t *pixels = (uint32_t *)bitmap_lock_buffer_for_write(bitmap);
  int width = bitmap->w;
  int height = bitmap->h;
  blur_ARGB_8888(pixels, width, height, radius);

  bitmap_unlock_buffer(bitmap);
}
static ret_t blur_widget_update_background(blur_view_t *blur_view)
{
  rect_t r = rect_init(blur_view->widget.x,
                       blur_view->widget.y,
                       blur_view->widget.w,
                       blur_view->widget.h);
  if (blur_view->blur_bitmap != NULL)
  {
    bitmap_destroy(blur_view->blur_bitmap);
  }

  blur_view->blur_bitmap = widget_take_snapshot_rect(blur_view->widget.parent, &r);

  apply_gaussian_blur(blur_view->blur_bitmap, blur_view->radius);

  // printf("blur_view**************blur_widget_update_background**************** %d\n", __LINE__);
  return RET_OK;
}

static ret_t blur_view_get_prop(widget_t *widget, const char *name, value_t *v)
{
  blur_view_t *blur_view = BLUR_VIEW(widget);
  return_value_if_fail(blur_view != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(BLUR_VIEW_PROP_RADIUS, name))
  {
    value_set_float32(v, blur_view->radius);
    return RET_OK;
  }
  return RET_NOT_FOUND;
}

static ret_t blur_view_set_prop(widget_t *widget, const char *name, const value_t *v)
{
  blur_view_t *blur_view = BLUR_VIEW(widget);
  return_value_if_fail(widget != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(BLUR_VIEW_PROP_RADIUS, name))
  {
    blur_view_set_radius(widget, value_float32(v));
    return RET_OK;
  }
  return RET_NOT_FOUND;
}

static ret_t blur_view_on_destroy(widget_t *widget)
{
  blur_view_t *blur_view = BLUR_VIEW(widget);
  return_value_if_fail(widget != NULL && blur_view != NULL, RET_BAD_PARAMS);
  return RET_OK;
}

static ret_t blur_view_on_paint_self(widget_t *widget, canvas_t *c)
{
  blur_view_t *blur_view = BLUR_VIEW(widget);
  canvas_set_stroke_color_str(c, "#FF00FF00");
  canvas_stroke_rect(c, 0, 0, widget->w, widget->h);
  // printf("blur_view********************blur_view_on_paint_self********** %d\n", __LINE__);
  if (tk_str_eq(widget_get_type(widget->parent), WIDGET_TYPE_VIEW_EXT))
  {
    view_ext_set_abort_widget(widget->parent, widget);
    blur_widget_update_background(blur_view);
    if (blur_view->blur_bitmap != NULL)
    {
      // printf("blur_view**************blur_view_on_paint_self**************** %d\n", __LINE__);
      rect_t r = rect_init(0, 0, widget->w, widget->h);
      canvas_draw_image(c, blur_view->blur_bitmap, &r, &r);
    }
    view_ext_set_abort_widget(widget->parent, NULL);
  }
  else
  {
    printf("blur_widget parent is not view_ext!!! \n");
  }
  return RET_OK;
}

static ret_t blur_widget_on_attach_parent(widget_t *widget, widget_t *parent)
{
  // printf("blur_view****************blur_widget_on_attach_parent************** %d\n", __LINE__);
  // 监听父控件的绘制事件
  // if (tk_str_eq(widget_get_type(parent), WIDGET_TYPE_VIEW_EXT))
  // {
  //   view_ext_set_abort_widget(parent,widget);
  // }else{
  //   printf("blur_widget parent is not view_ext!!! \n");
  // }

  return RET_OK;
}

const char *s_blur_view_properties[] = {
    BLUR_VIEW_PROP_RADIUS,
    BLUR_VIEW_PROP_DEBUG,
    NULL};

TK_DECL_VTABLE(blur_view) = {.size = sizeof(blur_view_t),
                             .type = WIDGET_TYPE_BLUR_VIEW,
                             .clone_properties = s_blur_view_properties,
                             .persistent_properties = s_blur_view_properties,
                             .parent = TK_PARENT_VTABLE(widget),
                             .create = blur_view_create,
                             .on_paint_self = blur_view_on_paint_self,
                             .on_attach_parent = blur_widget_on_attach_parent,
                             .set_prop = blur_view_set_prop,
                             .get_prop = blur_view_get_prop,
                             .on_destroy = blur_view_on_destroy};

widget_t *blur_view_create(widget_t *parent, xy_t x, xy_t y, wh_t w, wh_t h)
{
  widget_t *widget = widget_create(parent, TK_REF_VTABLE(blur_view), x, y, w, h);
  blur_view_t *blur_view = BLUR_VIEW(widget);
  return_value_if_fail(blur_view != NULL, NULL);
  blur_view->radius = 2.0;
  blur_view->abort = FALSE;
  printf("blur_view****************************** %d\n", __LINE__);
  return widget;
}

widget_t *blur_view_cast(widget_t *widget)
{
  return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, blur_view), NULL);

  return widget;
}

ret_t blur_view_set_radius(widget_t *widget, float_t radius)
{
  blur_view_t *blur_view = BLUR_VIEW(widget);
  return_value_if_fail(blur_view != NULL, RET_BAD_PARAMS);
  blur_view->radius = radius;
  printf("blur_view****************************** %d\n", __LINE__);
  return RET_OK;
}
