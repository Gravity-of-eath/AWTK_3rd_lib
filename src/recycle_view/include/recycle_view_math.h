/**
 * File:   recycle_view_math.h
 * Brief:  recycle_view 等尺寸布局的纯算术（不依赖 AWTK，便于单元测试）
 */
#ifndef TK_RECYCLE_VIEW_MATH_H
#define TK_RECYCLE_VIEW_MATH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 线性布局：沿滚动轴的内容总尺寸 = item_count * item_extent（参数非法时返回 0） */
int32_t recycle_linear_content_size(int32_t item_count, int32_t item_extent);

/* 线性布局：第 index 个 item 在内容坐标系中沿滚动轴的起点 = index * item_extent */
int32_t recycle_linear_item_main_pos(int32_t index, int32_t item_extent);

/* 线性布局：给定滚动 offset 计算可见 index 闭区间 [first,last]。
 * 空列表 / 非法参数时置 first=0,last=-1（表示空区间）。返回 0 表示成功，非 0 表示参数非法。 */
int32_t recycle_linear_visible_range(int32_t offset, int32_t viewport_main, int32_t item_extent,
                                     int32_t item_count, int32_t* first, int32_t* last);

/* 网格布局：行/列数 = ceil(item_count/span)，内容总尺寸 = 行数 * item_extent */
int32_t recycle_grid_content_size(int32_t item_count, int32_t span, int32_t item_extent);

/* 网格布局：第 index 个 item 的行列（row=index/span, col=index%span） */
int32_t recycle_grid_cell(int32_t index, int32_t span, int32_t* row, int32_t* col);

/* 网格布局：可见 index 闭区间 [first,last]，同线性规则但按整行进出。 */
int32_t recycle_grid_visible_range(int32_t offset, int32_t viewport_main, int32_t item_extent,
                                   int32_t span, int32_t item_count, int32_t* first, int32_t* last);

/* 把 offset 夹到 [0, max(0, content_size - viewport_main)] */
int32_t recycle_clamp_offset(int32_t offset, int32_t content_size, int32_t viewport_main);

/* fling 每帧速度衰减：返回 v * friction（friction 应在 (0,1)） */
float recycle_fling_next_v(float v, float friction);

#ifdef __cplusplus
}
#endif

#endif /*TK_RECYCLE_VIEW_MATH_H*/
