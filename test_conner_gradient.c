#include "base/widget.h"
#include "src/conner_gradient/include/conner_gradient_view.h"

static ret_t on_quit(void* ctx, event_t* e) {
  tk_quit();
  return RET_OK;
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
  
  // 设置属性
  conner_gradient_view_set_angles(view, 0, 1.5 * M_PI); // 270度
  conner_gradient_view_set_max(view, 100);
  conner_gradient_view_set_current(view, 75);
  
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

#include "src/conner_gradient/include/conner_gradient_view_register.h"

ret_t app_init_plugins(void) {
  conner_gradient_view_register();
  return RET_OK;
}

ret_t app_deinit_plugins(void) {
  return RET_OK;
}