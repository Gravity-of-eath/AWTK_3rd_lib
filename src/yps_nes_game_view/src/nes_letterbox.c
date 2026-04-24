#include "nes_letterbox.h"

nes_rect_t nes_letterbox_fit(int32_t widget_w, int32_t widget_h) {
  nes_rect_t r = {0,0,0,0};
  if (widget_w <= 0 || widget_h <= 0) return r;

  double sx = (double)widget_w / (double)NES_NATIVE_W;
  double sy = (double)widget_h / (double)NES_NATIVE_H;
  double s  = (sx < sy) ? sx : sy;

  r.w = (int32_t)(NES_NATIVE_W * s + 0.5);
  r.h = (int32_t)(NES_NATIVE_H * s + 0.5);
  if (r.w > widget_w) r.w = widget_w;
  if (r.h > widget_h) r.h = widget_h;
  r.x = (widget_w - r.w) / 2;
  r.y = (widget_h - r.h) / 2;
  return r;
}
