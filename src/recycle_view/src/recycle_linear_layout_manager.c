#include "tkc/mem.h"
#include "recycle_layout_manager.h"
#include "recycle_view_math.h"

/* 私有数据：沿滚动轴的 item 尺寸 */
typedef struct _linear_lm_ctx_t {
  int32_t item_extent;
} linear_lm_ctx_t;

static int32_t linear_get_content_size(recycle_layout_manager_t* lm, widget_t* rv, int32_t item_count) {
  linear_lm_ctx_t* ctx = (linear_lm_ctx_t*)(lm->ctx);
  return_value_if_fail(lm != NULL, 0);
  return recycle_linear_content_size(item_count, ctx->item_extent);
}

static ret_t linear_get_visible_range(recycle_layout_manager_t* lm, widget_t* rv, int32_t offset,
                                      int32_t item_count, int32_t* first, int32_t* last) {
  linear_lm_ctx_t* ctx = (linear_lm_ctx_t*)(lm->ctx);
  int32_t viewport_main = 0;
  return_value_if_fail(lm != NULL && rv != NULL && first != NULL && last != NULL, RET_BAD_PARAMS);
  viewport_main = lm->is_horizontal ? rv->w : rv->h;
  recycle_linear_visible_range(offset, viewport_main, ctx->item_extent, item_count, first, last);
  return RET_OK;
}

static ret_t linear_get_item_rect(recycle_layout_manager_t* lm, widget_t* rv, int32_t index, rect_t* r) {
  linear_lm_ctx_t* ctx = (linear_lm_ctx_t*)(lm->ctx);
  int32_t pos = 0;
  return_value_if_fail(lm != NULL && rv != NULL && r != NULL && index >= 0, RET_BAD_PARAMS);
  pos = recycle_linear_item_main_pos(index, ctx->item_extent);
  if (lm->is_horizontal) {
    r->x = pos;
    r->y = 0;
    r->w = ctx->item_extent;
    r->h = rv->h;
  } else {
    r->x = 0;
    r->y = pos;
    r->w = rv->w;
    r->h = ctx->item_extent;
  }
  return RET_OK;
}

static ret_t linear_on_destroy(recycle_layout_manager_t* lm) {
  return_value_if_fail(lm != NULL, RET_BAD_PARAMS);
  if (lm->ctx != NULL) {
    TKMEM_FREE(lm->ctx);
  }
  TKMEM_FREE(lm);
  return RET_OK;
}

recycle_layout_manager_t* recycle_linear_layout_manager_create(bool_t horizontal, int32_t item_extent) {
  recycle_layout_manager_t* lm = NULL;
  linear_lm_ctx_t* ctx = NULL;
  return_value_if_fail(item_extent > 0, NULL);

  lm = TKMEM_ZALLOC(recycle_layout_manager_t);
  return_value_if_fail(lm != NULL, NULL);
  ctx = TKMEM_ZALLOC(linear_lm_ctx_t);
  if (ctx == NULL) {
    TKMEM_FREE(lm);
    return NULL;
  }
  ctx->item_extent = item_extent;

  lm->is_horizontal = horizontal;
  lm->ctx = ctx;
  lm->get_content_size = linear_get_content_size;
  lm->get_visible_range = linear_get_visible_range;
  lm->get_item_rect = linear_get_item_rect;
  lm->on_destroy = linear_on_destroy;
  return lm;
}
