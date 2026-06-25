/**
 * File:   recycle_layout_manager.h
 * Brief:  recycle_view 的布局管理器（内置线性/网格 + 可自定义）
 */
#ifndef TK_RECYCLE_LAYOUT_MANAGER_H
#define TK_RECYCLE_LAYOUT_MANAGER_H

#include "base/widget.h"

BEGIN_C_DECLS

/* 传给 layout_children 的可见项（核心已完成回收/填充后） */
typedef struct _recycle_item_t {
  int32_t   index;
  widget_t* widget;
} recycle_item_t;

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

  /* 可选：若非 NULL，核心在回收/填充后把当前 offset + 可见项数组交给它全权摆位
   * （位置/尺寸/缩放/z 序）。为 NULL 时核心走 get_item_rect-offset 默认摆位。 */
  ret_t (*layout_children)(recycle_layout_manager_t* lm, widget_t* rv, int32_t offset,
                           recycle_item_t* items, uint32_t nr);

  void* ctx; /* 实现私有数据（如 item_extent / span） */
};

/* 内置：线性布局。item_extent = 沿滚动轴的尺寸（纵向=行高, 横向=列宽） */
recycle_layout_manager_t* recycle_linear_layout_manager_create(bool_t horizontal, int32_t item_extent);

/* 内置：网格布局。span_count=交叉轴等分数（纵向滚动=列数）；交叉轴尺寸 = viewport_cross/span_count */
recycle_layout_manager_t* recycle_grid_layout_manager_create(bool_t horizontal, int32_t span_count,
                                                             int32_t item_extent);

/* 内置：coverflow 焦点轮播。焦点项居中放大并置顶，两侧对称缩小且被中心压住。
 * step=滚动一项的 offset 步长；spacing=相邻项屏幕中心间距(<base_w ⇒ 重叠)；
 * base_w/base_h=满尺寸卡片；visible_radius=每侧可见数(5个=2)；shrink=每格缩小量；min_scale=最小缩放。
 * 注意：所有 item 须结构相同（缩放用一份 DFS 基线）。配合"卡片即一个容器+内部子控件"使用。 */
recycle_layout_manager_t* recycle_coverflow_layout_manager_create(
    bool_t horizontal, int32_t step, int32_t spacing, int32_t base_w, int32_t base_h,
    int32_t visible_radius, float_t shrink, float_t min_scale);

END_C_DECLS

#endif /*TK_RECYCLE_LAYOUT_MANAGER_H*/
