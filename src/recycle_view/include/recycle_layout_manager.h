/**
 * File:   recycle_layout_manager.h
 * Brief:  recycle_view 的布局管理器（内置线性/网格 + 可自定义）
 */
#ifndef TK_RECYCLE_LAYOUT_MANAGER_H
#define TK_RECYCLE_LAYOUT_MANAGER_H

#include "base/widget.h"

BEGIN_C_DECLS

typedef struct _recycle_layout_manager_t recycle_layout_manager_t;

struct _recycle_layout_manager_t {
  bool_t is_horizontal; /* 滚动轴：TRUE=横向滚动, FALSE=纵向滚动 */

  /* 沿滚动轴的内容总尺寸，用于 clamp offset / 滚动条 */
  int32_t (*get_content_size)(recycle_layout_manager_t* lm, widget_t* rv, int32_t item_count);

  /* 给定滚动 offset，算出可见 index 闭区间 [first,last] */
  ret_t (*get_visible_range)(recycle_layout_manager_t* lm, widget_t* rv, int32_t offset,
                             int32_t item_count, int32_t* first, int32_t* last);

  /* 第 index 个 item 在内容坐标系中的矩形（未减 offset） */
  ret_t (*get_item_rect)(recycle_layout_manager_t* lm, widget_t* rv, int32_t index, rect_t* r);

  ret_t (*on_destroy)(recycle_layout_manager_t* lm);

  void* ctx; /* 实现私有数据（如 item_extent / span） */
};

/* 内置：线性布局。item_extent = 沿滚动轴的尺寸（纵向=行高, 横向=列宽） */
recycle_layout_manager_t* recycle_linear_layout_manager_create(bool_t horizontal, int32_t item_extent);

/* 内置：网格布局。span_count=交叉轴等分数（纵向滚动=列数）；交叉轴尺寸 = viewport_cross/span_count */
recycle_layout_manager_t* recycle_grid_layout_manager_create(bool_t horizontal, int32_t span_count,
                                                             int32_t item_extent);

END_C_DECLS

#endif /*TK_RECYCLE_LAYOUT_MANAGER_H*/
