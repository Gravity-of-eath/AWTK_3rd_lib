/**
 * File:   main.c
 * Author: AWTK Develop Team
 * Brief:  Test application for yps_gl_view widget
 *
 * Copyright (c) 2018-2025 Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2025-01-10 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#include "awtk.h"
#include "ext_widgets/ext_widgets.h"
#include "yps_gl_view.h"

ret_t open_yps_gl_view_window(void* ctx, event_t* e) {
  widget_t* win = window_open("yps_gl_view");
  widget_t* view = widget_lookup(win, "yps_gl_view", TRUE);
  
  // Set properties for the 3D view
  yps_gl_view_set_scene_file(view, "models/scene.xml");
  yps_gl_view_set_pipeline_file(view, "pipelines/pipeline.xml");
  yps_gl_view_set_auto_start(view, TRUE);
  yps_gl_view_set_refresh_interval(view, 16); // ~60 FPS
  
  return RET_OK;
}

ret_t application_init(void) {
  widget_t* win = window_create(NULL, 0, 0, 0, 0);
  widget_t* view = yps_gl_view_create(win, 0, 0, 0, 0);
  
  // Set properties for the 3D view
  yps_gl_view_set_scene_file(view, "models/scene.xml");
  yps_gl_view_set_pipeline_file(view, "pipelines/pipeline.xml");
  yps_gl_view_set_auto_start(view, TRUE);
  yps_gl_view_set_refresh_interval(view, 16); // ~60 FPS
  
  // Set layout
  widget_set_self_layout_params(view, "0", "0", "100%", "100%");
  
  return RET_OK;
}

ret_t application_exit(void) {
  return RET_OK;
}