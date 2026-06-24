#include "tkc/mem.h"
#include "recycle_layout_manager.h"
#include "recycle_view_math.h"

/* 私有数据：交叉轴等分数 + 沿滚动轴的 item 尺寸 */
typedef struct _grid_lm_ctx_t {
  int32_t span_count;
  int32_t item_extent;
} grid_lm_ctx_t;

/* 交叉轴单格尺寸 = 视口交叉轴尺寸 / span_count（至少 1） */
static int32_t grid_cross_size(recycle_layout_manager_t* lm, widget_t* rv) {
  grid_lm_ctx_t* ctx = (grid_lm_ctx_t*)(lm->ctx);
  int32_t cross = lm->is_horizontal ? rv->h : rv->w;
  int32_t cell = (ctx->span_count > 0) ? (cross / ctx->span_count) : cross;
  return cell > 0 ? cell : 1;
}

static int32_t grid_get_content_size(recycle_layout_manager_t* lm, widget_t* rv, int32_t item_count) {
  grid_lm_ctx_t* ctx = (grid_lm_ctx_t*)(lm->ctx);
  return_value_if_fail(lm != NULL, 0);
  return recycle_grid_content_size(item_count, ctx->span_count, ctx->item_extent);
}

static ret_t grid_get_visible_range(recycle_layout_manager_t* lm, widget_t* rv, int32_t offset,
                                    int32_t item_count, int32_t* first, int32_t* last) {
  grid_lm_ctx_t* ctx = (grid_lm_ctx_t*)(lm->ctx);
  int32_t viewport_main = 0;
  return_value_if_fail(lm != NULL && rv != NULL && first != NULL && last != NULL, RET_BAD_PARAMS);
  viewport_main = lm->is_horizontal ? rv->w : rv->h;
  recycle_grid_visible_range(offset, viewport_main, ctx->item_extent, ctx->span_count, item_count,
                             first, last);
  return RET_OK;
}

static ret_t grid_get_item_rect(recycle_layout_manager_t* lm, widget_t* rv, int32_t index, rect_t* r) {
  grid_lm_ctx_t* ctx = (grid_lm_ctx_t*)(lm->ctx);
  int32_t row = 0, col = 0, cross = 0, main_pos = 0, cross_pos = 0;
  return_value_if_fail(lm != NULL && rv != NULL && r != NULL && index >= 0, RET_BAD_PARAMS);
  recycle_grid_cell(index, ctx->span_count, &row, &col);
  cross = grid_cross_size(lm, rv);
  main_pos = row * ctx->item_extent;
  cross_pos = col * cross;
  if (lm->is_horizontal) {
    r->x = main_pos;
    r->y = cross_pos;
    r->w = ctx->item_extent;
    r->h = cross;
  } else {
    r->x = cross_pos;
    r->y = main_pos;
    r->w = cross;
    r->h = ctx->item_extent;
  }
  return RET_OK;
}

static ret_t grid_on_destroy(recycle_layout_manager_t* lm) {
  return_value_if_fail(lm != NULL, RET_BAD_PARAMS);
  if (lm->ctx != NULL) {
    TKMEM_FREE(lm->ctx);
  }
  TKMEM_FREE(lm);
  return RET_OK;
}

recycle_layout_manager_t* recycle_grid_layout_manager_create(bool_t horizontal, int32_t span_count,
                                                             int32_t item_extent) {
  recycle_layout_manager_t* lm = NULL;
  grid_lm_ctx_t* ctx = NULL;
  return_value_if_fail(span_count > 0 && item_extent > 0, NULL);

  lm = TKMEM_ZALLOC(recycle_layout_manager_t);
  return_value_if_fail(lm != NULL, NULL);
  ctx = TKMEM_ZALLOC(grid_lm_ctx_t);
  if (ctx == NULL) {
    TKMEM_FREE(lm);
    return NULL;
  }
  ctx->span_count = span_count;
  ctx->item_extent = item_extent;

  lm->is_horizontal = horizontal;
  lm->ctx = ctx;
  lm->get_content_size = grid_get_content_size;
  lm->get_visible_range = grid_get_visible_range;
  lm->get_item_rect = grid_get_item_rect;
  lm->on_destroy = grid_on_destroy;
  return lm;
}
