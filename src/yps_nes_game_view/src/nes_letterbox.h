#ifndef YPS_NES_LETTERBOX_H
#define YPS_NES_LETTERBOX_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define NES_NATIVE_W 256
#define NES_NATIVE_H 240

typedef struct { int32_t x, y, w, h; } nes_rect_t;

/* Compute dst rect that fits 256x240 proportionally inside (widget_w, widget_h),
 * centered. Returns zero-sized rect if either dimension is 0. */
nes_rect_t nes_letterbox_fit(int32_t widget_w, int32_t widget_h);

#ifdef __cplusplus
}
#endif
#endif
