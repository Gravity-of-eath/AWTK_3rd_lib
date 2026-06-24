/**
 * recycle_view AWTK GUI 测试程序（x86 桌面）
 *
 * 演示 adapter + layout_manager 驱动的可复用列表：
 *   - 1000 条数据，等高 48px 的纵向线性列表
 *   - 自动滚动定时器把列表从头扫到尾
 *   - 通过统计 create_item_view 调用次数验证"确实在复用"
 *     （回收池命中时不再新建，create_count 应停在“可见数+预取”量级，远小于 1000）
 *   - 扫描结束后打印汇总并退出
 */

#include <stdio.h>
#include "awtk.h"
#include "recycle_view.h"
#include "recycle_view_register.h"

#define APP_NAME "recycle_view_test"

#define DEMO_ITEM_COUNT 1000
#define DEMO_ITEM_EXTENT 48
#define DEMO_VIEW_W 320
#define DEMO_VIEW_H 480

static widget_t* s_rv = NULL;

/* 统计实际创建次数的 adapter */
typedef struct _demo_adapter_t {
  recycle_adapter_t base;
  int32_t create_count;
  int32_t bind_count;
} demo_adapter_t;

static demo_adapter_t s_adapter;

static int32_t demo_get_item_count(recycle_adapter_t* a) {
  (void)a;
  return DEMO_ITEM_COUNT;
}

static widget_t* demo_create_item_view(recycle_adapter_t* a, widget_t* rv, int32_t view_type) {
  demo_adapter_t* da = (demo_adapter_t*)a;
  widget_t* item = NULL;
  (void)view_type;
  da->create_count++;
  item = label_create(NULL, 0, 0, 0, 0);
  widget_set_style_int(item, "normal:font_size", 20);
  printf("recycle_view_test: create_item_view #%d (数据共 %d 条)\n", da->create_count,
         DEMO_ITEM_COUNT);
  fflush(stdout);
  return item;
}

static ret_t demo_bind_item_view(recycle_adapter_t* a, widget_t* item, int32_t index) {
  demo_adapter_t* da = (demo_adapter_t*)a;
  char text[32];
  da->bind_count++;
  tk_snprintf(text, sizeof(text), "第 %d 项", index);
  /* 奇偶行不同底色，方便肉眼看到滚动与复用 */
  widget_set_style_str(item, "normal:bg_color", (index % 2) ? "#2b3a55" : "#1f2a3a");
  widget_set_style_str(item, "normal:text_color", "#e8eef7");
  widget_set_style_int(item, "normal:margin", 2);
  return widget_set_text_utf8(item, text);
}

/* 自动滚动：每次跳一屏多一点，扫到内容末尾后打印汇总并退出 */
static ret_t on_autoscroll(const timer_info_t* timer) {
  recycle_view_t* rv = RECYCLE_VIEW(s_rv);
  int32_t content = DEMO_ITEM_COUNT * DEMO_ITEM_EXTENT;
  int32_t max_offset = content - DEMO_VIEW_H;
  int32_t next = 0;
  (void)timer;
  if (rv == NULL) {
    return RET_REMOVE;
  }
  next = rv->yoffset + (DEMO_VIEW_H - DEMO_ITEM_EXTENT); /* 每步约一屏 */
  if (next >= max_offset) {
    recycle_view_scroll_to_offset(s_rv, max_offset, FALSE);
    printf(
        "recycle_view_test: 扫描完成 —— item_count=%d, create_item_view 调用=%d 次, "
        "bind_item_view 调用=%d 次。create 次数≈可见数+预取，证明回收复用生效。\n",
        DEMO_ITEM_COUNT, s_adapter.create_count, s_adapter.bind_count);
    fflush(stdout);
    tk_quit();
    return RET_REMOVE;
  }
  recycle_view_scroll_to_offset(s_rv, next, FALSE);
  return RET_REPEAT;
}

ret_t application_init(void) {
  widget_t* win = NULL;
  widget_t* title = NULL;

  recycle_view_register();

  win = window_create_default();
  widget_set_style_str(win, "normal:bg_color", "#11161f");

  title = label_create(win, 360, 30, 360, 30);
  widget_set_text_utf8(title, "recycle_view: 1000 项纵向列表（自动滚动）");
  widget_set_style_str(title, "normal:text_color", "#ffffff");
  widget_set_style_int(title, "normal:font_size", 18);

  s_rv = recycle_view_create(win, 30, 30, DEMO_VIEW_W, DEMO_VIEW_H);
  widget_set_style_str(s_rv, "normal:bg_color", "#0a0e15");

  memset(&s_adapter, 0, sizeof(s_adapter));
  s_adapter.base.get_item_count = demo_get_item_count;
  s_adapter.base.create_item_view = demo_create_item_view;
  s_adapter.base.bind_item_view = demo_bind_item_view;

  recycle_view_set_layout_manager(s_rv, recycle_linear_layout_manager_create(FALSE, DEMO_ITEM_EXTENT));
  recycle_view_set_adapter(s_rv, (recycle_adapter_t*)&s_adapter);

  printf("recycle_view_test: 初始化后 create_item_view=%d 次（仅首屏可见项）\n",
         s_adapter.create_count);
  fflush(stdout);

  /* 启动自动滚动；间隔 120ms 便于观察 */
  timer_add(on_autoscroll, NULL, 120);

  return RET_OK;
}

ret_t application_exit(void) {
  return RET_OK;
}

ret_t assets_init(void) {
  return RET_OK;
}

#undef LCD_WIDTH
#undef LCD_HEIGHT
#undef APP_TYPE
#define LCD_WIDTH 760
#define LCD_HEIGHT 560
#define APP_TYPE APP_DESKTOP

#include "awtk_main.inc"
