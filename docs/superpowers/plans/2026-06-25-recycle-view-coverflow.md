# recycle_view 布局下放 + coverflow 实现计划

> 执行用 superpowers:subagent-driven-development，逐任务 + 审查。

**目标**：把"摆位"从 recycle_view 核心下放给布局管理器（RecyclerView 式控制反转），新增一个 coverflow（焦点居中、两侧对称缩放、中心置顶压住两边）布局管理器，缩放用一个**内部自包含的缩放辅助**（拷贝 auto_scale_view 思路、去 printf，不依赖也不修改 auto_scale_view）。

**架构**：核心 relayout 的回收/填充（步骤 1–4）不变；步骤 5（摆位）若 LM 提供了可选 `layout_children` 回调则全权交给 LM（位置/尺寸/缩放/z 序），否则走原 `get_item_rect - offset` 默认路径——向后兼容，linear/grid 不受影响。

**关键已验证事实**：
- `widget_restack(w, index)`：index 越大越靠上，≥子控件总数则置顶。
- 字号：`style_get_int(widget_get_style(w), STYLE_ID_FONT_SIZE, 默认)` 读；`widget_set_prop_int(w, "style:normal:font_size", v)` 写。
- 缩放走"按基线 × ratio 重排子树"的布局重算（无 GPU 变换），从固定基线绝对计算 ⇒ 幂等、可反复对复用控件施加不同 ratio。
- 默认显示 5 个（radius=2）。

**改动文件**：
- `src/recycle_view/include/recycle_layout_manager.h`（+`recycle_item_t`、+可选 `layout_children` 回调）
- `src/recycle_view/src/recycle_view.c`（relayout 步骤 5 分支）
- `src/recycle_view/src/recycle_scale.{h,c}`（内部缩放辅助，新增）
- `src/recycle_view/src/recycle_coverflow_layout_manager.c` + 头文件构造函数声明（新增）
- `src/recycle_view/tests/recycle_view_coverflow_test.c`（GUI demo，新增）+ CMake

---

## Task A：核心钩子——把摆位下放给 LM（向后兼容）

**文件**：`recycle_layout_manager.h`、`recycle_view.c`

- [ ] 1. 在 `recycle_layout_manager.h` 的 `BEGIN_C_DECLS` 后、`struct _recycle_layout_manager_t` 前，加可见项结构：
```c
/* 传给 layout_children 的可见项（核心已完成回收/填充后） */
typedef struct _recycle_item_t {
  int32_t   index;
  widget_t* widget;
} recycle_item_t;
```
- [ ] 2. 在 `struct _recycle_layout_manager_t` 末尾（`on_destroy` 之后、`ctx` 之前）加可选回调：
```c
  /* 可选：若非 NULL，核心在回收/填充后把当前 offset + 可见项数组交给它全权摆位
   * （位置/尺寸/缩放/z 序）。为 NULL 时核心走 get_item_rect-offset 默认摆位。 */
  ret_t (*layout_children)(recycle_layout_manager_t* lm, widget_t* rv, int32_t offset,
                           recycle_item_t* items, uint32_t nr);
```
- [ ] 3. 在 `recycle_view.c` 的 `recycle_view_relayout` 里，把"已挂载项随 offset 重新定位"那个循环替换为分支：
```c
  /* 步骤5：摆位。LM 提供 layout_children 则全权下放，否则默认线性平移 */
  if (lm->layout_children != NULL) {
    uint32_t n = rv->visible_items->size, k = 0;
    recycle_item_t* arr = (n > 0) ? TKMEM_ZALLOCN(recycle_item_t, n) : NULL;
    if (arr != NULL) {
      for (k = 0; k < n; k++) {
        visible_item_t* vi = (visible_item_t*)darray_get(rv->visible_items, k);
        if (vi != NULL) { arr[k].index = vi->index; arr[k].widget = vi->widget; }
      }
      lm->layout_children(lm, widget, offset, arr, n);
      TKMEM_FREE(arr);
    }
  } else {
    uint32_t k = 0;
    for (k = 0; k < rv->visible_items->size; k++) {
      rect_t r;
      visible_item_t* vi = (visible_item_t*)darray_get(rv->visible_items, k);
      if (vi == NULL) continue;
      lm->get_item_rect(lm, widget, vi->index, &r);
      if (lm->is_horizontal) {
        widget_move_resize(vi->widget, r.x - offset, r.y, r.w, r.h);
      } else {
        widget_move_resize(vi->widget, r.x, r.y - offset, r.w, r.h);
      }
    }
  }
```
- [ ] 4. 构建：`cmake --build build --target recycle_view` 应通过；linear/grid 行为不变（layout_children 为 NULL）。提交。

---

## Task B：内部缩放辅助 + coverflow 布局管理器

**文件**：`recycle_scale.{h,c}`、`recycle_coverflow_layout_manager.c`、`recycle_layout_manager.h`（加构造声明）

### B1 内部缩放辅助 `recycle_scale.h/.c`（拷 auto_scale_view 思路、无 printf）

按 DFS 顺序捕获一次"基线几何"（每节点 rect + 字号），之后可对**任意同构子树**按 `ratio` 绝对重排（幂等）。因 coverflow 所有卡片结构相同，一份基线适配所有 item。

```c
/* recycle_scale.h */
#ifndef TK_RECYCLE_SCALE_H
#define TK_RECYCLE_SCALE_H
#include "base/widget.h"
BEGIN_C_DECLS
typedef struct _recycle_scale_t recycle_scale_t;
/* 从一棵"满尺寸"子树捕获基线（按 DFS 顺序记录 rect+字号） */
recycle_scale_t* recycle_scale_capture(widget_t* root);
/* 把同构子树 root 的每个节点设为 基线值×ratio（含字号），绝对、幂等 */
ret_t recycle_scale_apply(recycle_scale_t* s, widget_t* root, float_t ratio);
ret_t recycle_scale_destroy(recycle_scale_t* s);
END_C_DECLS
#endif
```
```c
/* recycle_scale.c 要点 */
#include "recycle_scale.h"
#include "tkc/mem.h"
#include "tkc/darray.h"
#include "base/style.h"

typedef struct _scale_node_t { rect_t rect; int32_t font; } scale_node_t;
struct _recycle_scale_t { scale_node_t* nodes; uint32_t nr; };

/* DFS 收集：先自身后子，顺序须与 apply 完全一致 */
static void scale_collect(widget_t* w, darray_t* out) {
  scale_node_t* n = TKMEM_ZALLOC(scale_node_t);
  uint32_t i = 0;
  n->rect = rect_init(w->x, w->y, w->w, w->h);
  n->font = 0;
  if (w->astyle != NULL) { n->font = style_get_int(w->astyle, STYLE_ID_FONT_SIZE, 0); }
  darray_push(out, n);
  for (i = 0; i < widget_count_children(w); i++) {
    scale_collect(widget_get_child(w, i), out);
  }
}
recycle_scale_t* recycle_scale_capture(widget_t* root) {
  recycle_scale_t* s = NULL; darray_t* tmp = NULL; uint32_t i = 0;
  return_value_if_fail(root != NULL, NULL);
  tmp = darray_create(16, NULL, NULL);
  scale_collect(root, tmp);
  s = TKMEM_ZALLOC(recycle_scale_t);
  s->nr = tmp->size;
  s->nodes = TKMEM_ZALLOCN(scale_node_t, s->nr);
  for (i = 0; i < s->nr; i++) { s->nodes[i] = *(scale_node_t*)darray_get(tmp, i); }
  darray_destroy(tmp); /* 元素 TKMEM_ZALLOC 的需在此释放：用带 destroy 的 darray 或手动 free */
  return s;
}
/* apply：DFS 同序遍历，第 k 个节点设为 nodes[k]×ratio */
static void scale_apply_dfs(recycle_scale_t* s, widget_t* w, float_t ratio, uint32_t* k) {
  uint32_t i = 0;
  if (*k < s->nr) {
    scale_node_t* n = &s->nodes[*k];
    widget_move_resize(w, (xy_t)(n->rect.x * ratio), (xy_t)(n->rect.y * ratio),
                       (wh_t)(n->rect.w * ratio), (wh_t)(n->rect.h * ratio));
    if (n->font > 0) {
      widget_set_prop_int(w, "style:normal:font_size", (int32_t)(n->font * ratio));
    }
  }
  (*k)++;
  for (i = 0; i < widget_count_children(w); i++) {
    scale_apply_dfs(s, widget_get_child(w, i), ratio, k);
  }
}
ret_t recycle_scale_apply(recycle_scale_t* s, widget_t* root, float_t ratio) {
  uint32_t k = 0;
  return_value_if_fail(s != NULL && root != NULL, RET_BAD_PARAMS);
  scale_apply_dfs(s, root, ratio, &k);
  return RET_OK;
}
ret_t recycle_scale_destroy(recycle_scale_t* s) {
  if (s != NULL) { if (s->nodes) TKMEM_FREE(s->nodes); TKMEM_FREE(s); }
  return RET_OK;
}
```
> 注意：`scale_collect` 里 `darray_create(16, NULL, NULL)` 的元素是 `TKMEM_ZALLOC` 出来的，capture 末尾 `darray_destroy` 前要先把每个元素 free（或建 darray 时传 `default_destroy`）。实现时用 `darray_create(16, default_destroy, NULL)` 让其自动释放，且 capture 里已把内容拷进 `s->nodes`。注意根节点（item 自身）也被记入 nodes[0]，但 apply 时 nodes[0] 会把 root 自身也 move_resize 成 base×ratio——这正好让外框尺寸=base×scale，coverflow LM 因此**不需要再单独 move_resize 外框**，只需定位（见下）。改为：apply 后再 `widget_move_resize` 设定 root 的最终屏幕 x/y（保持 apply 算出的 w/h）。

### B2 coverflow LM `recycle_coverflow_layout_manager.c`

ctx 保存参数 + 懒捕获的 `recycle_scale_t* baseline`：
```c
typedef struct _coverflow_ctx_t {
  int32_t step;        /* 焦点间 offset 步长（=一项滚动距离） */
  int32_t spacing;     /* 相邻项中心屏幕间距(<base ⇒ 重叠) */
  int32_t base_w, base_h;
  int32_t radius;      /* 每侧可见数；5个=2 */
  float_t shrink;      /* 每远一格缩小量 */
  float_t min_scale;
  recycle_scale_t* baseline; /* 懒捕获：首帧从满尺寸 item 捕获一次 */
} coverflow_ctx_t;
```
回调：
- `get_content_size = (item_count<=1)?viewport_main : (item_count-1)*step + viewport_main`（viewport_main = is_horizontal? rv->w : rv->h），使 offset 上限=(count-1)*step。
- `get_visible_range(offset)`：`focus = (step>0)? (offset + step/2)/step : 0`；`*first = focus-radius-1; *last = focus+radius+1`，clamp 到 [0,count-1]。
- `get_item_rect(index)`：主轴 = `index*step`，供 scroll_to_index：
  `if horizontal { r->x=index*step; r->y=0; r->w=base_w; r->h=base_h; } else { r->y=index*step; ... }`。
- `layout_children(rv, offset, items, nr)`：
```
focus_f = step>0 ? (float)offset/step : 0
center_main = is_horizontal ? rv->w/2 : rv->h/2
center_cross= is_horizontal ? rv->h/2 : rv->w/2
若 baseline==NULL 且 nr>0：从"最接近满尺寸"的项捕获——取 |d| 最小的项，其 children 此刻若已被缩放则不可靠；
  ⇒ 改为：首帧捕获前先对该项 apply ratio=1 不行(无基线)。安全做法：在 capture 前该项必须是满尺寸。
  做法：对每个尚未缩放过的新项，LM 不持久缩放状态；改为“捕获一次基线”发生在第一次 layout_children、且只从 items[0] 捕获——但 items[0] 可能非满尺寸(被复用)。
  ⇒ 采用稳妥策略：捕获基线时，先把该 item 用 get_item_rect 的满尺寸 move_resize 还原 children 不可行。
  最终方案：baseline 在“首个 item 进入可见、尚未缩放”时捕获 —— 由 adapter 创建时即满尺寸且未缩放，LM 在 baseline==NULL 时直接 capture(items[0].widget) 即可（首帧所有项都是刚创建/绑定的满尺寸）。
for each item:
  d = index - focus_f; ad = fabs(d)
  if ad > radius + 1: widget_set_visible(false); continue
  widget_set_visible(true)
  scale = max(min_scale, 1 - shrink*ad)
  recycle_scale_apply(baseline, item.widget, scale)   /* 缩放子树(含外框 w/h) */
  iw = base_w*scale; ih = base_h*scale
  main_pos  = center_main + (int)(d*spacing)
  if horizontal: x = main_pos - iw/2; y = center_cross - ih/2
  else:          y = main_pos - ih/2; x = center_cross - iw/2
  widget_move_resize(item.widget, x, y, iw, ih)        /* 设定最终屏幕位置(覆盖 apply 的 x/y) */
  记录 (item.widget, ad) 备 z 序
/* z 序：按 ad 降序 restack，最近者最后→置顶压住两边 */
对可见集合按 ad 降序排序，依次 widget_restack(w, k++)（k 从 0 起，越大越上）
```
> baseline 懒捕获定稿：`if (ctx->baseline == NULL && nr > 0) ctx->baseline = recycle_scale_capture(items[0].widget);` 首帧 items 都是刚创建绑定的满尺寸，安全。其后复用项被 apply 绝对重排，不漂移。

构造声明加到 `recycle_layout_manager.h`：
```c
recycle_layout_manager_t* recycle_coverflow_layout_manager_create(
    bool_t horizontal, int32_t step, int32_t spacing, int32_t base_w, int32_t base_h,
    int32_t visible_radius, float_t shrink, float_t min_scale);
```
`on_destroy` 释放 baseline + ctx + lm。

- [ ] 构建 `recycle_view` 通过，提交。

---

## Task C：coverflow GUI demo + 跑起来

**文件**：`tests/recycle_view_coverflow_test.c`、`CMakeLists.txt`

- adapter：`get_item_count=20`；`create_item_view` 建一张满尺寸卡片 = 一个 `view`(base_w×base_h, 设 bg_color) + 内部一个居中 label(设 "style:normal:font_size")；`bind` 设卡片底色按 index 变化 + label 文本 "第 N 项"。**所有卡片结构相同**（DFS 基线通用）。
- `application_init`：注册、建窗口、建 recycle_view(横向, 视口约 760×360)，`set_layout_manager(recycle_coverflow_layout_manager_create(TRUE, step=180, spacing=140, base_w=200, base_h=280, radius=2, shrink=0.18, min_scale=0.6))`，`set_adapter`。
- 自动推进焦点：定时器每 ~600ms `recycle_view_scroll_to_index(rv, ++focus, TRUE)`，到末项停；用 printf 报告每步 focus 与 create_item_view 次数（应 ≤ ~7）。
- CMake：x86-only 加 `recycle_view_coverflow_test`，链 `${LIBRARY_NAME} ${AWTK_SO} m dl pthread`，include `${AWTK_INCLUDE}/ext_widgets`。
- [ ] 构建并在 DISPLAY=:0 运行；截图；确认：中间最大且压住两侧、两侧对称递减、焦点平滑切换、create 次数远小于数据量。提交。

---

## 完成标准
- linear/grid 回归不变（layout_children=NULL 默认路径）。
- coverflow：焦点居中放大置顶、两侧对称缩小被压、scroll_to_index 平滑切焦点。
- 复用：20 项（或更多）只创建 ~2*radius+3 个控件。
