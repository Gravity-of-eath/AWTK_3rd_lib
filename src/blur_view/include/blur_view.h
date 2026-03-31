/**
 * File:   blur_view.h
 * Author:
 * Brief:
 *
 * Copyright (c) 2025 - 2025
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
 * 2025-7-17  created
 *
 */

#ifndef TK_BLUR_VIEW_H
#define TK_BLUR_VIEW_H

#include "base/widget.h"
#include "base/vgcanvas.h"

BEGIN_C_DECLS
/**
 * @class blur_view_t
 * @parent widget_t
 * @annotation ["scriptable","design","widget"]
 *
 * 在xml中使用"line\_chart"标签创建控件。如：
 *
 * ```xml
 * <!-- ui -->
 * <blur_view x="c" y="50" w="100" h="100"/>
 * ```
 *
 * 可用通过style来设置控件的显示风格，如字体的大小和颜色等等。如：
 *
 * ```xml
 * <!-- style -->
 * <blur_view>
 *   <style name="default" font_size="32">
 *     <normal text_color="black" />
 *   </style>
 * </blur_view>
 * ```
 */
typedef struct _blur_view_t
{
  widget_t widget;

  float_t radius;

  bitmap_t* blur_bitmap;

  bool_t DEBUG;

  bool_t abort;

  /**
   * 正在截图标志，防止 widget_take_snapshot_rect 触发自身 on_paint 导致死递归
   */
  bool_t is_capturing;

  /**
   * 上次更新的时间戳(ms)，用于节流控制刷新频率
   */
  uint64_t last_update_ms;

  /**
   * 最小更新间隔(ms)，默认33ms约30fps，可通过属性调节
   */
  uint32_t update_interval_ms;

  /**
   * 降采样因子，默认2表示1/2分辨率截图+模糊，越大越快但越模糊
   */
  uint32_t downscale;

  framebuffer_object_t fbo_a;
  int32_t fbo_w;
  int32_t fbo_h;
  bool_t fbo_inited;

} blur_view_t;

/**
 * @method blur_view_create
 * 创建blur_view对象
 * @param {widget_t*} parent 父控件
 * @param {xy_t} x x坐标
 * @param {xy_t} y y坐标
 * @param {wh_t} w 宽度
 * @param {wh_t} h 高度
 *
 * @return {widget_t*} blur_view对象。
 */
widget_t *blur_view_create(widget_t *parent, xy_t x, xy_t y, wh_t w, wh_t h);

/**
 * @method blur_view_cast
 * 转换为blur_view对象(供脚本语言使用)。
 * @annotation ["cast", "scriptable"]
 * @param {widget_t*} widget blur_view对象。
 *
 * @return {widget_t*} blur_view对象。
 */
widget_t *blur_view_cast(widget_t *widget);

ret_t blur_view_set_radius(widget_t *widget, float_t radius);

#define BLUR_VIEW_PROP_RADIUS "radius"
#define BLUR_VIEW_PROP_DEBUG "debug"
#define BLUR_VIEW_PROP_UPDATE_INTERVAL "update_interval"

#define WIDGET_TYPE_BLUR_VIEW "blur_view"

#define BLUR_VIEW(widget) ((blur_view_t *)(blur_view_cast(WIDGET(widget))))

/*public for subclass and runtime type check*/
TK_EXTERN_VTABLE(blur_view);

END_C_DECLS

#endif /*TK_BLUR_VIEW_H*/
