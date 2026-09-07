/**
 * conner_gradient 帧间缓存验证（x86，软件帧缓冲）
 *
 * 1) 正确性：同一组参数下，"缓存未命中(实际渲染)" 与 "缓存命中(贴图)" 两次绘制
 *    产生的像素必须完全一致——这是缓存唯一的正确性风险点。
 * 2) 性能：对比参数每帧都变(全部 miss) 与 参数不变(全部 hit) 的耗时。
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "awtk.h"
#include "base/canvas_offline.h"
#include "conner_gradient_view.h"
#include "conner_gradient_view_register.h"

#define VW 200
#define VH 200

static double now_ms(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

/* 把控件画到一张离屏 canvas 上并取回像素 */
static ret_t paint_to(widget_t* w, canvas_t* oc, uint8_t* out, uint32_t size) {
  bitmap_t* bmp = NULL;
  uint8_t* data = NULL;

  canvas_offline_begin_draw(oc);
  canvas_offline_clear_canvas(oc);
  widget_paint(w, oc);
  canvas_offline_end_draw(oc);
  canvas_offline_flush_bitmap(oc);

  bmp = canvas_offline_get_bitmap(oc);
  return_value_if_fail(bmp != NULL, RET_FAIL);

  data = bitmap_lock_buffer_for_read(bmp);
  return_value_if_fail(data != NULL, RET_FAIL);
  memcpy(out, data, size);
  bitmap_unlock_buffer(bmp);

  return RET_OK;
}

int main(void) {
  widget_t* win = NULL;
  widget_t* cg = NULL;
  canvas_t* oc = NULL;
  uint32_t size = VW * VH * 4;
  uint8_t *buf_miss = NULL, *buf_hit = NULL;
  int i, bad = 0;
  double t0, t_miss, t_hit;

  tk_init(320, 240, APP_SIMULATOR, "conner_gradient_cache_test", NULL);
  conner_gradient_view_register();

  win = window_create(NULL, 0, 0, 0, 0);
  cg = conner_gradient_view_create(win, 0, 0, VW, VH);
  return_value_if_fail(cg != NULL, 1);

  widget_set_prop_str(cg, "start_color", "#FF0000");
  widget_set_prop_str(cg, "stop_color", "#0000FF");
  widget_set_prop_float(cg, "start_angle", 135);
  widget_set_prop_float(cg, "stop_angle", 405);
  widget_set_prop_float(cg, "full_ratio", 0.25f);
  widget_set_prop_int(cg, "max", 100);

  oc = canvas_offline_create(VW, VH, BITMAP_FMT_RGBA8888);
  return_value_if_fail(oc != NULL, 1);

  buf_miss = TKMEM_ALLOC(size);
  buf_hit = TKMEM_ALLOC(size);
  return_value_if_fail(buf_miss != NULL && buf_hit != NULL, 1);

  /* --- 正确性：每个 current 值，先 miss 后 hit，逐像素比对 --- */
  for (i = 0; i <= 100; i += 5) {
    widget_set_prop_int(cg, "current", i); /* 置脏 -> 下次绘制是 miss */
    if (paint_to(cg, oc, buf_miss, size) != RET_OK) { printf("paint failed\n"); return 1; }
    /* 不改任何参数 -> 这次是 hit */
    if (paint_to(cg, oc, buf_hit, size) != RET_OK) { printf("paint failed\n"); return 1; }

    if (memcmp(buf_miss, buf_hit, size) != 0) {
      uint32_t k, n = 0;
      for (k = 0; k < size; k++) if (buf_miss[k] != buf_hit[k]) n++;
      printf("  current=%3d  不一致字节 %u / %u\n", i, n, size);
      bad++;
    }
  }
  printf("[正确性] miss vs hit 比对 21 组: %s\n", bad ? "失败" : "全部逐像素一致");

  /* --- 关键：直写像素路径(cache=TRUE) 必须与 canvas 路径(cache=FALSE) 完全一致 --- */
  {
    int bad2 = 0;
    for (i = 0; i <= 100; i += 5) {
      widget_set_prop_bool(cg, "cache", FALSE);
      widget_set_prop_int(cg, "current", i);
      paint_to(cg, oc, buf_miss, size);        /* canvas_fill_rect 路径 */
      widget_set_prop_bool(cg, "cache", TRUE);
      paint_to(cg, oc, buf_hit, size);         /* 直写像素 + 贴图 路径 */
      if (memcmp(buf_miss, buf_hit, size) != 0) {
        uint32_t k, n = 0;
        for (k = 0; k < size; k++) if (buf_miss[k] != buf_hit[k]) n++;
        printf("  current=%3d  不一致字节 %u / %u\n", i, n, size);
        bad2++;
      }
    }
    printf("[正确性] canvas路径 vs 直写路径 21 组: %s\n", bad2 ? "失败" : "全部逐像素一致");
    bad += bad2;
  }
  widget_set_prop_bool(cg, "cache", TRUE);

  /* --- 性能 --- */
  t0 = now_ms();
  for (i = 0; i < 200; i++) {
    widget_set_prop_int(cg, "current", i % 101); /* 每帧都变 -> 全 miss */
    paint_to(cg, oc, buf_miss, size);
  }
  t_miss = (now_ms() - t0) / 200;

  widget_set_prop_int(cg, "current", 50);
  paint_to(cg, oc, buf_miss, size); /* 预热 */
  t0 = now_ms();
  for (i = 0; i < 200; i++) {
    paint_to(cg, oc, buf_hit, size); /* 参数不变 -> 全 hit */
  }
  t_hit = (now_ms() - t0) / 200;

  printf("[性能]   参数每帧变化(全 miss): %.4f ms/帧\n", t_miss);
  printf("[性能]   参数不变  (全 hit)  : %.4f ms/帧   缓存命中加速 %.1fx\n",
         t_hit, t_miss / (t_hit > 0 ? t_hit : 1e-9));
  printf("         注: 两者都含离屏 canvas 的 clear/flush/memcpy 固定开销\n");

  /* --- 只测 widget_paint 本身：一次 begin/end 里连续绘制，不做 clear 和拷贝 --- */
  canvas_offline_begin_draw(oc);
  widget_set_prop_int(cg, "current", 50);
  widget_paint(cg, oc); /* 预热，把缓存填上 */
  t0 = now_ms();
  for (i = 0; i < 500; i++) widget_paint(cg, oc);
  t_hit = (now_ms() - t0) / 500;
  t0 = now_ms();
  for (i = 0; i < 500; i++) {
    ((conner_gradient_view_t*)cg)->cache_dirty = TRUE; /* 强制每帧重渲染 */
    widget_paint(cg, oc);
  }
  t_miss = (now_ms() - t0) / 500;
  canvas_offline_end_draw(oc);

  printf("[性能]   净 widget_paint  重渲染: %.4f ms   贴图: %.4f ms   %.1fx\n",
         t_miss, t_hit, t_miss / (t_hit > 0 ? t_hit : 1e-9));

  /* --- 生命周期：反复创建/绘制/销毁，验证缓存位图被正确释放 --- */
  {
    int k;
    for (k = 0; k < 50; k++) {
      widget_t* tmp = conner_gradient_view_create(win, 0, 0, 120, 120);
      widget_set_prop_int(tmp, "max", 100);
      widget_set_prop_int(tmp, "current", k);
      canvas_offline_begin_draw(oc);
      widget_paint(tmp, oc);   /* 触发缓存位图分配 */
      canvas_offline_end_draw(oc);
      widget_destroy(tmp);
    }
    printf("[生命周期] 创建/绘制/销毁 50 次完成\n");
  }

  TKMEM_FREE(buf_miss);
  TKMEM_FREE(buf_hit);
  canvas_offline_destroy(oc);
  return bad ? 1 : 0;
}
