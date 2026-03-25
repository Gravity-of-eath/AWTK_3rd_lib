/**
 * File:   yps_gl_view.h
 * Author: AWTK Develop Team
 * Brief:  3D OpenGL view widget
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
 * 2025-01-09 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#ifndef YPS_GL_VIEW_H
#define YPS_GL_VIEW_H

#include "tkc/types_def.h"
#include "base/widget.h"
#include "base/canvas.h"
#include "base/image_manager.h"
#include "base/system_info.h"
#include "base/asset_loader.h"

BEGIN_C_DECLS

  

typedef struct _yps_gl_view_t {
  widget_t widget;

  /* private */
  char* scene_file;           /* 场景文件路径 */ 

  /* 新增字段 - 资源管理 */
  char* content_dir;          /* 内容资源目录路径（替代硬编码） */ 
  char** model_list;          /* 要加载的模型列表 */ 
  uint32_t target_fps;
  
} yps_gl_view_t;

/**
 * @method yps_gl_view_create
 * @annotation ["constructor", "scriptable"]
 * @param {widget_t*} parent 父控件
 * @param {xy_t} x x坐标
 * @param {xy_t} y y坐标
 * @param {wh_t} w 宽度
 * @param {wh_t} h 高度
 *
 * @return {widget_t*} 控件对象。
 */
widget_t* yps_gl_view_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h);

/**
 * @method yps_gl_view_set_scene_file
 * @annotation ["scriptable"]
 * 设置场景文件。
 * @param {widget_t*} widget 控件对象。
 * @param {const char*} scene_file 场景文件路径。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_gl_view_set_scene_file(widget_t* widget, const char* scene_file);



/**
 * @method yps_gl_view_set_content_dir
 * @annotation ["scriptable"]
 * 设置内容资源目录。
 * @param {widget_t*} widget 控件对象。
 * @param {const char*} content_dir 内容资源目录路径。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_gl_view_set_content_dir(widget_t* widget, const char* content_dir);

/**
 * @method yps_gl_view_get_content_dir
 * @annotation ["scriptable"]
 * 获取内容资源目录。
 * @param {widget_t*} widget 控件对象。
 *
 * @return {const char*} 返回内容资源目录路径。
 */
const char* yps_gl_view_get_content_dir(widget_t* widget);

/**
 * @method yps_gl_view_switch_scene
 * @annotation ["scriptable"]
 * 动态切换场景文件。
 * @param {widget_t*} widget 控件对象。
 * @param {const char*} scene_file 场景文件路径。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_gl_view_switch_scene(widget_t* widget, const char* scene_file);

/**
 * @method yps_gl_view_reload_scene
 * @annotation ["scriptable"]
 * 重新加载当前场景。
 * @param {widget_t*} widget 控件对象。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_gl_view_reload_scene(widget_t* widget);

/**
 * @method yps_gl_view_unload_scene
 * @annotation ["scriptable"]
 * 卸载当前场景。
 * @param {widget_t*} widget 控件对象。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_gl_view_unload_scene(widget_t* widget);



/**
 * @method yps_gl_view_set_target_fps
 * @annotation ["scriptable"]
 * 设置目标帧率。
 * @param {widget_t*} widget 控件对象。
 * @param {uint32_t} fps 目标帧率。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_gl_view_set_target_fps(widget_t* widget, uint32_t fps);

#define WIDGET_TYPE_YPS_GL_VIEW "yps_gl_view"
#define YPS_GL_VIEW_PROP_SCENE_FILE "scene_file"
#define YPS_GL_VIEW_PROP_PIPELINE_FILE "pipeline_file"
#define YPS_GL_VIEW_PROP_CONTENT_DIR "content_dir"


#define YPS_GL_VIEW(widget) ((yps_gl_view_t*)(widget))

END_C_DECLS

#endif /*YPS_GL_VIEW_H*/