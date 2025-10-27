#include "awtk.h"
#include "src/conner_gradient/include/conner_gradient_view.h"
#include "src/conner_gradient/include/conner_gradient_view_register.h"

static ret_t on_quit(void* ctx, event_t* e) {
  tk_quit();
  return RET_OK;
}

/**
 * 应用程序入口
 */
ret_t application_init(void) {
  widget_t* win = window_create(NULL, 0, 0, 0, 0);
  
  // 创建一个完整的扇形（full_ratio = 1.0）
  widget_t* view1 = conner_gradient_view_create(win, 10, 10, 150, 150);
  if (view1 != NULL) {
    conner_gradient_view_set_angles(view1, 0, 270);  // 0度到270度
    widget_set_prop_float(view1, "full_ratio", 1.0f);  // 绘制完整扇形
    widget_set_prop_str(view1, CONNER_GRADIENT_VIEW_PROP_START_COLOR, "#FF0000"); // 红色
    widget_set_prop_str(view1, CONNER_GRADIENT_VIEW_PROP_STOP_COLOR, "#0000FF");  // 蓝色
  }
  
  // 创建一个弧环（full_ratio = 0.5）
  widget_t* view2 = conner_gradient_view_create(win, 170, 10, 150, 150);
  if (view2 != NULL) {
    conner_gradient_view_set_angles(view2, 0, 270);  // 0度到270度
    widget_set_prop_float(view2, "full_ratio", 0.5f);  // 绘制弧环
    widget_set_prop_str(view2, CONNER_GRADIENT_VIEW_PROP_START_COLOR, "#00FF00"); // 绿色
    widget_set_prop_str(view2, CONNER_GRADIENT_VIEW_PROP_STOP_COLOR, "#FFFF00");  // 黄色
  }
  
  // 添加关闭按钮
  widget_t* close_btn = button_create(win, 10, 170, 80, 30);
  widget_set_text(close_btn, L"Close");
  widget_on(close_btn, EVT_CLICK, on_quit, NULL);
  
  widget_layout(win);
  return RET_OK;
}

/**
 * 应用程序退出
 */
ret_t application_exit(void) {
  log_debug("Application exited\n");
  return RET_OK;
}

ret_t app_init_plugins(void) {
  conner_gradient_view_register();
  return RET_OK;
}

ret_t app_deinit_plugins(void) {
  return RET_OK;
}