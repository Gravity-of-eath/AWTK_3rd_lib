/**
 * File:   auto_scale_view.h
 * Author: 云片松
 * Brief:  圆形表盘光带
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
 * 2025-11-12 wangdongpo created
 *
 */

#ifndef TK_YPS_CIRLE_GAUGE_H
#define TK_YPS_CIRLE_GAUGE_H

#include "base/widget.h"
#include "widget_node_info.h"

BEGIN_C_DECLS
/**
 * @class auto_scale_view_t
 * @parent widget_t
 * @annotation ["scriptable","design","widget"]
 * 圆形表盘光带
 * 在xml中使用"yps\_cirle\_gauge"标签创建控件。如：
 *
 * ```xml
 * <!-- ui -->
 * <auto_scale_view x="c" y="50" w="100" h="100"/>
 * ```
 *
 * 可用通过style来设置控件的显示风格，如字体的大小和颜色等等。如：
 *
 * ```xml
 * <!-- style -->
 * <auto_scale_view>
 *   <style name="default" font_size="32">
 *     <normal text_color="black" />
 *   </style>
 * </auto_scale_view>
 * ```
 */
typedef struct _auto_scale_view_t {
  widget_t widget;

  /**
   * 私有变量，不要访问
   */
  int32_t children_count;
  /**
   * 私有变量，不要访问
   */
  widget_t **childrens;

  float_t scale_ratio; 
  int32_t initial_width;
  int32_t initial_height;
  int32_t current_width;
  int32_t current_height;
  
  widget_node_info *child_info;

} auto_scale_view_t;

/**
 * @method auto_scale_view_create
 * @annotation ["constructor", "scriptable"]
 * 创建auto_scale_view对象
 * @param {widget_t*} parent 父控件
 * @param {xy_t} x x坐标
 * @param {xy_t} y y坐标
 * @param {wh_t} w 宽度
 * @param {wh_t} h 高度
 *
 * @return {widget_t*} auto_scale_view对象。
 */
widget_t *auto_scale_view_create(widget_t *parent, xy_t x, xy_t y, wh_t w,
                                 wh_t h);

/**
 * @method auto_scale_view_set_scale_ratio
 * 设置缩放比率。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget auto_scale_view对象。
 * @param {float_t} scale_ratio 缩放比率。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
void auto_scale_view_set_scale_ratio(widget_t *widget, float_t scale_ratio);

/**
 * @method auto_scale_view_set_font_scale_ratio
 * 设置字体缩放比率。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget auto_scale_view对象。
 * @param {float_t} font_scale_ratio 字体缩放比率。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
void auto_scale_view_set_font_scale_ratio(widget_t *widget,
                                           float_t font_scale_ratio);

/**
 * @method auto_scale_view_set_width_scale_ratio
 * 设置宽度缩放比率。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget auto_scale_view对象。
 * @param {float_t} width_scale_ratio 宽度缩放比率。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
void auto_scale_view_set_width_scale_ratio(widget_t *widget, float_t width_scale_ratio);

/**
 * @method auto_scale_view_set_height_scale_ratio
 * 设置高度缩放比率。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget auto_scale_view对象。
 * @param {float_t} height_scale_ratio 高度缩放比率。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
void auto_scale_view_set_height_scale_ratio(widget_t *widget, float_t height_scale_ratio);

/**
 * @method auto_scale_view_cast
 * 转换为auto_scale_view对象(供脚本语言使用)。
 * @annotation ["cast", "scriptable"]
 * @param {widget_t*} widget auto_scale_view对象。
 *
 * @return {widget_t*} auto_scale_view对象。
 */
widget_t *auto_scale_view_cast(widget_t *widget);

#define WIDGET_TYPE_AUTO_SCALE_VIEW "auto_scale_view"
#define AUTO_SCALE_VIEW_PROP_SCALE_RATIO "scale_ratio"
#define AUTO_SCALE_VIEW_PROP_FONT_SCALE_RATIO "font_scale_ratio"

#define AUTO_SCALE_VIEW(widget)                                                \
  ((auto_scale_view_t *)(auto_scale_view_cast(WIDGET(widget))))

/*public for subclass and runtime type check*/
TK_EXTERN_VTABLE(auto_scale_view);

END_C_DECLS

#endif /*TK_AUTO_SCALE_VIEW_H*/
