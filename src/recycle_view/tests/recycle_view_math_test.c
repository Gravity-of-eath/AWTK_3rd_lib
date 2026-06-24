#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "recycle_view_math.h"

static int nearly_equal(float a, float b, float eps) { return fabsf(a - b) <= eps; }

int main(void) {
  int32_t first = 0, last = 0, row = 0, col = 0;

  /* 线性内容尺寸 */
  assert(recycle_linear_content_size(10, 40) == 400);
  assert(recycle_linear_content_size(0, 40) == 0);
  assert(recycle_linear_content_size(10, 0) == 0);
  assert(recycle_linear_content_size(-3, 40) == 0);

  /* 线性 item 起点 */
  assert(recycle_linear_item_main_pos(0, 40) == 0);
  assert(recycle_linear_item_main_pos(3, 40) == 120);

  /* 线性可见区间：offset=0,viewport=100,extent=40,count=10 → 0..2 */
  assert(recycle_linear_visible_range(0, 100, 40, 10, &first, &last) == 0);
  assert(first == 0 && last == 2);
  /* offset=50 → first=1,last=3 */
  assert(recycle_linear_visible_range(50, 100, 40, 10, &first, &last) == 0);
  assert(first == 1 && last == 3);
  /* 末尾夹紧：offset 很大 → last 不超过 count-1 */
  assert(recycle_linear_visible_range(100000, 100, 40, 10, &first, &last) == 0);
  assert(last == 9);
  /* 空列表 → 空区间 */
  assert(recycle_linear_visible_range(0, 100, 40, 0, &first, &last) == 0);
  assert(first == 0 && last == -1);

  /* 网格内容尺寸：count=10,span=3 → rows=4 → 4*40=160 */
  assert(recycle_grid_content_size(10, 3, 40) == 160);
  assert(recycle_grid_content_size(9, 3, 40) == 120);

  /* 网格行列 */
  assert(recycle_grid_cell(7, 3, &row, &col) == 0);
  assert(row == 2 && col == 1);

  /* 网格可见区间：offset=0,viewport=100,extent=40,span=3,count=10 → 行0..2 → 0..8（夹到9? 行2末=8） */
  assert(recycle_grid_visible_range(0, 100, 40, 3, 10, &first, &last) == 0);
  assert(first == 0 && last == 8);
  /* offset=50 → 行1..3 → first=3,last=11→夹到9 */
  assert(recycle_grid_visible_range(50, 100, 40, 3, 10, &first, &last) == 0);
  assert(first == 3 && last == 9);

  /* offset 夹紧 */
  assert(recycle_clamp_offset(-5, 400, 100) == 0);
  assert(recycle_clamp_offset(500, 400, 100) == 300);
  assert(recycle_clamp_offset(50, 400, 100) == 50);
  assert(recycle_clamp_offset(50, 80, 100) == 0); /* 内容比视口小 */

  /* fling 衰减 */
  assert(nearly_equal(recycle_fling_next_v(10.0f, 0.9f), 9.0f, 1e-4f));

  printf("recycle_view_math_test: all passed\n");
  return 0;
}
