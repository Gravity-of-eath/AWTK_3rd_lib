#include "awtk.h"
#include "src/conner_gradient/include/conner_gradient_view.h"
#include "src/conner_gradient/include/conner_gradient_view_register.h"

static ret_t on_quit(void* ctx, event_t* e) {
  tk_quit();
  return RET_OK;
}

static ret_t on_timer(const timer_info_t* info) {
  widget_t* widget = WIDGET(info->ctx);
  conner_gradient_view_t* view = CONNER_GRADIENT_VIEW(widget);
  
  // 更新当前值以显示动画效果
  view->current = (view->current + 1) % (view->max + 1);
  widget_invalidate(widget, NULL);
  
  return RET_REPEAT;
}

/**
 * 应用程序入口
 */
ret_t application_init(void) {
  widget_t* win = window_create(NULL, 0, 0, 0, 0);
  widget_t* view = conner_gradient_view_create(win, 50, 50, 200, 200);
  
  if (view == NULL) {
    log_debug("Failed to create conner_gradient_view\n");
    return RET_FAIL;
  }
  
  // 设置属性 - 从0度到90度
  conner_gradient_view_set_angles(view, 0, 90);
  conner_gradient_view_set_max(view, 100);
  conner_gradient_view_set_current(view, 100); // 显示完整角度范围
  
  // 设置颜色
  widget_set_prop_str(view, CONNER_GRADIENT_VIEW_PROP_START_COLOR, "#FF0000"); // 红色
  widget_set_prop_str(view, CONNER_GRADIENT_VIEW_PROP_STOP_COLOR, "#0000FF");  // 蓝色
  
  // 添加关闭按钮
  widget_t* close_btn = button_create(win, 10, 10, 80, 30);
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