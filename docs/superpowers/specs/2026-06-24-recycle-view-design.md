# recycle_view 设计文档

日期：2026-06-24
状态：已确认，待实现

## 1. 目标

基于 [AWTK](https://github.com/zlgopen/awtk) 实现一个类似 Android `RecyclerView` 的可复用视图容器控件。通过 **adapter**（数据↔视图绑定）与 **layout manager**（摆放规则）两层抽象，配合**回收池**，在数据量很大时只创建可见区域 + 少量缓冲的 item 控件，滚出屏幕的控件回收进池并重新绑定新数据。

AWTK 自带的 `list_view`/`scroll_view`/`children_layouter_list_view` 会把所有子控件一次性创建并布局后再裁剪滚动，没有视图复用。本控件填补这一空白。

## 2. 范围（第一期）

- **内置 layout manager**：线性纵向、线性横向、网格（Grid）；并预留自定义接口。
- **adapter 接口**：纯 C 回调风格（RecyclerView 风），支持多 view type。
- **item 尺寸**：等尺寸（同一滚动方向上统一），简化定位/回收算法。
- **交互**：触摸/鼠标拖动滚动、惯性 fling、`scroll_to` 程序滚动、数据变更刷新。

非目标（预留扩展，本期不做）：可变尺寸 item、瀑布流（StaggeredGrid）、item 增删动画、拖拽排序。

## 3. 命名与目录

- 目录：`src/recycle_view/`，结构遵循项目既有 widget 约定（`include/` + `src/` + `CMakeLists.txt`）。
- widget 类型名 / XML 标签：`recycle_view`
- 公开结构：`recycle_view_t`
- 构建产物：随其他 widget 一起由顶层 `src/CMakeLists.txt` 聚合，安装到 `3rdlib/<platform>/recycle_view/`。
- 目标平台：先在 x86 开发与单元测试，随后跟随项目支持 t113 / t507 / cv181。

## 4. 整体分层

```
recycle_view_t  (widget_t 子类 —— 容器/调度核心)
   ├── recycle_adapter_t*        数据 → item 视图（使用者实现回调）
   ├── recycle_layout_manager_t* 摆放规则（内置 3 种 + 可自定义）
   └── 内部状态：
        offset (xoffset/yoffset)、可见 item 映射表、按 view_type 分桶的回收池、
        fling 速度/定时器、content 总尺寸
```

职责边界（任意一层内部实现可替换而不影响其他两层）：

- **adapter**：只关心"有多少条数据、第 i 条是什么类型、怎么造一个该类型的空壳、怎么把第 i 条数据填进一个壳"。不关心滚动和位置。
- **layout_manager**：只关心"等尺寸是多少、内容总尺寸多大、给定滚动 offset 哪些 index 可见、把第 i 个 item 摆在哪个矩形"。不关心数据内容，也不碰回收池。
- **recycle_view 核心**：调度者。监听 offset 变化 → 问 layout_manager 要可见区间 → 把出界的 item 回收进池、给新进入的 index 从池里取壳（或让 adapter 新建）、调 adapter 绑定、调 layout_manager 摆放。负责手势 / fling / 事件。

## 5. 接口设计

### 5.1 adapter（使用者实现）

```c
typedef struct _recycle_adapter_t recycle_adapter_t;
struct _recycle_adapter_t {
  /* 必填：数据条数 */
  int32_t (*get_item_count)(recycle_adapter_t* adapter);

  /* 可选：第 index 条的 view_type；为 NULL 时所有项视为类型 0 */
  int32_t (*get_item_type)(recycle_adapter_t* adapter, int32_t index);

  /* 必填：为某 view_type 创建一个"空壳"item 控件（不填数据）。
   * parent 即 recycle_view，使用者用 C API 手搓控件或 ui_loader 加载模板都可 */
  widget_t* (*create_item_view)(recycle_adapter_t* adapter, widget_t* recycle_view, int32_t view_type);

  /* 必填：把第 index 条数据填进 item 控件（item 可能是从回收池复用来的） */
  ret_t (*bind_item_view)(recycle_adapter_t* adapter, widget_t* item, int32_t index);

  /* 可选：item 被回收进池时回调，做解绑/清理；为 NULL 跳过 */
  ret_t (*on_item_recycled)(recycle_adapter_t* adapter, widget_t* item, int32_t view_type);

  /* 可选：adapter 自身资源销毁 */
  ret_t (*on_destroy)(recycle_adapter_t* adapter);

  void* ctx;  /* 使用者自定义数据指针 */
};
```

### 5.2 layout_manager（内置 3 种 + 可自定义）

核心约定：layout manager 在**内容坐标系**（完整虚拟画布，未减 offset）里思考；core 负责减去 offset 转成控件坐标。

```c
typedef struct _recycle_layout_manager_t recycle_layout_manager_t;
struct _recycle_layout_manager_t {
  bool_t is_horizontal;  /* 滚动轴：TRUE=横向滚动, FALSE=纵向滚动 */

  /* 单个 item 尺寸（等尺寸）。可读 rv->w/h 决定填充交叉轴 */
  ret_t (*get_item_size)(recycle_layout_manager_t* lm, widget_t* rv, wh_t* w, wh_t* h);

  /* 沿滚动轴的内容总尺寸，用于 clamp offset / 滚动条 */
  int32_t (*get_content_size)(recycle_layout_manager_t* lm, widget_t* rv, int32_t item_count);

  /* 给定滚动 offset，算出可见 index 闭区间 [first,last]（预取冗余由 core 追加） */
  ret_t (*get_visible_range)(recycle_layout_manager_t* lm, widget_t* rv,
                             int32_t offset, int32_t item_count, int32_t* first, int32_t* last);

  /* 第 index 个 item 在内容坐标系中的矩形 */
  ret_t (*get_item_rect)(recycle_layout_manager_t* lm, widget_t* rv, int32_t index, rect_t* r);

  ret_t (*on_destroy)(recycle_layout_manager_t* lm);
  void* ctx;
};

/* 内置构造函数：item_extent = 沿滚动轴的尺寸（纵向=行高, 横向=列宽） */
recycle_layout_manager_t* recycle_linear_layout_manager_create(bool_t horizontal, int32_t item_extent);
/* 网格：span_count = 交叉轴等分数（纵向滚动=列数）；交叉轴尺寸 = viewport_cross / span_count */
recycle_layout_manager_t* recycle_grid_layout_manager_create(bool_t horizontal, int32_t span_count, int32_t item_extent);
```

### 5.3 recycle_view 核心（公开 API）

```c
widget_t* recycle_view_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h);
recycle_view_t* recycle_view_cast(widget_t* widget);

/* set_* 后 recycle_view 接管所有权，销毁时调用对应 on_destroy 释放 */
ret_t recycle_view_set_adapter(widget_t* widget, recycle_adapter_t* adapter);
ret_t recycle_view_set_layout_manager(widget_t* widget, recycle_layout_manager_t* lm);

ret_t recycle_view_scroll_to_index(widget_t* widget, int32_t index, bool_t animate);
ret_t recycle_view_scroll_to_offset(widget_t* widget, int32_t offset, bool_t animate);
ret_t recycle_view_notify_data_changed(widget_t* widget);  /* 数据集变化：重算 count + 重绑可见项 */

ret_t recycle_view_register(void);
```

**所有权约定**：`set_adapter` / `set_layout_manager` 后控件持有它们，控件销毁时依次调用各自 `on_destroy`。`item_extent` 等参数在构造时给定（等尺寸首期最简单、可预测）。

## 6. 核心算法与数据流

### 6.1 内部状态

```c
struct _recycle_view_t {
  widget_t widget;
  recycle_adapter_t* adapter;
  recycle_layout_manager_t* layout_manager;

  int32_t xoffset;
  int32_t yoffset;          /* 当前滚动 offset（沿滚动轴使用其一） */
  int32_t item_count;       /* 缓存的 adapter->get_item_count() */

  /* 可见项映射：index -> 当前挂载的 item 控件 */
  darray_t* visible_items;  /* 元素含 {index, view_type, widget_t* w} */

  /* 回收池：按 view_type 分桶，存 detach 的空闲控件 */
  darray_t* recycle_pools;  /* 元素含 {view_type, darray_t* free_widgets} */

  /* fling */
  velocity_t velocity;      /* AWTK 自带速度追踪 */
  float_t fling_v;          /* 当前 fling 速度 */
  uint32_t fling_timer_id;
  bool_t dragging;
  point_t down_point;

  /* scroll_to 动画 */
  uint32_t scroll_animator_id;
};
```

### 6.2 核心 relayout（offset / size / 数据变化都走它）

```
relayout():
  1. clamp offset 到 [0, max(0, content_size - viewport)]
  2. [first,last] = layout_manager.get_visible_range(offset)   // 含预取冗余 ±1
  3. 回收阶段：遍历 visible_items，凡 index ∉ [first,last]：
        - widget_remove_child(rv, w)         // detach，不销毁
        - adapter.on_item_recycled(w)         // 可选清理
        - 按 view_type 放回 recycle_pools
        - 从 visible_items 移除
  4. 填充阶段：for index in [first,last] 且不在 visible_items：
        - type = adapter.get_item_type(index)
        - w = 从 type 对应池 pop()；池空则 adapter.create_item_view(type)
        - adapter.bind_item_view(w, index)
        - widget_add_child(rv, w)
        - rect = layout_manager.get_item_rect(index); widget_move_resize(w, rect - offset)
  5. 触发重绘
```

**复用关键**：步骤 3 detach 的控件不销毁，进池；步骤 4 优先从同 type 池里取来复用，只重新 `bind` + `move`。滑动时进出 1~2 个控件，池大小 ≈ 一屏可见数 + 冗余，与数据总量无关 —— 这就是 RecyclerView 的本质。

### 6.3 各入口如何驱动 relayout

| 入口 | 处理 |
|---|---|
| **拖动** | `POINTER_DOWN` 记起点 + 启动 velocity 追踪；`POINTER_MOVE` 累加位移到 offset → relayout；`POINTER_UP` 取末速度启动 fling |
| **fling** | 定时器（FPS≈30/60）按摩擦系数衰减 `fling_v`，每帧 offset += v → relayout；\|v\| < 阈值 或 offset 触边 时停 |
| **scroll_to_offset / index** | index 经 `get_item_rect` 换算成目标 offset；`animate=TRUE` 用 widget_animator 插值改 offset 每帧 relayout，否则直接设 offset + relayout |
| **notify_data_changed** | 重读 `item_count`；对仍可见的 index 重新 `bind`（数据变了但壳还在），新增/减少的进/出走标准回收填充；clamp offset |
| **resize / 设置 adapter·lm** | 直接全量 relayout（视口变了 item 尺寸 / 可见数都变） |

### 6.4 边界与错误处理

- adapter 或 layout_manager 任一为空 → relayout 直接 return（控件空白，不崩）。
- `item_count == 0` → 回收所有可见项，content_size = 0。
- `create_item_view` 返回 NULL → 跳过该 index 并告警，不崩。
- 控件销毁：停 fling / scroll 定时器 → 清空 visible_items 与所有池中控件（销毁）→ 调 adapter / lm 的 `on_destroy`。
- offset 始终 clamp，空数据 / 超短列表不会滚过界。

## 7. 测试

仿 `breath_ellipse` 模式，把纯算法抽成可单测的函数，建 `src/recycle_view/tests/`：

- 线性 / 网格 layout manager 的 `get_visible_range`、`get_item_rect`、`get_content_size`（给定 offset / viewport / count 断言区间与矩形）。
- fling 速度衰减、offset clamp 边界。
- 回收池 push/pop 复用计数（绑定数据集驱动滑动后，`create_item_view` 调用次数应远小于数据量 —— 验证"确实在复用"）。

## 8. 文件清单（预期）

```
src/recycle_view/
  CMakeLists.txt
  include/
    recycle_view.h               # recycle_view_t + 公开 API
    recycle_adapter.h            # recycle_adapter_t
    recycle_layout_manager.h     # recycle_layout_manager_t + 内置构造函数声明
  src/
    recycle_view.c               # 核心调度 + relayout + 事件 + fling + scroll_to
    recycle_view_register.c      # 注册
    recycle_linear_layout_manager.c
    recycle_grid_layout_manager.c
  tests/
    recycle_view_test.c          # 纯算法单测
```
