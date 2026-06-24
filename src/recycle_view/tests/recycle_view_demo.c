/**
 * 手动集成示例（非默认构建）：演示 recycle_view 的 adapter + layout_manager 用法，
 * 并通过统计 create_item_view 调用次数验证"确实在复用"。
 * 需在完整 AWTK 应用中调用 demo_recycle_view_create(win) 后滚动观察日志。
 */
#include "recycle_view.h"
#include "tkc/log.h"
#include "tkc/mem.h"
#include "tkc/utils.h"
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
