/**
 * conner_gradient 渲染结果导出为 PNG，供肉眼检查（x86）。
 *
 * 用法: conner_gradient_shot [输出目录]
 * 每种参数组合导出一张 PNG，透明区域用棋盘格衬底以便看清 alpha。
 */
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include "awtk.h"
#include "base/canvas_offline.h"
#include "conner_gradient_view.h"
#include "conner_gradient_view_register.h"

#define VW 240
#define VH 240

static void put_u32(uint8_t* p, uint32_t v) {
  p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}

/* 写一张 24 位 PNG（zlib 直接压，无需额外依赖） */
static ret_t write_png(const char* path, const uint8_t* rgb, uint32_t w, uint32_t h) {
  FILE* f = NULL;
  uLongf clen = compressBound((uLong)(h * (w * 3 + 1)));
  uint8_t* raw = TKMEM_ALLOC(h * (w * 3 + 1));
  uint8_t* comp = TKMEM_ALLOC(clen);
  uint8_t hdr[25], chunk[12];
  uint32_t y;

  return_value_if_fail(raw != NULL && comp != NULL, RET_OOM);

  for (y = 0; y < h; y++) {
    raw[y * (w * 3 + 1)] = 0; /* filter: none */
    memcpy(raw + y * (w * 3 + 1) + 1, rgb + (size_t)y * w * 3, w * 3);
  }
  compress2(comp, &clen, raw, h * (w * 3 + 1), 6);

  f = fopen(path, "wb");
  if (f == NULL) { TKMEM_FREE(raw); TKMEM_FREE(comp); return RET_FAIL; }
  fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);

  /* IHDR */
  put_u32(hdr, 13); memcpy(hdr + 4, "IHDR", 4);
  put_u32(hdr + 8, w); put_u32(hdr + 12, h);
  hdr[16] = 8; hdr[17] = 2; hdr[18] = 0; hdr[19] = 0; hdr[20] = 0;
  put_u32(hdr + 21, (uint32_t)crc32(0, hdr + 4, 17));
  fwrite(hdr, 1, 25, f);

  /* IDAT */
  put_u32(chunk, (uint32_t)clen); memcpy(chunk + 4, "IDAT", 4);
  fwrite(chunk, 1, 8, f);
  fwrite(comp, 1, clen, f);
  {
    uLong c = crc32(0, (const Bytef*)"IDAT", 4);
    c = crc32(c, comp, (uInt)clen);
    put_u32(chunk, (uint32_t)c); fwrite(chunk, 1, 4, f);
  }

  /* IEND */
  put_u32(chunk, 0); memcpy(chunk + 4, "IEND", 4);
  put_u32(chunk + 8, (uint32_t)crc32(0, chunk + 4, 4));
  fwrite(chunk, 1, 12, f);
  fclose(f);

  TKMEM_FREE(raw); TKMEM_FREE(comp);
  return RET_OK;
}

int main(int argc, char** argv) {
  const char* dir = argc > 1 ? argv[1] : ".";
  widget_t *win = NULL, *cg = NULL;
  canvas_t* oc = NULL;
  uint8_t* rgb = NULL;
  uint32_t i;

  struct {
    const char* name;
    float start, stop, ratio;
    int current;
    const char *c0, *c1;
  } shots[] = {
    {"01_整圆_实心",        0, 360, 1.00f, 100, "#FF0000", "#0000FF"},
    {"02_整圆_圆环",        0, 360, 0.25f, 100, "#FF0000", "#0000FF"},
    {"03_270度仪表_满",   135, 405, 0.25f, 100, "#00FF88", "#FF0066"},
    {"04_270度仪表_60%",  135, 405, 0.25f,  60, "#00FF88", "#FF0066"},
    {"05_270度仪表_15%",  135, 405, 0.25f,  15, "#00FF88", "#FF0066"},
    {"06_跨0度_细环",     315,  45, 0.15f, 100, "#FFFF00", "#FF00FF"},
    {"07_180度_粗环",     180, 360, 0.50f, 100, "#FFFFFF", "#0088FF"},
    {"08_半透明渐变",       0, 360, 0.30f, 100, "#FF000020", "#0000FFFF"},
  };

  tk_init(320, 240, APP_SIMULATOR, "conner_gradient_shot", NULL);
  conner_gradient_view_register();
  win = window_create(NULL, 0, 0, 0, 0);
  cg = conner_gradient_view_create(win, 0, 0, VW, VH);
  return_value_if_fail(cg != NULL, 1);

  oc = canvas_offline_create(VW, VH, BITMAP_FMT_RGBA8888);
  return_value_if_fail(oc != NULL, 1);
  rgb = TKMEM_ALLOC(VW * VH * 3);
  return_value_if_fail(rgb != NULL, 1);

  for (i = 0; i < ARRAY_SIZE(shots); i++) {
    char path[512];
    bitmap_t* bmp = NULL;
    uint8_t* src = NULL;
    uint32_t x, y, stride;

    widget_set_prop_str(cg, "start_color", shots[i].c0);
    widget_set_prop_str(cg, "stop_color", shots[i].c1);
    widget_set_prop_float(cg, "start_angle", shots[i].start);
    widget_set_prop_float(cg, "stop_angle", shots[i].stop);
    widget_set_prop_float(cg, "full_ratio", shots[i].ratio);
    widget_set_prop_int(cg, "max", 100);
    widget_set_prop_int(cg, "current", shots[i].current);

    canvas_offline_begin_draw(oc);
    canvas_offline_clear_canvas(oc);
    widget_paint(cg, oc);
    canvas_offline_end_draw(oc);
    canvas_offline_flush_bitmap(oc);

    bmp = canvas_offline_get_bitmap(oc);
    src = bitmap_lock_buffer_for_read(bmp);
    stride = bitmap_get_line_length(bmp);

    /* 合成到棋盘格底上，方便看清透明区域 */
    for (y = 0; y < VH; y++) {
      for (x = 0; x < VW; x++) {
        const uint8_t* s = src + (size_t)y * stride + x * 4;
        uint8_t bg = ((x >> 3) + (y >> 3)) & 1 ? 0x99 : 0x66;
        uint32_t a = s[3];
        uint8_t* d = rgb + ((size_t)y * VW + x) * 3;
        d[0] = (uint8_t)((s[0] * a + bg * (255 - a)) / 255);
        d[1] = (uint8_t)((s[1] * a + bg * (255 - a)) / 255);
        d[2] = (uint8_t)((s[2] * a + bg * (255 - a)) / 255);
      }
    }
    bitmap_unlock_buffer(bmp);

    tk_snprintf(path, sizeof(path), "%s/%s.png", dir, shots[i].name);
    if (write_png(path, rgb, VW, VH) == RET_OK) {
      printf("  %s\n", path);
    } else {
      printf("  写入失败: %s\n", path);
    }
  }

  TKMEM_FREE(rgb);
  canvas_offline_destroy(oc);
  return 0;
}
