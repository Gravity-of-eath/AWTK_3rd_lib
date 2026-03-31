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
#include "base/vgcanvas.h"
#include "base/native_window.h"
#include "base/window_manager.h"
#include "base/lcd.h"
#include "stackblur.h"
#include "stdio.h"
#include "tkc/time_now.h"
#define WITH_GPU

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
#ifdef WITH_GPU
  native_window_t* native_window =
      (native_window_t*)widget_get_prop_pointer(window_manager(), WIDGET_PROP_NATIVE_WINDOW);
  if (native_window != NULL) {
    canvas_t* c = native_window_get_canvas(native_window);
    vgcanvas_t* vg = lcd_get_vgcanvas(c->lcd);
    if (vg != NULL) {
      int w0 = bitmap->w;
      int h0 = bitmap->h;
      int levels = 1;
      if (radius <= 2) {
        levels = 1;
      } else if (radius <= 6) {
        levels = 2;
      } else {
        levels = 3;
      }
      if (levels < 1) {
        levels = 1;
      }
      if (levels > 4) {
        levels = 4;
      }
      framebuffer_object_t fbos_down[4];
      framebuffer_object_t fbos_up[4];
      bitmap_t imgs_down[4];
      bitmap_t imgs_up[4];
      int ws[4];
      int hs[4];
      int i;
      for (i = 0; i < 4; i++) {
        memset(&fbos_down[i], 0, sizeof(framebuffer_object_t));
        memset(&fbos_up[i], 0, sizeof(framebuffer_object_t));
        memset(&imgs_down[i], 0, sizeof(bitmap_t));
        memset(&imgs_up[i], 0, sizeof(bitmap_t));
        ws[i] = 0;
        hs[i] = 0;
      }
      bitmap_t* src = bitmap;
      int sw = w0;
      int sh = h0;
      for (i = 0; i < levels; i++) {
        int dw = tk_max(1, sw >> 1);
        int dh = tk_max(1, sh >> 1);
        if (vgcanvas_create_fbo(vg, dw, dh, FALSE, &fbos_down[i]) != RET_OK) {
          break;
        }
        vgcanvas_bind_fbo(vg, &fbos_down[i]);
        vgcanvas_reset_curr_state(vg);
        vgcanvas_draw_image(vg, src, 0, 0, (float_t)sw, (float_t)sh, 0, 0, (float_t)dw, (float_t)dh);
        vgcanvas_unbind_fbo(vg, &fbos_down[i]);
        ws[i] = dw;
        hs[i] = dh;
        imgs_down[i].w = dw;
        imgs_down[i].h = dh;
        fbo_to_img(&fbos_down[i], &imgs_down[i]);
        src = &imgs_down[i];
        sw = dw;
        sh = dh;
      }
      bitmap_t* usrc = src;
      int uw = sw;
      int uh = sh;
      int up_count = i;
      for (i = up_count - 1; i >= 0; i--) {
        int tw = (i == 0) ? w0 : ws[i - 1];
        int th = (i == 0) ? h0 : hs[i - 1];
        if (vgcanvas_create_fbo(vg, tw, th, FALSE, &fbos_up[i]) != RET_OK) {
          break;
        }
        vgcanvas_bind_fbo(vg, &fbos_up[i]);
        vgcanvas_reset_curr_state(vg);
        vgcanvas_draw_image(vg, usrc, 0, 0, (float_t)uw, (float_t)uh, 0, 0, (float_t)tw, (float_t)th);
        vgcanvas_unbind_fbo(vg, &fbos_up[i]);
        imgs_up[i].w = tw;
        imgs_up[i].h = th;
        fbo_to_img(&fbos_up[i], &imgs_up[i]);
        usrc = &imgs_up[i];
        uw = tw;
        uh = th;
      }
      if (uw == w0 && uh == h0) {
        vgcanvas_fbo_to_bitmap(vg, &fbos_up[0], bitmap, NULL);
      }
      for (i = 0; i < 4; i++) {
        if (fbos_down[i].handle != NULL) {
          vgcanvas_destroy_fbo(vg, &fbos_down[i]);
        }
        if (fbos_up[i].handle != NULL) {
          vgcanvas_destroy_fbo(vg, &fbos_up[i]);
        }
      }
      return;
    }
  }
#endif
  uint32_t *pixels = (uint32_t *)bitmap_lock_buffer_for_write(bitmap);
  int width = bitmap->w;
  int height = bitmap->h;
  blur_ARGB_8888(pixels, width, height, radius);

  bitmap_unlock_buffer(bitmap);
}
static ret_t blur_widget_update_background(blur_view_t *blur_view)
{
  point_t p = {0, 0};
  widget_to_screen(&blur_view->widget, &p);
  rect_t r = rect_init(p.x, p.y, blur_view->widget.w, blur_view->widget.h);
  blur_view->is_capturing = TRUE;
  blur_view->widget.visible = FALSE;
#ifdef WITH_GPU
  {
    native_window_t* nw = (native_window_t*)widget_get_prop_pointer(window_manager(), WIDGET_PROP_NATIVE_WINDOW);
    if (nw != NULL) {
      canvas_t* c = native_window_get_canvas(nw);
      vgcanvas_t* vg = lcd_get_vgcanvas(c->lcd);
      if (vg != NULL) {
        int w0 = blur_view->widget.w;
        int h0 = blur_view->widget.h;
        bool_t need_recreate = FALSE;
        if (!blur_view->fbo_inited) need_recreate = TRUE;
        if (blur_view->fbo_w != w0 || blur_view->fbo_h != h0) need_recreate = TRUE;
        if (need_recreate) {
          if (blur_view->blur_bitmap != NULL) {
            bitmap_destroy(blur_view->blur_bitmap);
            blur_view->blur_bitmap = NULL;
          }
          if (blur_view->fbo_a.handle != NULL) vgcanvas_destroy_fbo(vg, &blur_view->fbo_a);
          memset(&blur_view->fbo_a, 0, sizeof(blur_view->fbo_a));
          vgcanvas_create_fbo(vg, w0, h0, FALSE, &blur_view->fbo_a);
          blur_view->fbo_w = w0;
          blur_view->fbo_h = h0;
          blur_view->fbo_inited = TRUE;
        }
        if (blur_view->blur_bitmap == NULL ||
            (int)blur_view->blur_bitmap->w != w0 || (int)blur_view->blur_bitmap->h != h0) {
          if (blur_view->blur_bitmap != NULL) {
            bitmap_destroy(blur_view->blur_bitmap);
          }
          blur_view->blur_bitmap = bitmap_create_ex(w0, h0, w0 * 4, BITMAP_FMT_RGBA8888);
        }

        /* Step 1: Save canvas state */
        xy_t save_ox = c->ox;
        xy_t save_oy = c->oy;
        rect_t save_clip;
        canvas_get_clip_rect(c, &save_clip);

        vgcanvas_flush(vg);

        /* Step 2: Capture the scene into fbo_a (full resolution) */
        vgcanvas_bind_fbo(vg, &blur_view->fbo_a);
        c->ox = 0;
        c->oy = 0;
        {
          rect_t r_fb = rect_init(0, 0, w0, h0);
          canvas_set_clip_rect(c, &r_fb);
        }
        canvas_translate(c, -r.x, -r.y);
        widget_paint(widget_get_window_manager(&blur_view->widget), c);
        canvas_translate(c, r.x, r.y);
        vgcanvas_unbind_fbo(vg, &blur_view->fbo_a);

        /* Step 3: Restore canvas state */
        c->ox = save_ox;
        c->oy = save_oy;
        canvas_set_clip_rect(c, &save_clip);

        /* Step 4: Read FBO pixels back to CPU bitmap */
        vgcanvas_fbo_to_bitmap(vg, &blur_view->fbo_a, blur_view->blur_bitmap, NULL);

        /* Step 5: CPU downsample + blur */
        {
          uint32_t blur_radius = (uint32_t)blur_view->radius;
          if (blur_radius > 0) {
            uint32_t ds = blur_view->downscale;
            if (ds < 1) ds = 1;
            uint32_t* pixels = (uint32_t*)bitmap_lock_buffer_for_write(blur_view->blur_bitmap);
            if (pixels != NULL) {
              if (ds > 1) {
                int dw = w0 / (int)ds;
                int dh = h0 / (int)ds;
                int sx, sy, dx, dy;
                for (dy = 0; dy < dh; dy++) {
                  sy = dy * (int)ds;
                  for (dx = 0; dx < dw; dx++) {
                    sx = dx * (int)ds;
                    pixels[dy * dw + dx] = pixels[sy * w0 + sx];
                  }
                }
                blur_ARGB_8888(pixels, dw, dh, blur_radius);
                for (dy = h0 - 1; dy >= 0; dy--) {
                  sy = dy / (int)ds;
                  if (sy >= dh) sy = dh - 1;
                  for (dx = w0 - 1; dx >= 0; dx--) {
                    sx = dx / (int)ds;
                    if (sx >= dw) sx = dw - 1;
                    pixels[dy * w0 + dx] = pixels[sy * dw + sx];
                  }
                }
              } else {
                blur_ARGB_8888(pixels, w0, h0, blur_radius);
              }
              bitmap_unlock_buffer(blur_view->blur_bitmap);
            }
          }
        }

        /* Mark dirty so AWTK updates existing GPU texture in-place (no leak) */
        bitmap_set_dirty(blur_view->blur_bitmap, TRUE);
      }
    }
  }
#else
  if (blur_view->blur_bitmap != NULL)
  {
    bitmap_destroy(blur_view->blur_bitmap);
  }
  blur_view->blur_bitmap = widget_take_snapshot_rect((blur_view->widget.parent), &r);
  apply_gaussian_blur(blur_view->blur_bitmap, blur_view->radius);
#endif
  blur_view->is_capturing = FALSE;
  blur_view->widget.visible = TRUE;
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
  else if (tk_str_eq(BLUR_VIEW_PROP_UPDATE_INTERVAL, name))
  {
    value_set_uint32(v, blur_view->update_interval_ms);
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
  else if (tk_str_eq(BLUR_VIEW_PROP_UPDATE_INTERVAL, name))
  {
    blur_view->update_interval_ms = value_uint32(v);
    return RET_OK;
  }
  return RET_NOT_FOUND;
}

static ret_t blur_view_on_destroy(widget_t *widget)
{
  blur_view_t *blur_view = BLUR_VIEW(widget);
  return_value_if_fail(widget != NULL && blur_view != NULL, RET_BAD_PARAMS);

  if (blur_view->blur_bitmap != NULL)
  {
    bitmap_destroy(blur_view->blur_bitmap);
    blur_view->blur_bitmap = NULL;
  }

#ifdef WITH_GPU
  {
    native_window_t* nw = (native_window_t*)widget_get_prop_pointer(window_manager(), WIDGET_PROP_NATIVE_WINDOW);
    if (nw != NULL) {
      canvas_t* canvas = native_window_get_canvas(nw);
      if (canvas != NULL) {
        vgcanvas_t* vg = lcd_get_vgcanvas(canvas->lcd);
        if (vg != NULL) {
          if (blur_view->fbo_a.handle != NULL) {
            vgcanvas_destroy_fbo(vg, &blur_view->fbo_a);
            memset(&blur_view->fbo_a, 0, sizeof(blur_view->fbo_a));
          }
        }
      }
    }
  }
#endif

  return RET_OK;
}

static ret_t blur_view_on_paint_self(widget_t *widget, canvas_t *c)
{
  blur_view_t *blur_view = BLUR_VIEW(widget);

  if (blur_view->is_capturing)
  {
    return RET_OK;
  }

  uint64_t now = time_now_ms();
  uint64_t elapsed = now - blur_view->last_update_ms;

  if (elapsed >= blur_view->update_interval_ms)
  {
    blur_widget_update_background(blur_view);
    blur_view->last_update_ms = now;
  }

  if (blur_view->blur_bitmap != NULL)
  {
    rect_t src = rect_init(0, 0, blur_view->blur_bitmap->w, blur_view->blur_bitmap->h);
    rect_t dst = rect_init(0, 0, widget->w, widget->h);
    canvas_draw_image(c, blur_view->blur_bitmap, &src, &dst);
  }

  widget_invalidate_force(widget, NULL);

  return RET_OK;
}

const char *s_blur_view_properties[] = {
    BLUR_VIEW_PROP_RADIUS,
    BLUR_VIEW_PROP_DEBUG,
    BLUR_VIEW_PROP_UPDATE_INTERVAL,
    NULL};

TK_DECL_VTABLE(blur_view) = {.size = sizeof(blur_view_t),
                             .type = WIDGET_TYPE_BLUR_VIEW,
                             .clone_properties = s_blur_view_properties,
                             .persistent_properties = s_blur_view_properties,
                             .parent = TK_PARENT_VTABLE(widget),
                             .create = blur_view_create,
                             .on_paint_self = blur_view_on_paint_self,
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
  blur_view->is_capturing = FALSE;
  blur_view->last_update_ms = 0;
  blur_view->update_interval_ms = 33;
  blur_view->downscale = 2;
#ifdef WITH_GPU
  memset(&blur_view->fbo_a, 0, sizeof(blur_view->fbo_a));
  blur_view->fbo_inited = FALSE;
  blur_view->fbo_w = 0;
  blur_view->fbo_h = 0;
#endif
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
  return RET_OK;
}
