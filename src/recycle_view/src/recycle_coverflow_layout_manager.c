#include <math.h>
#include "tkc/mem.h"
#include "recycle_layout_manager.h"
#include "recycle_scale.h"

typedef struct _coverflow_ctx_t {
  int32_t step;        /* 焦点间 offset 步长（=滚动一项的距离） */
  int32_t spacing;     /* 相邻项中心的屏幕间距（< base ⇒ 重叠） */
  int32_t base_w, base_h;
  int32_t radius;      /* 每侧可见数（5个=2） */
  float_t shrink;      /* 每远离一格的缩小量 */
  float_t min_scale;
  recycle_scale_t* baseline; /* 懒捕获：首帧从满尺寸 item 捕获一次 */
} coverflow_ctx_t;

static int32_t cf_get_content_size(recycle_layout_manager_t* lm, widget_t* rv, int32_t item_count) {
  coverflow_ctx_t* c = (coverflow_ctx_t*)lm->ctx;
  int32_t viewport_main = lm->is_horizontal ? rv->w : rv->h;
  if (item_count <= 1) return viewport_main;
  return (item_count - 1) * c->step + viewport_main; /* ⇒ offset 上限=(count-1)*step */
}

static ret_t cf_get_visible_range(recycle_layout_manager_t* lm, widget_t* rv, int32_t offset,
                                  int32_t item_count, int32_t* first, int32_t* last) {
  coverflow_ctx_t* c = (coverflow_ctx_t*)lm->ctx;
  int32_t focus = 0;
  (void)rv;
  return_value_if_fail(first != NULL && last != NULL, RET_BAD_PARAMS);
  if (item_count <= 0) { *first = 0; *last = -1; return RET_OK; }
  focus = (c->step > 0) ? (offset + c->step / 2) / c->step : 0;
  *first = focus - c->radius - 1;
  *last  = focus + c->radius + 1;
  if (*first < 0) *first = 0;
  if (*last > item_count - 1) *last = item_count - 1;
  return RET_OK;
}

static ret_t cf_get_item_rect(recycle_layout_manager_t* lm, widget_t* rv, int32_t index, rect_t* r) {
  coverflow_ctx_t* c = (coverflow_ctx_t*)lm->ctx;
  (void)rv;
  return_value_if_fail(r != NULL && index >= 0, RET_BAD_PARAMS);
  /* 仅供 scroll_to_index：把焦点对到该项 ⇒ 目标 offset = index*step（主轴取该值） */
  if (lm->is_horizontal) { r->x = index * c->step; r->y = 0; r->w = c->base_w; r->h = c->base_h; }
  else                   { r->x = 0; r->y = index * c->step; r->w = c->base_w; r->h = c->base_h; }
  return RET_OK;
}

static ret_t cf_layout_children(recycle_layout_manager_t* lm, widget_t* rv, int32_t offset,
                                recycle_item_t* items, uint32_t nr) {
  coverflow_ctx_t* c = (coverflow_ctx_t*)lm->ctx;
  float_t focus_f = (c->step > 0) ? (float_t)offset / (float_t)c->step : 0.0f;
  int32_t center_main  = lm->is_horizontal ? rv->w / 2 : rv->h / 2;
  int32_t center_cross = lm->is_horizontal ? rv->h / 2 : rv->w / 2;
  uint32_t i = 0, m = 0;
  /* 可见集合（用于 z 序排序）：保存 widget 与 |d| */
  typedef struct { widget_t* w; float_t ad; } vz_t;
  vz_t* vis = (nr > 0) ? TKMEM_ZALLOCN(vz_t, nr) : NULL;

  /* 懒捕获基线：首帧 items 都是刚创建绑定的满尺寸，安全 */
  if (c->baseline == NULL && nr > 0) {
    c->baseline = recycle_scale_capture(items[0].widget);
  }

  for (i = 0; i < nr; i++) {
    widget_t* w = items[i].widget;
    float_t d = (float_t)items[i].index - focus_f;
    float_t ad = (d < 0) ? -d : d;
    float_t scale;
    int32_t iw, ih, main_pos, x, y;
    if (ad > (float_t)c->radius + 1.0f) { widget_set_visible(w, FALSE); continue; }
    widget_set_visible(w, TRUE);
    scale = 1.0f - c->shrink * ad;
    if (scale < c->min_scale) scale = c->min_scale;
    if (c->baseline != NULL) recycle_scale_apply(c->baseline, w, scale); /* 缩放子树(含外框) */
    iw = (int32_t)(c->base_w * scale);
    ih = (int32_t)(c->base_h * scale);
    main_pos = center_main + (int32_t)(d * (float_t)c->spacing);
    if (lm->is_horizontal) { x = main_pos - iw / 2; y = center_cross - ih / 2; }
    else                   { y = main_pos - ih / 2; x = center_cross - iw / 2; }
    widget_move_resize(w, x, y, iw, ih); /* 设定最终屏幕位置（覆盖 apply 的 x/y） */
    if (vis != NULL) { vis[m].w = w; vis[m].ad = ad; m++; }
  }

  /* z 序：按 |d| 降序排序，依次 restack（index 越大越靠上 ⇒ 中心置顶压住两边） */
  if (vis != NULL) {
    uint32_t a = 0, b = 0, k = 0;
    for (a = 1; a < m; a++) { /* 插入排序：ad 降序 */
      vz_t key = vis[a];
      b = a;
      while (b > 0 && vis[b - 1].ad < key.ad) { vis[b] = vis[b - 1]; b--; }
      vis[b] = key;
    }
    for (k = 0; k < m; k++) { widget_restack(vis[k].w, k); }
    TKMEM_FREE(vis);
  }
  return RET_OK;
}

static ret_t cf_on_destroy(recycle_layout_manager_t* lm) {
  coverflow_ctx_t* c = NULL;
  return_value_if_fail(lm != NULL, RET_BAD_PARAMS);
  c = (coverflow_ctx_t*)lm->ctx;
  if (c != NULL) {
    if (c->baseline != NULL) recycle_scale_destroy(c->baseline);
    TKMEM_FREE(c);
  }
  TKMEM_FREE(lm);
  return RET_OK;
}

recycle_layout_manager_t* recycle_coverflow_layout_manager_create(
    bool_t horizontal, int32_t step, int32_t spacing, int32_t base_w, int32_t base_h,
    int32_t visible_radius, float_t shrink, float_t min_scale) {
  recycle_layout_manager_t* lm = NULL;
  coverflow_ctx_t* c = NULL;
  return_value_if_fail(step > 0 && base_w > 0 && base_h > 0 && visible_radius > 0, NULL);
  lm = TKMEM_ZALLOC(recycle_layout_manager_t);
  return_value_if_fail(lm != NULL, NULL);
  c = TKMEM_ZALLOC(coverflow_ctx_t);
  if (c == NULL) { TKMEM_FREE(lm); return NULL; }
  c->step = step; c->spacing = spacing; c->base_w = base_w; c->base_h = base_h;
  c->radius = visible_radius; c->shrink = shrink; c->min_scale = min_scale;
  c->baseline = NULL;
  lm->is_horizontal = horizontal;
  lm->ctx = c;
  lm->get_content_size = cf_get_content_size;
  lm->get_visible_range = cf_get_visible_range;
  lm->get_item_rect = cf_get_item_rect;
  lm->layout_children = cf_layout_children;
  lm->on_destroy = cf_on_destroy;
  return lm;
}
