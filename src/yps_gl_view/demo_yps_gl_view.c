/*
 * File: demo_yps_gl_view.c
 * Description: YPS GL View控件演示程序
 * Author: Li XianJing <xianjimli@hotmail.com>
 * Date: 2025-01-10
 */

#include "awtk.h"
#include "yps_gl_view/yps_gl_view.h"
#include "yps_gl_view/yps_gl_view_register.h"

static ret_t app_init(void) {
  widget_t* win = window_create(NULL, 0, 0, 0, 0);
  widget_set_text_utf8(win, "YPS GL View Demo");
  
  // 创建YPS GL View控件
  widget_t* gl_view = yps_gl_view_create(win, 0, 0, 0, 0);
  widget_set_self_layout_params(gl_view, "0", "0", "100%", "100%");
  
  // 设置场景文件和渲染管线文件
  yps_gl_view_set_scene_file(gl_view, "model.scene.xml");
  yps_gl_view_set_pipeline_file(gl_view, "pipeline.xml");
  
  // 设置控件属性
  widget_set_prop_str(gl_view, WIDGET_PROP_ANCHOR_DEFAULT, "left=top=0;right=0;bottom=0");
  
  str_t str;
  str_init(&str, 0);
  str_set(&str, "YPS GL View控件演示");
  widget_set_prop_str(gl_view, "desc", str.str);
  str_reset(&str);
  
  widget_layout(win);
  
  return RET_OK;
}

static ret_t app_exit(void) {
  log_debug("app_exit\n");
  
  return RET_OK;
}

#include "awtk_main.inc"