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
#include "base/vgcanvas.h"

BEGIN_C_DECLS

typedef struct _yps_gl_view_t {
  widget_t widget;

  /* private */
  char* scene_file;           /* 场景文件路径 */ 
  char** mode_lists;           /* 模型文件路径列表（当有scene_file时忽略优先加载scene_file） */ 
  int32_t mode_count;           /* 模型文件个数 */ 
  char* content_dir;          /* 内容资源目录路径 */ 
  //uint32_t target_fps; //无须手动控制FPS，AWTK应用会设置FPS，并在每一帧回调控件的on_paint

  /* FBO & Texture integration */
  bitmap_t* bitmap;           /* AWTK wrapper for GPU texture */
  framebuffer_object_t fbo;   /* AWTK-managed FBO info */
  bool_t use_readback;        /* TRUE: glReadPixels, FALSE: zero-copy */
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
 * @method yps_gl_view_cast
 * @annotation ["cast"]
 * 转换为yps_gl_view对象(供脚本语言使用)。
 * @param {widget_t*} widget yps_gl_view对象。
 *
 * @return {widget_t*} yps_gl_view对象。
 */
widget_t* yps_gl_view_cast(widget_t* widget);

/**
 * @method yps_gl_view_set_scene_file
 * @annotation ["scriptable"]
 * 设置场景文件（.scene，需要 Plugin_DotScene）。设置后下一帧生效。
 * 传NULL或空串表示不使用场景文件，此时按模型列表加载。
 * @param {widget_t*} widget 控件对象。
 * @param {const char*} scene_file 场景文件路径。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_gl_view_set_scene_file(widget_t* widget, const char* scene_file);

/**
 * @method yps_gl_view_get_scene_file
 * @annotation ["scriptable"]
 * 获取场景文件路径。
 * @param {widget_t*} widget 控件对象。
 *
 * @return {const char*} 返回场景文件路径。
 */
const char* yps_gl_view_get_scene_file(widget_t* widget);

/**
 * @method yps_gl_view_set_model_list
 * @annotation ["scriptable"]
 * 设置模型（.mesh）列表，多个模型之间用英文分号或逗号分隔。设置后下一帧生效。
 * 注意：scene_file 非空时优先加载 scene_file，模型列表被忽略。
 * @param {widget_t*} widget 控件对象。
 * @param {const char*} model_list 模型文件路径列表，如 "a.mesh;b.mesh"，可为NULL表示清空。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_gl_view_set_model_list(widget_t* widget, const char* model_list);

/**
 * @method yps_gl_view_set_models
 * 设置模型（.mesh）列表。设置后下一帧生效。
 * @param {widget_t*} widget 控件对象。
 * @param {const char**} files 模型文件路径数组，可为NULL表示清空。
 * @param {uint32_t} count 模型个数。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_gl_view_set_models(widget_t* widget, const char** files, uint32_t count);

/**
 * @method yps_gl_view_set_content_dir
 * @annotation ["scriptable"]
 * 设置内容资源目录。必须在首次绘制之前设置才有效。
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
 * 动态切换场景文件，下一帧生效。
 * @param {widget_t*} widget 控件对象。
 * @param {const char*} scene_file 场景文件路径。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_gl_view_switch_scene(widget_t* widget, const char* scene_file);

/**
 * @method yps_gl_view_reload_scene
 * @annotation ["scriptable"]
 * 重新加载当前场景（或模型列表），下一帧生效。
 * @param {widget_t*} widget 控件对象。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_gl_view_reload_scene(widget_t* widget);

/**
 * @method yps_gl_view_unload_scene
 * @annotation ["scriptable"]
 * 卸载场景内容（保留GL上下文，画面变为空场景）。
 * 之后可调用 yps_gl_view_reload_scene 恢复。
 * @param {widget_t*} widget 控件对象。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_gl_view_unload_scene(widget_t* widget);

/**
 * @method yps_gl_view_set_target_fps
 * @annotation ["scriptable"]
 * 设置目标帧率（控件内部定时器触发重绘的频率）。
 * @param {widget_t*} widget 控件对象。
 * @param {uint32_t} fps 目标帧率。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_gl_view_set_target_fps(widget_t* widget, uint32_t fps);

/**
 * @method yps_gl_view_get_target_fps
 * @annotation ["scriptable"]
 * 获取目标帧率。
 * @param {widget_t*} widget 控件对象。
 *
 * @return {uint32_t} 返回目标帧率。
 */
uint32_t yps_gl_view_get_target_fps(widget_t* widget);

/**
 * @method yps_gl_view_set_readback
 * @annotation ["scriptable"]
 * 设置是否使用回读模式。
 * @param {widget_t*} widget 控件对象。
 * @param {bool_t} use_readback TRUE表示使用回读。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_gl_view_set_readback(widget_t* widget, bool_t use_readback);

#define WIDGET_TYPE_YPS_GL_VIEW "yps_gl_view"
#define YPS_GL_VIEW_PROP_SCENE_FILE "scene_file"
#define YPS_GL_VIEW_PROP_MODEL_LIST "model_list"
#define YPS_GL_VIEW_PROP_CONTENT_DIR "content_dir"
#define YPS_GL_VIEW_PROP_USE_READBACK "use_readback"
#define YPS_GL_VIEW_PROP_TARGET_FPS "target_fps"

#define YPS_GL_VIEW(widget) ((yps_gl_view_t*)(yps_gl_view_cast(WIDGET(widget))))

/*public for subclass and runtime type check*/
TK_EXTERN_VTABLE(yps_gl_view);

END_C_DECLS

#endif /*YPS_GL_VIEW_H*/
