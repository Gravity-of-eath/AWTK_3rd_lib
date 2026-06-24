# recycle_view Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build an AWTK custom widget `recycle_view` — an Android-RecyclerView-style container that recycles item widgets via an adapter (data↔view binding) and a layout manager (placement rules).

**Architecture:** Three layers communicating through C-callback structs: `recycle_adapter_t` (data → item view), `recycle_layout_manager_t` (placement / visible range, built-in linear + grid), and the `recycle_view_t` widget core (scheduler: offset model, recycle pool, drag/fling, scroll_to). Equal-size items make visible-range computation pure integer arithmetic, which is extracted into an AWTK-free math layer (`recycle_view_math.c`) so it can be unit-tested by a standalone executable, exactly like `breath_ellipse_math.c`.

**Tech Stack:** C, AWTK GUI framework (`widget_t` vtable contract, `darray_t`, `velocity_t`, `widget_add_timer`, `widget_grab`), CMake.

**Spec:** `docs/superpowers/specs/2026-06-24-recycle-view-design.md`

**Reference files (read before starting):**
- `src/breath_ellipse/src/breath_ellipse_view.c` — widget vtable / create / on_event / on_destroy pattern
- `src/breath_ellipse/include/breath_ellipse_view.h` — header macro pattern (`TK_EXTERN_VTABLE`, `WIDGET_TYPE_*`, cast macro)
- `src/breath_ellipse/src/breath_ellipse_view_register.c` — registration pattern
- `src/breath_ellipse/CMakeLists.txt` — library + standalone test exe + install pattern
- `src/breath_ellipse/tests/breath_ellipse_view_test.c` — plain assert/main test style
- `src/CMakeLists.txt` — top-level `add_subdirectory` aggregation
- `include/x86/awtk/src/ext_widgets/scroll_view/scroll_view.c` (if present) — reference for `widget_grab`/drag-threshold/fling handling

**Conventions to follow:**
- Comments and user-facing log strings in Chinese (matches existing widgets).
- `return_value_if_fail(...)` guards at top of every public function.
- The math layer (`recycle_view_math.h/.c`) MUST NOT include any AWTK header — it uses only `<stdint.h>`/plain ints so the test exe links without `libawtk.so`.

---

## File Structure

```
src/recycle_view/
  CMakeLists.txt                       # library + test exe + install
  include/
    recycle_view_math.h                # AWTK-free pure arithmetic (declarations)
    recycle_adapter.h                  # recycle_adapter_t
    recycle_layout_manager.h           # recycle_layout_manager_t + built-in constructors
    recycle_view.h                     # recycle_view_t + public API + vtable/macros
    recycle_view_register.h            # register declaration
  src/
    recycle_view_math.c                # pure arithmetic (compiled into lib AND test exe)
    recycle_linear_layout_manager.c    # linear (vertical/horizontal) layout manager
    recycle_grid_layout_manager.c      # grid layout manager
    recycle_view.c                     # core: struct, vtable, relayout, drag, fling, scroll_to
    recycle_view_register.c            # registration
  tests/
    recycle_view_math_test.c           # unit tests for the pure math layer
```

---

## Task 1: Pure math layer + test scaffolding

The math layer holds every integer computation the layout managers and core need, with no AWTK dependency, so it is unit-testable by a standalone executable.

**Files:**
- Create: `src/recycle_view/include/recycle_view_math.h`
- Create: `src/recycle_view/src/recycle_view_math.c`
- Create: `src/recycle_view/tests/recycle_view_math_test.c`
- Create: `src/recycle_view/CMakeLists.txt`
- Modify: `src/CMakeLists.txt` (add `add_subdirectory(recycle_view)`)

- [ ] **Step 1: Write the header**

Create `src/recycle_view/include/recycle_view_math.h`:

```c
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
```

- [ ] **Step 2: Write the failing test**

Create `src/recycle_view/tests/recycle_view_math_test.c`:

```c
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
```

- [ ] **Step 3: Write the CMakeLists (library stub + test exe)**

Create `src/recycle_view/CMakeLists.txt`. The library `file(GLOB)` will pick up only `recycle_view_math.c` until later tasks add more `src/*.c` files — that is fine.

```cmake
cmake_minimum_required (VERSION 3.10)
include_directories(${AWTK_INCLUDE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(LIBRARY_NAME recycle_view)

file(GLOB SOURCES
    ./*.c
    src/*.c
)

add_library(${LIBRARY_NAME} ${SOURCES})
target_link_libraries(${LIBRARY_NAME} ${AWTK_SO})

# 纯算术单元测试（不链接 AWTK）
add_executable(recycle_view_math_test
    tests/recycle_view_math_test.c
    src/recycle_view_math.c)
target_include_directories(recycle_view_math_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(recycle_view_math_test m)

install(
    TARGETS ${LIBRARY_NAME}
    LIBRARY DESTINATION ${LIBRARY_NAME}
)

install(
    DIRECTORY include
    DESTINATION ${LIBRARY_NAME}
)
```

- [ ] **Step 4: Register the subdirectory in the top-level CMake**

In `src/CMakeLists.txt`, add after the line `add_subdirectory(yps_nes_game_view)`:

```cmake
add_subdirectory(recycle_view)
```

- [ ] **Step 5: Run the test to verify it fails (link error: undefined symbols)**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library
mkdir -p build && cd build && cmake -DPLATFORM=x86 ../src >/dev/null && make recycle_view_math_test 2>&1 | tail -20
```
Expected: FAIL — linker errors `undefined reference to recycle_linear_content_size` etc. (header exists, no implementation yet).

- [ ] **Step 6: Write the implementation**

Create `src/recycle_view/src/recycle_view_math.c`:

```c
#include "recycle_view_math.h"

int32_t recycle_linear_content_size(int32_t item_count, int32_t item_extent) {
  if (item_count <= 0 || item_extent <= 0) {
    return 0;
  }
  return item_count * item_extent;
}

int32_t recycle_linear_item_main_pos(int32_t index, int32_t item_extent) {
  if (index < 0 || item_extent <= 0) {
    return 0;
  }
  return index * item_extent;
}

int32_t recycle_linear_visible_range(int32_t offset, int32_t viewport_main, int32_t item_extent,
                                     int32_t item_count, int32_t* first, int32_t* last) {
  if (first == NULL || last == NULL) {
    return -1;
  }
  *first = 0;
  *last = -1;
  if (item_count <= 0 || item_extent <= 0 || viewport_main <= 0) {
    return 0;
  }
  if (offset < 0) {
    offset = 0;
  }
  *first = offset / item_extent;
  *last = (offset + viewport_main - 1) / item_extent;
  if (*first < 0) {
    *first = 0;
  }
  if (*last > item_count - 1) {
    *last = item_count - 1;
  }
  if (*first > *last) {
    /* offset 越过末尾：退回最后一项 */
    *first = item_count - 1;
    *last = item_count - 1;
  }
  return 0;
}

int32_t recycle_grid_content_size(int32_t item_count, int32_t span, int32_t item_extent) {
  int32_t rows = 0;
  if (item_count <= 0 || span <= 0 || item_extent <= 0) {
    return 0;
  }
  rows = (item_count + span - 1) / span;
  return rows * item_extent;
}

int32_t recycle_grid_cell(int32_t index, int32_t span, int32_t* row, int32_t* col) {
  if (row == NULL || col == NULL || span <= 0 || index < 0) {
    return -1;
  }
  *row = index / span;
  *col = index % span;
  return 0;
}

int32_t recycle_grid_visible_range(int32_t offset, int32_t viewport_main, int32_t item_extent,
                                   int32_t span, int32_t item_count, int32_t* first, int32_t* last) {
  int32_t first_row = 0;
  int32_t last_row = 0;
  if (first == NULL || last == NULL) {
    return -1;
  }
  *first = 0;
  *last = -1;
  if (item_count <= 0 || span <= 0 || item_extent <= 0 || viewport_main <= 0) {
    return 0;
  }
  if (offset < 0) {
    offset = 0;
  }
  first_row = offset / item_extent;
  last_row = (offset + viewport_main - 1) / item_extent;
  *first = first_row * span;
  *last = last_row * span + (span - 1);
  if (*first < 0) {
    *first = 0;
  }
  if (*last > item_count - 1) {
    *last = item_count - 1;
  }
  if (*first > item_count - 1) {
    *first = item_count - 1;
    *last = item_count - 1;
  }
  return 0;
}

int32_t recycle_clamp_offset(int32_t offset, int32_t content_size, int32_t viewport_main) {
  int32_t max_offset = content_size - viewport_main;
  if (max_offset < 0) {
    max_offset = 0;
  }
  if (offset < 0) {
    return 0;
  }
  if (offset > max_offset) {
    return max_offset;
  }
  return offset;
}

float recycle_fling_next_v(float v, float friction) {
  return v * friction;
}
```

- [ ] **Step 7: Run the test to verify it passes**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library/build
make recycle_view_math_test 2>&1 | tail -5 && ./recycle_view/recycle_view_math_test
```
Expected: PASS — prints `recycle_view_math_test: all passed`, exit 0.

- [ ] **Step 8: Commit**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library
git add src/recycle_view/include/recycle_view_math.h src/recycle_view/src/recycle_view_math.c \
        src/recycle_view/tests/recycle_view_math_test.c src/recycle_view/CMakeLists.txt src/CMakeLists.txt
git commit -m "recycle_view: add pure layout math layer with unit tests"
```

---

## Task 2: adapter and layout_manager interface headers

Pure type/declaration headers. No behavior yet — verified by compilation in later tasks. Provide complete definitions so later tasks reference real types.

**Files:**
- Create: `src/recycle_view/include/recycle_adapter.h`
- Create: `src/recycle_view/include/recycle_layout_manager.h`

- [ ] **Step 1: Write the adapter header**

Create `src/recycle_view/include/recycle_adapter.h`:

```c
/**
 * File:   recycle_adapter.h
 * Brief:  recycle_view 的数据适配器（使用者实现回调）
 */
#ifndef TK_RECYCLE_ADAPTER_H
#define TK_RECYCLE_ADAPTER_H

#include "base/widget.h"

BEGIN_C_DECLS

typedef struct _recycle_adapter_t recycle_adapter_t;

struct _recycle_adapter_t {
  /* 必填：数据条数 */
  int32_t (*get_item_count)(recycle_adapter_t* adapter);

  /* 可选：第 index 条的 view_type；为 NULL 时所有项视为类型 0 */
  int32_t (*get_item_type)(recycle_adapter_t* adapter, int32_t index);

  /* 必填：为某 view_type 创建一个"空壳"item 控件（不填数据）。parent 即 recycle_view */
  widget_t* (*create_item_view)(recycle_adapter_t* adapter, widget_t* recycle_view, int32_t view_type);

  /* 必填：把第 index 条数据填进 item 控件（item 可能是从回收池复用而来） */
  ret_t (*bind_item_view)(recycle_adapter_t* adapter, widget_t* item, int32_t index);

  /* 可选：item 被回收进池时回调，做解绑/清理；为 NULL 跳过 */
  ret_t (*on_item_recycled)(recycle_adapter_t* adapter, widget_t* item, int32_t view_type);

  /* 可选：adapter 自身资源销毁 */
  ret_t (*on_destroy)(recycle_adapter_t* adapter);

  void* ctx; /* 使用者自定义数据指针 */
};

END_C_DECLS

#endif /*TK_RECYCLE_ADAPTER_H*/
```

- [ ] **Step 2: Write the layout_manager header**

Create `src/recycle_view/include/recycle_layout_manager.h`:

```c
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

  /* 单个 item 尺寸（等尺寸）。实现可读 rv->w/h 决定填充交叉轴 */
  ret_t (*get_item_size)(recycle_layout_manager_t* lm, widget_t* rv, wh_t* w, wh_t* h);

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
```

- [ ] **Step 3: Verify headers compile**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library
gcc -fsyntax-only -I include/x86/awtk/src -I src/recycle_view/include \
    -x c src/recycle_view/include/recycle_adapter.h && \
gcc -fsyntax-only -I include/x86/awtk/src -I src/recycle_view/include \
    -x c src/recycle_view/include/recycle_layout_manager.h && echo OK
```
Expected: prints `OK` (no syntax errors).

- [ ] **Step 4: Commit**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library
git add src/recycle_view/include/recycle_adapter.h src/recycle_view/include/recycle_layout_manager.h
git commit -m "recycle_view: add adapter and layout_manager interface headers"
```

---

## Task 3: Linear layout manager

Wraps the pure math layer behind the `recycle_layout_manager_t` callbacks. Cross axis fills the viewport.

**Files:**
- Create: `src/recycle_view/src/recycle_linear_layout_manager.c`

- [ ] **Step 1: Write the implementation**

Create `src/recycle_view/src/recycle_linear_layout_manager.c`:

```c
#include "tkc/mem.h"
#include "recycle_layout_manager.h"
#include "recycle_view_math.h"

/* 私有数据：沿滚动轴的 item 尺寸 */
typedef struct _linear_lm_ctx_t {
  int32_t item_extent;
} linear_lm_ctx_t;

static ret_t linear_get_item_size(recycle_layout_manager_t* lm, widget_t* rv, wh_t* w, wh_t* h) {
  linear_lm_ctx_t* ctx = (linear_lm_ctx_t*)(lm->ctx);
  return_value_if_fail(lm != NULL && rv != NULL && w != NULL && h != NULL, RET_BAD_PARAMS);
  if (lm->is_horizontal) {
    *w = ctx->item_extent;
    *h = rv->h; /* 交叉轴填满 */
  } else {
    *w = rv->w; /* 交叉轴填满 */
    *h = ctx->item_extent;
  }
  return RET_OK;
}

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
  lm->get_item_size = linear_get_item_size;
  lm->get_content_size = linear_get_content_size;
  lm->get_visible_range = linear_get_visible_range;
  lm->get_item_rect = linear_get_item_rect;
  lm->on_destroy = linear_on_destroy;
  return lm;
}
```

- [ ] **Step 2: Verify it compiles**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library
gcc -fsyntax-only -I include/x86/awtk/src -I src/recycle_view/include \
    src/recycle_view/src/recycle_linear_layout_manager.c && echo OK
```
Expected: prints `OK`.

- [ ] **Step 3: Commit**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library
git add src/recycle_view/src/recycle_linear_layout_manager.c
git commit -m "recycle_view: add linear layout manager"
```

---

## Task 4: Grid layout manager

Cross-axis size = viewport_cross / span_count; main-axis extent fixed. Uses grid math helpers.

**Files:**
- Create: `src/recycle_view/src/recycle_grid_layout_manager.c`

- [ ] **Step 1: Write the implementation**

Create `src/recycle_view/src/recycle_grid_layout_manager.c`:

```c
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

static ret_t grid_get_item_size(recycle_layout_manager_t* lm, widget_t* rv, wh_t* w, wh_t* h) {
  grid_lm_ctx_t* ctx = (grid_lm_ctx_t*)(lm->ctx);
  int32_t cross = 0;
  return_value_if_fail(lm != NULL && rv != NULL && w != NULL && h != NULL, RET_BAD_PARAMS);
  cross = grid_cross_size(lm, rv);
  if (lm->is_horizontal) {
    *w = ctx->item_extent;
    *h = cross;
  } else {
    *w = cross;
    *h = ctx->item_extent;
  }
  return RET_OK;
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
  lm->get_item_size = grid_get_item_size;
  lm->get_content_size = grid_get_content_size;
  lm->get_visible_range = grid_get_visible_range;
  lm->get_item_rect = grid_get_item_rect;
  lm->on_destroy = grid_on_destroy;
  return lm;
}
```

- [ ] **Step 2: Verify it compiles**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library
gcc -fsyntax-only -I include/x86/awtk/src -I src/recycle_view/include \
    src/recycle_view/src/recycle_grid_layout_manager.c && echo OK
```
Expected: prints `OK`.

- [ ] **Step 3: Commit**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library
git add src/recycle_view/src/recycle_grid_layout_manager.c
git commit -m "recycle_view: add grid layout manager"
```

---

## Task 5: recycle_view core — struct, vtable, create/cast/register, ownership

Sets up the widget skeleton so it registers and builds into the `.so`. No relayout yet (added in Task 6). Includes the internal `visible_item_t` and pool element types used by later tasks.

**Files:**
- Create: `src/recycle_view/include/recycle_view.h`
- Create: `src/recycle_view/include/recycle_view_register.h`
- Create: `src/recycle_view/src/recycle_view.c`
- Create: `src/recycle_view/src/recycle_view_register.c`

- [ ] **Step 1: Write the public header**

Create `src/recycle_view/include/recycle_view.h`:

```c
/**
 * File:   recycle_view.h
 * Brief:  类 Android RecyclerView 的可复用视图容器
 */
#ifndef TK_RECYCLE_VIEW_H
#define TK_RECYCLE_VIEW_H

#include "base/widget.h"
#include "tkc/darray.h"
#include "base/velocity.h"
#include "recycle_adapter.h"
#include "recycle_layout_manager.h"

BEGIN_C_DECLS

/**
 * @class recycle_view_t
 * @parent widget_t
 * @annotation ["scriptable","design","widget"]
 * 类 Android RecyclerView 的可复用视图容器。通过 adapter + layout_manager 驱动，
 * 滚出可视区域的 item 控件回收进池并复用。
 *
 * 在 xml 中使用 "recycle_view" 标签创建控件。如：
 *
 * ```xml
 * <recycle_view x="0" y="0" w="240" h="320"/>
 * ```
 * adapter 与 layout_manager 通过 C API 设置。
 */
typedef struct _recycle_view_t {
  widget_t widget;

  /* 私有变量，不要直接访问 */
  recycle_adapter_t* adapter;
  recycle_layout_manager_t* layout_manager;

  int32_t xoffset;
  int32_t yoffset;
  int32_t item_count;

  darray_t* visible_items; /* 元素类型 visible_item_t* */
  darray_t* recycle_pools; /* 元素类型 recycle_pool_t* */

  velocity_t velocity;
  float_t fling_v;
  uint32_t fling_timer_id;
  bool_t dragging;
  xy_t down_x;
  xy_t down_y;
  int32_t down_offset;

  uint32_t scroll_animator_id;
} recycle_view_t;

widget_t* recycle_view_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h);
recycle_view_t* recycle_view_cast(widget_t* widget);

/* set_* 后 recycle_view 接管所有权，销毁时调用对应 on_destroy 释放 */
ret_t recycle_view_set_adapter(widget_t* widget, recycle_adapter_t* adapter);
ret_t recycle_view_set_layout_manager(widget_t* widget, recycle_layout_manager_t* lm);

ret_t recycle_view_scroll_to_index(widget_t* widget, int32_t index, bool_t animate);
ret_t recycle_view_scroll_to_offset(widget_t* widget, int32_t offset, bool_t animate);
ret_t recycle_view_notify_data_changed(widget_t* widget);

#define WIDGET_TYPE_RECYCLE_VIEW "recycle_view"

#define RECYCLE_VIEW(widget) ((recycle_view_t*)(recycle_view_cast(WIDGET(widget))))

TK_EXTERN_VTABLE(recycle_view);

END_C_DECLS

#endif /*TK_RECYCLE_VIEW_H*/
```

- [ ] **Step 2: Write the register header**

Create `src/recycle_view/include/recycle_view_register.h`:

```c
#ifndef TK_RECYCLE_VIEW_REGISTER_H
#define TK_RECYCLE_VIEW_REGISTER_H

#include "base/widget.h"

BEGIN_C_DECLS

ret_t recycle_view_register(void);
const char* recycle_view_supported_render_mode(void);

END_C_DECLS

#endif /*TK_RECYCLE_VIEW_REGISTER_H*/
```

- [ ] **Step 3: Write the core skeleton**

Create `src/recycle_view/src/recycle_view.c`. This defines the internal types, the create/cast, ownership setters, on_destroy, and an empty `recycle_view_relayout` stub filled in Task 6.

```c
#include "recycle_view.h"

#include "tkc/mem.h"
#include "tkc/utils.h"
#include "base/timer.h"
#include "recycle_view_math.h"

/* 当前挂载的可见项 */
typedef struct _visible_item_t {
  int32_t index;
  int32_t view_type;
  widget_t* widget;
} visible_item_t;

/* 某 view_type 的回收池 */
typedef struct _recycle_pool_t {
  int32_t view_type;
  darray_t* free_widgets; /* 元素 widget_t*（detach 状态，未销毁） */
} recycle_pool_t;

/* 预取冗余：可见区间前后各多保留的项数，减少快速滑动时的临界抖动 */
#define RECYCLE_VIEW_PREFETCH 1
#define RECYCLE_VIEW_FRAME_INTERVAL_MS 16
#define RECYCLE_VIEW_FLING_FRICTION 0.92f
#define RECYCLE_VIEW_FLING_MIN_V 0.5f
#define RECYCLE_VIEW_DRAG_THRESHOLD 3

/* Task 6 实现，先前向声明，供 setter 调用 */
static ret_t recycle_view_relayout(widget_t* widget);

/* ---- 回收池辅助（Task 6 使用，这里先定义以便复用） ---- */

static recycle_pool_t* recycle_view_get_pool(recycle_view_t* rv, int32_t view_type, bool_t create) {
  uint32_t i = 0;
  recycle_pool_t* pool = NULL;
  for (i = 0; i < rv->recycle_pools->size; i++) {
    pool = (recycle_pool_t*)darray_get(rv->recycle_pools, i);
    if (pool != NULL && pool->view_type == view_type) {
      return pool;
    }
  }
  if (!create) {
    return NULL;
  }
  pool = TKMEM_ZALLOC(recycle_pool_t);
  return_value_if_fail(pool != NULL, NULL);
  pool->view_type = view_type;
  pool->free_widgets = darray_create(8, NULL, NULL); /* 不在此销毁 widget */
  darray_push(rv->recycle_pools, pool);
  return pool;
}

static ret_t recycle_pool_destroy(void* data) {
  recycle_pool_t* pool = (recycle_pool_t*)data;
  if (pool != NULL) {
    /* 池中是 detach 状态的 widget，需在此销毁 */
    uint32_t i = 0;
    for (i = 0; i < pool->free_widgets->size; i++) {
      widget_t* w = (widget_t*)darray_get(pool->free_widgets, i);
      if (w != NULL) {
        widget_destroy(w);
      }
    }
    darray_destroy(pool->free_widgets);
    TKMEM_FREE(pool);
  }
  return RET_OK;
}

/* ---- ownership setters ---- */

ret_t recycle_view_set_adapter(widget_t* widget, recycle_adapter_t* adapter) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  return_value_if_fail(rv != NULL && adapter != NULL, RET_BAD_PARAMS);
  if (rv->adapter != NULL && rv->adapter->on_destroy != NULL) {
    rv->adapter->on_destroy(rv->adapter);
  }
  rv->adapter = adapter;
  rv->yoffset = 0;
  rv->xoffset = 0;
  recycle_view_relayout(widget);
  return RET_OK;
}

ret_t recycle_view_set_layout_manager(widget_t* widget, recycle_layout_manager_t* lm) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  return_value_if_fail(rv != NULL && lm != NULL, RET_BAD_PARAMS);
  if (rv->layout_manager != NULL && rv->layout_manager->on_destroy != NULL) {
    rv->layout_manager->on_destroy(rv->layout_manager);
  }
  rv->layout_manager = lm;
  recycle_view_relayout(widget);
  return RET_OK;
}

/* ---- relayout 占位（Task 6 替换实现） ---- */
static ret_t recycle_view_relayout(widget_t* widget) {
  (void)widget;
  return RET_OK;
}

/* ---- scroll_to / notify 占位（Task 9 / 10 替换实现） ---- */
ret_t recycle_view_scroll_to_offset(widget_t* widget, int32_t offset, bool_t animate) {
  (void)widget;
  (void)offset;
  (void)animate;
  return RET_OK;
}

ret_t recycle_view_scroll_to_index(widget_t* widget, int32_t index, bool_t animate) {
  (void)widget;
  (void)index;
  (void)animate;
  return RET_OK;
}

ret_t recycle_view_notify_data_changed(widget_t* widget) {
  return recycle_view_relayout(widget);
}

/* ---- vtable callbacks ---- */

static ret_t recycle_view_on_destroy(widget_t* widget) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  return_value_if_fail(rv != NULL, RET_BAD_PARAMS);

  if (rv->fling_timer_id != 0) {
    timer_remove(rv->fling_timer_id);
    rv->fling_timer_id = 0;
  }
  if (rv->visible_items != NULL) {
    darray_destroy(rv->visible_items); /* 元素 widget 由 awtk 子控件树销毁，这里只放裸结构 */
    rv->visible_items = NULL;
  }
  if (rv->recycle_pools != NULL) {
    darray_destroy(rv->recycle_pools); /* recycle_pool_destroy 会销毁池中 detach 的 widget */
    rv->recycle_pools = NULL;
  }
  if (rv->layout_manager != NULL && rv->layout_manager->on_destroy != NULL) {
    rv->layout_manager->on_destroy(rv->layout_manager);
    rv->layout_manager = NULL;
  }
  if (rv->adapter != NULL && rv->adapter->on_destroy != NULL) {
    rv->adapter->on_destroy(rv->adapter);
    rv->adapter = NULL;
  }
  return RET_OK;
}

static ret_t recycle_view_on_event(widget_t* widget, event_t* e) {
  (void)widget;
  (void)e;
  return RET_OK; /* Task 7/8 填充拖动与 fling */
}

TK_DECL_VTABLE(recycle_view) = {.size = sizeof(recycle_view_t),
                                .type = WIDGET_TYPE_RECYCLE_VIEW,
                                .inputable = TRUE,
                                .parent = TK_PARENT_VTABLE(widget),
                                .create = recycle_view_create,
                                .on_event = recycle_view_on_event,
                                .on_destroy = recycle_view_on_destroy};

widget_t* recycle_view_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h) {
  widget_t* widget = widget_create(parent, TK_REF_VTABLE(recycle_view), x, y, w, h);
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  return_value_if_fail(rv != NULL, NULL);

  rv->adapter = NULL;
  rv->layout_manager = NULL;
  rv->xoffset = 0;
  rv->yoffset = 0;
  rv->item_count = 0;
  rv->visible_items = darray_create(16, NULL, NULL); /* 元素是裸结构指针，自管理 */
  rv->recycle_pools = darray_create(4, recycle_pool_destroy, NULL);
  rv->fling_v = 0.0f;
  rv->fling_timer_id = 0;
  rv->dragging = FALSE;
  rv->scroll_animator_id = 0;
  velocity_reset(&(rv->velocity));

  /* 裁剪子控件到自身范围 */
  widget_set_prop_bool(widget, WIDGET_PROP_CLIP_VIEW, TRUE);
  return widget;
}

widget_t* recycle_view_cast(widget_t* widget) {
  return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, recycle_view), NULL);
  return widget;
}
```

> Note: `visible_items` stores heap `visible_item_t*` structs allocated in Task 6; its `darray_destroy` with a NULL element-destroy will leak those small structs unless freed first. Task 6 Step (cleanup) replaces the `visible_items` destroy with a proper element-destroy callback. For now the skeleton keeps it simple (empty list), so no leak yet.

- [ ] **Step 4: Write the register source**

Create `src/recycle_view/src/recycle_view_register.c`:

```c
#include "base/widget_factory.h"
#include "recycle_view.h"
#include "recycle_view_register.h"

ret_t recycle_view_register(void) {
  return widget_factory_register(widget_factory(), WIDGET_TYPE_RECYCLE_VIEW, recycle_view_create);
}

const char* recycle_view_supported_render_mode(void) {
  return "OpenGL|AGGE-BGR565|AGGE-BGRA8888|AGGE-MONO";
}
```

- [ ] **Step 5: Build the library to verify it compiles and links**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library/build
cmake -DPLATFORM=x86 ../src >/dev/null && make recycle_view 2>&1 | tail -15
```
Expected: `librecycle_view.so` (or `.a`) builds with no errors. Resolve any missing-symbol/typo issues before continuing. (`WIDGET_PROP_CLIP_VIEW` and `widget_set_prop_bool` are standard AWTK; if `WIDGET_PROP_CLIP_VIEW` is absent in this AWTK version, grep `include/x86/awtk/src/base/widget_consts.h` for the correct clip property name and use it.)

- [ ] **Step 6: Commit**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library
git add src/recycle_view/include/recycle_view.h src/recycle_view/include/recycle_view_register.h \
        src/recycle_view/src/recycle_view.c src/recycle_view/src/recycle_view_register.c
git commit -m "recycle_view: add widget core skeleton, vtable, registration, ownership"
```

---

## Task 6: relayout — the recycle + fill engine

Replace the `recycle_view_relayout` stub with the real recycle/fill loop, and fix the `visible_items` element lifecycle.

**Files:**
- Modify: `src/recycle_view/src/recycle_view.c`

- [ ] **Step 1: Add a `visible_item_t` destroy callback and use it for the darray**

In `recycle_view.c`, add this helper after the `recycle_pool_destroy` function:

```c
static ret_t visible_item_destroy(void* data) {
  /* 仅释放裸结构；widget 的归属由回收/子控件树管理，不在此销毁 */
  if (data != NULL) {
    TKMEM_FREE(data);
  }
  return RET_OK;
}
```

In `recycle_view_create`, change the `visible_items` creation line:

```c
  rv->visible_items = darray_create(16, visible_item_destroy, NULL);
```

- [ ] **Step 2: Add visible-list lookup + recycle/obtain helpers**

Add above the (stub) `recycle_view_relayout` in `recycle_view.c`:

```c
static visible_item_t* recycle_view_find_visible(recycle_view_t* rv, int32_t index) {
  uint32_t i = 0;
  for (i = 0; i < rv->visible_items->size; i++) {
    visible_item_t* vi = (visible_item_t*)darray_get(rv->visible_items, i);
    if (vi != NULL && vi->index == index) {
      return vi;
    }
  }
  return NULL;
}

/* 取得某 view_type 的一个空壳：优先复用池，池空则新建 */
static widget_t* recycle_view_obtain_item(recycle_view_t* rv, int32_t view_type) {
  recycle_pool_t* pool = recycle_view_get_pool(rv, view_type, FALSE);
  if (pool != NULL && pool->free_widgets->size > 0) {
    widget_t* w = (widget_t*)darray_get(pool->free_widgets, pool->free_widgets->size - 1);
    darray_remove_index(pool->free_widgets, pool->free_widgets->size - 1);
    return w;
  }
  return rv->adapter->create_item_view(rv->adapter, (widget_t*)rv, view_type);
}
```

(The recycle path — detach + on_item_recycled + push to pool — is inlined directly in `recycle_view_relayout` below, since it needs the relayout-local `offset`/list-index context.)

- [ ] **Step 3: Replace the `recycle_view_relayout` stub with the real implementation**

Replace the entire stub:

```c
static ret_t recycle_view_relayout(widget_t* widget) {
  (void)widget;
  return RET_OK;
}
```

with:

```c
static int32_t recycle_view_main_offset(recycle_view_t* rv) {
  return rv->layout_manager->is_horizontal ? rv->xoffset : rv->yoffset;
}

static void recycle_view_set_main_offset(recycle_view_t* rv, int32_t offset) {
  if (rv->layout_manager->is_horizontal) {
    rv->xoffset = offset;
    rv->yoffset = 0;
  } else {
    rv->yoffset = offset;
    rv->xoffset = 0;
  }
}

static ret_t recycle_view_relayout(widget_t* widget) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  recycle_layout_manager_t* lm = NULL;
  recycle_adapter_t* adapter = NULL;
  int32_t first = 0, last = -1, i = 0, offset = 0, content_size = 0, viewport_main = 0;
  return_value_if_fail(rv != NULL, RET_BAD_PARAMS);

  lm = rv->layout_manager;
  adapter = rv->adapter;
  if (lm == NULL || adapter == NULL) {
    return RET_OK; /* 未就绪：空白，不崩 */
  }

  rv->item_count = adapter->get_item_count != NULL ? adapter->get_item_count(adapter) : 0;

  /* clamp offset 到合法范围 */
  content_size = lm->get_content_size(lm, widget, rv->item_count);
  viewport_main = lm->is_horizontal ? widget->w : widget->h;
  offset = recycle_clamp_offset(recycle_view_main_offset(rv), content_size, viewport_main);
  recycle_view_set_main_offset(rv, offset);

  /* 计算可见区间并加预取冗余 */
  if (rv->item_count > 0) {
    lm->get_visible_range(lm, widget, offset, rv->item_count, &first, &last);
    first -= RECYCLE_VIEW_PREFETCH;
    last += RECYCLE_VIEW_PREFETCH;
    if (first < 0) first = 0;
    if (last > rv->item_count - 1) last = rv->item_count - 1;
  } else {
    first = 0;
    last = -1;
  }

  /* 回收：可见表中落在区间外的项 */
  {
    uint32_t k = 0;
    while (k < rv->visible_items->size) {
      visible_item_t* vi = (visible_item_t*)darray_get(rv->visible_items, k);
      if (vi != NULL && (vi->index < first || vi->index > last)) {
        recycle_pool_t* pool = NULL;
        widget_remove_child(widget, vi->widget);
        if (adapter->on_item_recycled != NULL) {
          adapter->on_item_recycled(adapter, vi->widget, vi->view_type);
        }
        pool = recycle_view_get_pool(rv, vi->view_type, TRUE);
        if (pool != NULL) {
          darray_push(pool->free_widgets, vi->widget);
        } else {
          widget_destroy(vi->widget);
        }
        darray_remove_index(rv->visible_items, k); /* visible_item_destroy 释放裸结构 */
      } else {
        k++;
      }
    }
  }

  /* 填充：区间内尚未挂载的 index */
  for (i = first; i <= last; i++) {
    rect_t r;
    int32_t view_type = 0;
    widget_t* item = NULL;
    visible_item_t* vi = NULL;
    if (recycle_view_find_visible(rv, i) != NULL) {
      continue;
    }
    view_type = (adapter->get_item_type != NULL) ? adapter->get_item_type(adapter, i) : 0;
    item = recycle_view_obtain_item(rv, view_type);
    if (item == NULL) {
      log_debug("recycle_view: create_item_view 返回 NULL，跳过 index=%d\n", i);
      continue;
    }
    adapter->bind_item_view(adapter, item, i);
    widget_add_child(widget, item);
    lm->get_item_rect(lm, widget, i, &r);
    if (lm->is_horizontal) {
      widget_move_resize(item, r.x - offset, r.y, r.w, r.h);
    } else {
      widget_move_resize(item, r.x, r.y - offset, r.w, r.h);
    }
    vi = TKMEM_ZALLOC(visible_item_t);
    if (vi != NULL) {
      vi->index = i;
      vi->view_type = view_type;
      vi->widget = item;
      darray_push(rv->visible_items, vi);
    }
  }

  /* 已挂载项随 offset 重新定位（滑动时需要） */
  {
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

  widget_invalidate_force(widget, NULL);
  return RET_OK;
}
```

> Ensure `#include "tkc/log.h"` is present for `log_debug` (added in Step 4).

- [ ] **Step 4: Add the log include**

At the top of `recycle_view.c` includes, add:

```c
#include "tkc/log.h"
```

- [ ] **Step 5: Build to verify it compiles and links**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library/build
cmake -DPLATFORM=x86 ../src >/dev/null && make recycle_view 2>&1 | tail -15
```
Expected: builds with no errors. Fix any unused-function warnings by removing dead helpers.

- [ ] **Step 6: Commit**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library
git add src/recycle_view/src/recycle_view.c
git commit -m "recycle_view: implement relayout recycle+fill engine"
```

---

## Task 7: Pointer drag scrolling

Implement drag handling in `recycle_view_on_event`: grab on down, accumulate move delta into offset, ungrab on up. Feed `velocity_t` so Task 8 can fling.

**Files:**
- Modify: `src/recycle_view/src/recycle_view.c`

- [ ] **Step 1: Replace `recycle_view_on_event` with drag handling**

Replace:

```c
static ret_t recycle_view_on_event(widget_t* widget, event_t* e) {
  (void)widget;
  (void)e;
  return RET_OK; /* Task 7/8 填充拖动与 fling */
}
```

with:

```c
/* Task 8 实现 fling 启动；此处前向声明 */
static ret_t recycle_view_start_fling(widget_t* widget, float_t v);

static ret_t recycle_view_on_event(widget_t* widget, event_t* e) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  return_value_if_fail(rv != NULL && e != NULL, RET_BAD_PARAMS);
  if (rv->layout_manager == NULL) {
    return RET_OK;
  }

  switch (e->type) {
    case EVT_POINTER_DOWN: {
      pointer_event_t* evt = (pointer_event_t*)e;
      /* 停止正在进行的 fling */
      if (rv->fling_timer_id != 0) {
        timer_remove(rv->fling_timer_id);
        rv->fling_timer_id = 0;
        rv->fling_v = 0.0f;
      }
      rv->dragging = FALSE;
      rv->down_x = evt->x;
      rv->down_y = evt->y;
      rv->down_offset = recycle_view_main_offset(rv);
      velocity_reset(&(rv->velocity));
      velocity_update(&(rv->velocity), e->time, evt->x, evt->y);
      widget_grab(widget->parent, widget);
      break;
    }
    case EVT_POINTER_MOVE: {
      pointer_event_t* evt = (pointer_event_t*)e;
      int32_t delta = rv->layout_manager->is_horizontal ? (evt->x - rv->down_x) : (evt->y - rv->down_y);
      if (!rv->dragging && tk_abs(delta) < RECYCLE_VIEW_DRAG_THRESHOLD) {
        break;
      }
      rv->dragging = TRUE;
      velocity_update(&(rv->velocity), e->time, evt->x, evt->y);
      recycle_view_set_main_offset(rv, rv->down_offset - delta);
      recycle_view_relayout(widget);
      break;
    }
    case EVT_POINTER_UP: {
      pointer_event_t* evt = (pointer_event_t*)e;
      float_t v = 0.0f;
      widget_ungrab(widget->parent, widget);
      if (rv->dragging) {
        velocity_update(&(rv->velocity), e->time, evt->x, evt->y);
        /* velocity 是 px/ms，乘帧间隔得到 px/frame；拖动方向与 offset 相反 */
        v = -(rv->layout_manager->is_horizontal ? rv->velocity.xv : rv->velocity.yv) *
            RECYCLE_VIEW_FRAME_INTERVAL_MS;
        rv->dragging = FALSE;
        recycle_view_start_fling(widget, v);
      }
      break;
    }
    default:
      break;
  }
  return RET_OK;
}
```

- [ ] **Step 2: Build to verify it compiles (fling stub still missing → expect link error referencing recycle_view_start_fling)**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library/build
make recycle_view 2>&1 | tail -15
```
Expected: FAIL — `undefined reference to recycle_view_start_fling` (it is implemented in Task 8). This confirms the event code compiles. Proceed directly to Task 8 (do not commit a non-linking state).

---

## Task 8: Inertial fling

Implement `recycle_view_start_fling` and its timer: each frame decays velocity by friction, advances offset, relayouts, and stops at the edge or below the min velocity.

**Files:**
- Modify: `src/recycle_view/src/recycle_view.c`

- [ ] **Step 1: Add the fling timer + start function**

Add near the other static helpers in `recycle_view.c` (before `recycle_view_on_event`, after `recycle_view_relayout`):

```c
static ret_t recycle_view_on_fling_timer(const timer_info_t* timer) {
  widget_t* widget = WIDGET(timer->ctx);
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  int32_t before = 0, after = 0;
  if (rv == NULL || rv->layout_manager == NULL) {
    return RET_REMOVE;
  }

  before = recycle_view_main_offset(rv);
  recycle_view_set_main_offset(rv, before + (int32_t)rv->fling_v);
  recycle_view_relayout(widget); /* relayout 内部会 clamp offset */
  after = recycle_view_main_offset(rv);

  rv->fling_v = recycle_fling_next_v(rv->fling_v, RECYCLE_VIEW_FLING_FRICTION);

  /* offset 未变化（已触边）或速度过小 → 停止 */
  if (after == before || tk_fabs(rv->fling_v) < RECYCLE_VIEW_FLING_MIN_V) {
    rv->fling_timer_id = 0;
    rv->fling_v = 0.0f;
    return RET_REMOVE;
  }
  return RET_REPEAT;
}

static ret_t recycle_view_start_fling(widget_t* widget, float_t v) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  return_value_if_fail(rv != NULL, RET_BAD_PARAMS);
  rv->fling_v = v;
  if (tk_fabs(v) < RECYCLE_VIEW_FLING_MIN_V) {
    rv->fling_v = 0.0f;
    return RET_OK;
  }
  if (rv->fling_timer_id == 0) {
    rv->fling_timer_id =
        widget_add_timer(widget, recycle_view_on_fling_timer, RECYCLE_VIEW_FRAME_INTERVAL_MS);
  }
  return RET_OK;
}
```

- [ ] **Step 2: Confirm `tk_fabs` availability**

`tk_fabs` and `tk_abs` are in `tkc/utils.h` (already included). If `tk_fabs` is not defined in this AWTK version, use `fabsf` with `#include <math.h>`. Verify:

```bash
grep -rE 'define tk_fabs|tk_fabs\(' /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library/include/x86/awtk/src/tkc/utils.h | head
```
If no output, add `#include <math.h>` and replace `tk_fabs` with `fabsf`.

- [ ] **Step 3: Build to verify it compiles and links**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library/build
make recycle_view 2>&1 | tail -15
```
Expected: builds and links with no errors.

- [ ] **Step 4: Commit**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library
git add src/recycle_view/src/recycle_view.c
git commit -m "recycle_view: add pointer drag scrolling and inertial fling"
```

---

## Task 9: scroll_to_offset / scroll_to_index

Replace the scroll_to stubs with real implementations. Non-animated sets offset and relayouts immediately; animated steps the offset over a short timer toward the target.

**Files:**
- Modify: `src/recycle_view/src/recycle_view.c`

- [ ] **Step 1: Add a scroll animation target + timer, replace the stubs**

Add two fields to track an animated scroll. In `recycle_view.h`, inside `recycle_view_t`, add after `uint32_t scroll_animator_id;`:

```c
  int32_t scroll_target;   /* 动画目标 offset */
  uint32_t scroll_timer_id;
```

Replace the two stub functions in `recycle_view.c`:

```c
ret_t recycle_view_scroll_to_offset(widget_t* widget, int32_t offset, bool_t animate) {
  (void)widget; (void)offset; (void)animate; return RET_OK;
}
ret_t recycle_view_scroll_to_index(widget_t* widget, int32_t index, bool_t animate) {
  (void)widget; (void)index; (void)animate; return RET_OK;
}
```

with:

```c
static ret_t recycle_view_on_scroll_timer(const timer_info_t* timer) {
  widget_t* widget = WIDGET(timer->ctx);
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  int32_t cur = 0, diff = 0, step = 0;
  if (rv == NULL || rv->layout_manager == NULL) {
    return RET_REMOVE;
  }
  cur = recycle_view_main_offset(rv);
  diff = rv->scroll_target - cur;
  /* 每帧前进剩余距离的 1/4，最小步长 1，保证收敛 */
  step = diff / 4;
  if (step == 0) {
    step = (diff > 0) ? 1 : ((diff < 0) ? -1 : 0);
  }
  if (diff == 0) {
    rv->scroll_timer_id = 0;
    return RET_REMOVE;
  }
  recycle_view_set_main_offset(rv, cur + step);
  recycle_view_relayout(widget);
  if (recycle_view_main_offset(rv) == rv->scroll_target ||
      recycle_view_main_offset(rv) == cur /* 已触边无法继续 */) {
    rv->scroll_timer_id = 0;
    return RET_REMOVE;
  }
  return RET_REPEAT;
}

ret_t recycle_view_scroll_to_offset(widget_t* widget, int32_t offset, bool_t animate) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  return_value_if_fail(rv != NULL && rv->layout_manager != NULL, RET_BAD_PARAMS);

  if (rv->fling_timer_id != 0) {
    timer_remove(rv->fling_timer_id);
    rv->fling_timer_id = 0;
    rv->fling_v = 0.0f;
  }

  if (!animate) {
    recycle_view_set_main_offset(rv, offset);
    return recycle_view_relayout(widget);
  }

  rv->scroll_target = offset; /* relayout 会把实际 offset clamp 到合法范围 */
  if (rv->scroll_timer_id == 0) {
    rv->scroll_timer_id =
        widget_add_timer(widget, recycle_view_on_scroll_timer, RECYCLE_VIEW_FRAME_INTERVAL_MS);
  }
  return RET_OK;
}

ret_t recycle_view_scroll_to_index(widget_t* widget, int32_t index, bool_t animate) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  rect_t r;
  int32_t target = 0;
  return_value_if_fail(rv != NULL && rv->layout_manager != NULL, RET_BAD_PARAMS);
  if (index < 0) index = 0;
  rv->layout_manager->get_item_rect(rv->layout_manager, widget, index, &r);
  target = rv->layout_manager->is_horizontal ? r.x : r.y; /* 把该 item 对齐到视口起点 */
  return recycle_view_scroll_to_offset(widget, target, animate);
}
```

- [ ] **Step 2: Initialize the new fields in create and stop the timer in destroy**

In `recycle_view_create`, after `rv->scroll_animator_id = 0;` add:

```c
  rv->scroll_target = 0;
  rv->scroll_timer_id = 0;
```

In `recycle_view_on_destroy`, after the fling-timer removal block add:

```c
  if (rv->scroll_timer_id != 0) {
    timer_remove(rv->scroll_timer_id);
    rv->scroll_timer_id = 0;
  }
```

- [ ] **Step 3: Build to verify it compiles and links**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library/build
cmake -DPLATFORM=x86 ../src >/dev/null && make recycle_view 2>&1 | tail -15
```
Expected: builds and links with no errors.

- [ ] **Step 4: Commit**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library
git add src/recycle_view/include/recycle_view.h src/recycle_view/src/recycle_view.c
git commit -m "recycle_view: add scroll_to_offset/index with animation"
```

---

## Task 10: notify_data_changed (re-bind visible items)

The skeleton's `notify_data_changed` already calls `relayout`, which re-clamps and fills/recycles. But items that stay visible while their underlying data changed must be re-bound. Make `notify_data_changed` re-bind all currently visible items, then relayout.

**Files:**
- Modify: `src/recycle_view/src/recycle_view.c`

- [ ] **Step 1: Replace `recycle_view_notify_data_changed`**

Replace:

```c
ret_t recycle_view_notify_data_changed(widget_t* widget) {
  return recycle_view_relayout(widget);
}
```

with:

```c
ret_t recycle_view_notify_data_changed(widget_t* widget) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  uint32_t k = 0;
  return_value_if_fail(rv != NULL, RET_BAD_PARAMS);
  if (rv->adapter == NULL) {
    return RET_OK;
  }
  /* 先重算总数 */
  rv->item_count = rv->adapter->get_item_count != NULL ? rv->adapter->get_item_count(rv->adapter) : 0;
  /* 对仍可见且仍在数据范围内的项重新绑定（数据变了但壳还在） */
  for (k = 0; k < rv->visible_items->size; k++) {
    visible_item_t* vi = (visible_item_t*)darray_get(rv->visible_items, k);
    if (vi != NULL && vi->index < rv->item_count) {
      rv->adapter->bind_item_view(rv->adapter, vi->widget, vi->index);
    }
  }
  /* 再走标准回收/填充（越界项回收、新进入项填充、offset 夹紧） */
  return recycle_view_relayout(widget);
}
```

- [ ] **Step 2: Build to verify it compiles and links**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library/build
make recycle_view 2>&1 | tail -10
```
Expected: builds and links with no errors.

- [ ] **Step 3: Commit**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library
git add src/recycle_view/src/recycle_view.c
git commit -m "recycle_view: re-bind visible items on notify_data_changed"
```

---

## Task 11: Full build, manual integration demo, and docs

Build the whole project, add a small manual demo that proves recycling (create-count ≪ data-count), and update CLAUDE.md's widget list.

**Files:**
- Create: `src/recycle_view/tests/recycle_view_demo.c` (manual, not built by default)
- Modify: `CLAUDE.md` (add `recycle_view` to the Widgets list)

- [ ] **Step 1: Build all widgets for x86**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library
./build.sh 2>&1 | tail -25
```
Expected: full build succeeds; `3rdlib/x86/recycle_view/` contains the `.so` and `include/`.

- [ ] **Step 2: Run the math unit test once more (regression)**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library/build
./recycle_view/recycle_view_math_test
```
Expected: `recycle_view_math_test: all passed`.

- [ ] **Step 3: Write a manual integration demo (documents intended usage)**

Create `src/recycle_view/tests/recycle_view_demo.c`. This is a reference/usage doc compiled manually against a full AWTK app, NOT wired into CMake (it needs an AWTK app harness). It demonstrates the adapter pattern and is the place to verify recycling by counting `create_item_view` calls.

```c
/**
 * 手动集成示例（非默认构建）：演示 recycle_view 的 adapter + layout_manager 用法，
 * 并通过统计 create_item_view 调用次数验证"确实在复用"。
 * 需在完整 AWTK 应用中调用 demo_recycle_view_create(win) 后滚动观察日志。
 */
#include "recycle_view.h"
#include "tkc/log.h"
#include "widgets/label.h"

typedef struct _demo_adapter_t {
  recycle_adapter_t base;
  int32_t create_count; /* 统计实际创建次数，应远小于数据总量 */
} demo_adapter_t;

static int32_t demo_get_item_count(recycle_adapter_t* a) {
  (void)a;
  return 1000; /* 1000 条数据 */
}

static widget_t* demo_create_item_view(recycle_adapter_t* a, widget_t* rv, int32_t view_type) {
  demo_adapter_t* da = (demo_adapter_t*)a;
  (void)view_type;
  da->create_count++;
  log_info("recycle_view demo: create_item_view 第 %d 次（数据共 1000 条）\n", da->create_count);
  return label_create(NULL, 0, 0, 0, 0); /* 空壳，relayout 会负责 add_child + 定位 */
}

static ret_t demo_bind_item_view(recycle_adapter_t* a, widget_t* item, int32_t index) {
  char text[32];
  (void)a;
  tk_snprintf(text, sizeof(text), "第 %d 项", index);
  return widget_set_text_utf8(item, text);
}

widget_t* demo_recycle_view_create(widget_t* parent) {
  widget_t* rv = recycle_view_create(parent, 0, 0, 240, 320);
  demo_adapter_t* adapter = TKMEM_ZALLOC(demo_adapter_t);
  adapter->base.get_item_count = demo_get_item_count;
  adapter->base.create_item_view = demo_create_item_view;
  adapter->base.bind_item_view = demo_bind_item_view;
  recycle_view_set_layout_manager(rv, recycle_linear_layout_manager_create(FALSE, 48));
  recycle_view_set_adapter(rv, (recycle_adapter_t*)adapter);
  return rv;
}
```

> Manual verification (when integrated into an AWTK app): scroll the full 1000-item list top to bottom. The `create_item_view` log count should plateau at roughly `visible items + 2*prefetch` (e.g. ~9 for a 320px viewport with 48px rows), NOT climb toward 1000 — this is the proof of recycling.

- [ ] **Step 4: Update CLAUDE.md widget list**

In `CLAUDE.md`, under `### Widgets`, add this bullet after the `yps_gl_view` line:

```markdown
- **recycle_view** - RecyclerView 风格可复用视图容器（adapter + layout manager + 回收池；内置线性/网格布局）
```

- [ ] **Step 5: Commit**

```bash
cd /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/claud_prj/Awtk_3rd_library
git add src/recycle_view/tests/recycle_view_demo.c CLAUDE.md
git commit -m "recycle_view: add manual integration demo and update docs"
```

---

## Done

All spec sections implemented:
- adapter / layout_manager / core三层接口 → Tasks 2, 5
- 内置线性纵/横向 + 网格 layout manager → Tasks 3, 4
- 等尺寸纯算术 + 单元测试 → Task 1
- relayout 回收/复用引擎 → Task 6
- 拖动 → Task 7；fling → Task 8；scroll_to → Task 9；notify_data_changed → Task 10
- 全量构建 + 复用验证 demo + 文档 → Task 11

**Post-implementation:** verify on a cross-compile target (`./build.sh t113` etc.) before relying on it on-device; the math layer and core are platform-independent C, so x86 correctness is a strong signal.
```