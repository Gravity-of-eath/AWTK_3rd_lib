/**
 * recycle_view coverflow AWTK GUI 测试程序（x86 桌面）
 *
 * 演示布局下放（layout_children 钩子）+ coverflow 布局管理器：
 *   - 一组卡片，焦点项居中放大且置于最上层（zorder 最高）
 *   - 两侧对称缩小、间距重叠，被中心焦点压住内缘
 *   - 自动推进焦点（scroll_to_index 平滑过渡），观察缩放/层级过渡
 *   - 统计 create_item_view 次数，验证回收复用（只创建约 2*radius+3 个控件）
 *   - 每张卡片结构相同：一个 view 容器 + 内部居中 label，供 coverflow 的 DFS 基线复用
 */

#include <stdio.h>
#include "awtk.h"
#include "widgets/view.h"
#include "widgets/label.h"
#include "recycle_view.h"
#include "recycle_view_register.h"

#define APP_NAME "recycle_view_coverflow_test"

#define DEMO_ITEM_COUNT 20
#define CARD_W 200
#define CARD_H 280
#define CF_STEP 180
#define CF_SPACING 130
#define CF_RADIUS 2 /* 5 个：中间 1 + 每侧 2 */

static widget_t* s_rv = NULL;
static int32_t s_focus = 0;

typedef struct _demo_adapter_t {
  recycle_adapter_t base;
  int32_t create_count;
} demo_adapter_t;

static demo_adapter_t s_adapter;

static int32_t demo_get_item_count(recycle_adapter_t* a) {
  (void)a;
  return DEMO_ITEM_COUNT;
}

/* 卡片：满尺寸 view 容器 + 内部居中 label（所有卡片结构一致） */
static widget_t* demo_create_item_view(recycle_adapter_t* a, widget_t* rv, int32_t view_type) {
  demo_adapter_t* da = (demo_adapter_t*)a;
  widget_t* card = NULL;
  widget_t* lbl = NULL;
  (void)view_type;
  da->create_count++;
  card = view_create(NULL, 0, 0, CARD_W, CARD_H);
  widget_set_style_int(card, "normal:round_radius", 12);
  widget_set_style_int(card, "normal:border_width", 2);
  widget_set_style_str(card, "normal:border_color", "#ffffff");
  lbl = label_create(card, 0, CARD_H / 2 - 20, CARD_W, 40);
  widget_set_style_int(lbl, "normal:font_size", 28);
  widget_set_style_str(lbl, "normal:text_color", "#ffffff");
  printf("coverflow: create_item_view #%d (数据共 %d 条)\n", da->create_count, DEMO_ITEM_COUNT);
  fflush(stdout);
  return card;
}

static ret_t demo_bind_item_view(recycle_adapter_t* a, widget_t* item, int32_t index) {
  widget_t* lbl = widget_get_child(item, 0);
  char text[32];
  (void)a;
  /* 卡片底色随 index 变化，便于肉眼区分与观察复用 */
  static const char* colors[] = {"#c0392b", "#2980b9", "#27ae60", "#8e44ad", "#d35400"};
  widget_set_style_str(item, "normal:bg_color", colors[index % 5]);
  if (lbl != NULL) {
    tk_snprintf(text, sizeof(text), "卡片 %d", index);
    widget_set_text_utf8(lbl, text);
  }
  return RET_OK;
}

/* 自动推进焦点：每 700ms 切到下一张，到末项后打印汇总并退出 */
static ret_t on_advance(const timer_info_t* timer) {
  (void)timer;
  if (s_rv == NULL) return RET_REMOVE;
  if (s_focus >= DEMO_ITEM_COUNT - 1) {
    printf(
        "coverflow: 扫描完成 —— item_count=%d, create_item_view 调用=%d 次"
        "（≈2*radius+3=%d），证明回收复用生效。\n",
        DEMO_ITEM_COUNT, s_adapter.create_count, 2 * CF_RADIUS + 3);
    fflush(stdout);
    tk_quit();
    return RET_REMOVE;
  }
  s_focus++;
  recycle_view_scroll_to_index(s_rv, s_focus, TRUE);
  printf("coverflow: focus -> %d\n", s_focus);
  fflush(stdout);
  return RET_REPEAT;
}

ret_t application_init(void) {
  widget_t* win = NULL;
  widget_t* title = NULL;

  recycle_view_register();

  win = window_create_default();
  widget_set_style_str(win, "normal:bg_color", "#11161f");

  title = label_create(win, 0, 10, 760, 28);
  widget_set_text_utf8(title, "recycle_view coverflow: 焦点居中放大置顶，两侧对称缩小被压");
  widget_set_style_str(title, "normal:text_color", "#ffffff");
  widget_set_style_int(title, "normal:font_size", 18);

  /* 横向 coverflow 视口：宽 760，高 360（足够放下 280 高卡片） */
  s_rv = recycle_view_create(win, 0, 50, 760, 360);
  widget_set_style_str(s_rv, "normal:bg_color", "#0a0e15");

  memset(&s_adapter, 0, sizeof(s_adapter));
  s_adapter.base.get_item_count = demo_get_item_count;
  s_adapter.base.create_item_view = demo_create_item_view;
  s_adapter.base.bind_item_view = demo_bind_item_view;

  recycle_view_set_layout_manager(
      s_rv, recycle_coverflow_layout_manager_create(TRUE, CF_STEP, CF_SPACING, CARD_W, CARD_H,
                                                    CF_RADIUS, 0.18f, 0.6f));
  recycle_view_set_adapter(s_rv, (recycle_adapter_t*)&s_adapter);

  printf("coverflow: 初始化后 create_item_view=%d 次（仅首屏可见项）\n", s_adapter.create_count);
  fflush(stdout);

  timer_add(on_advance, NULL, 700);
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
#define LCD_HEIGHT 440
#define APP_TYPE APP_DESKTOP

#include "awtk_main.inc"
