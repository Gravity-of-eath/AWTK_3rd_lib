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
#include "horde3d/horde3d.h"
#include "horde3d/egInit.h"

BEGIN_C_DECLS

/**
 * @class yps_gl_view_t
 * @parent widget_t
 * @annotation ["scriptable","design:widget","widget"]
 * @order -10
 * @alternative_name yps.gl_view
 * @brief 3D OpenGL view控件。
 *
 * 该控件用于渲染3D场景，基于Horde3D引擎实现。
 *
 * 示例：
 *
 * ```xml
 * <!-- 用XML创建 -->
 * <yps_gl_view x="0" y="0" w="100%" h="100%" scene_file="model.scene.xml" 
 *              pipeline_file="pipeline.xml"/>
 * ```
 *
 * ```c
 * // 用C创建
 * widget_t* yps_gl_view = yps_gl_view_create(parent, 0, 0, 0, 0);
 * yps_gl_view_set_scene_file(yps_gl_view, "model.scene.xml");
 * yps_gl_view_set_pipeline_file(yps_gl_view, "pipeline.xml");
 * ```
 */

/**
 * @property {char*} scene_file
 * @annotation ["set_prop","get_prop","scriptable"]
 * 场景文件。
 */

/**
 * @property {char*} pipeline_file
 * @annotation ["set_prop","get_prop","scriptable"]
 * 渲染管线文件。
 */

typedef struct _yps_gl_view_t {
  widget_t widget;
  
  /* private */
  char* scene_file;           /* 场景文件路径 */
  char* pipeline_file;        /* 渲染管线文件路径 */
  EGLWindowData* window_data; /* 窗口数据 */
  H3DNode _cam;
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
 * @method yps_gl_view_set_pipeline_file
 * @annotation ["scriptable"]
 * 设置渲染管线文件。
 * @param {widget_t*} widget 控件对象。
 * @param {const char*} pipeline_file 渲染管线文件路径。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_gl_view_set_pipeline_file(widget_t* widget, const char* pipeline_file);

#define YPS_GL_VIEW_PROP_SCENE_FILE "scene_file"
#define YPS_GL_VIEW_PROP_PIPELINE_FILE "pipeline_file"

#define YPS_GL_VIEW(widget) ((yps_gl_view_t*)(widget))

END_C_DECLS

#endif /*YPS_GL_VIEW_H*/