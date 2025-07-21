/**
 * File:   line_chart.h
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


#ifndef TK_LINE_CHART_H
#define TK_LINE_CHART_H

#include "base/widget.h"

BEGIN_C_DECLS
/**
 * @class line_chart_t
 * @parent widget_t
 * @annotation ["scriptable","design","widget"]
 * 
 * 在xml中使用"line\_chart"标签创建控件。如：
 *
 * ```xml
 * <!-- ui -->
 * <line_chart x="c" y="50" w="100" h="100"/>
 * ```
 *
 * 可用通过style来设置控件的显示风格，如字体的大小和颜色等等。如：
 * 
 * ```xml
 * <!-- style -->
 * <line_chart>
 *   <style name="default" font_size="32">
 *     <normal text_color="black" />
 *   </style>
 * </line_chart>
 * ```
 */
typedef struct _line_chart_t {
  widget_t widget;


  /**
   * @property {char*} fg_color
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 。
   */
  char* fg_color;

  /**
   * @property {char*} secd_color
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 。
   */
  char* secd_color;

  /**
   * @property {int32_t} mode
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 。
   */
  int32_t mode;

  /**
   * @property {int32_t} max_point
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 。
   */
  int32_t max_point;

  /**
   * @property {float_t} divide_value
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 。
   */
  float_t divide_value;

  /**
   * @property {int32_t} align
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 。
   */
  int32_t align;

} line_chart_t;

/**
 * @method line_chart_create
 * @annotation ["constructor", "scriptable"]
 * 创建line_chart对象
 * @param {widget_t*} parent 父控件
 * @param {xy_t} x x坐标
 * @param {xy_t} y y坐标
 * @param {wh_t} w 宽度
 * @param {wh_t} h 高度
 *
 * @return {widget_t*} line_chart对象。
 */
widget_t* line_chart_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h);

/**
 * @method line_chart_cast
 * 转换为line_chart对象(供脚本语言使用)。
 * @annotation ["cast", "scriptable"]
 * @param {widget_t*} widget line_chart对象。
 *
 * @return {widget_t*} line_chart对象。
 */
widget_t* line_chart_cast(widget_t* widget);


/**
 * @method line_chart_set_fg_color
 * 设置 。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {const char*} fg_color 。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t line_chart_set_fg_color(widget_t* widget, const char* fg_color);

/**
 * @method line_chart_set_secd_color
 * 设置 。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {const char*} secd_color 。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t line_chart_set_secd_color(widget_t* widget, const char* secd_color);

/**
 * @method line_chart_set_mode
 * 设置 。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {int32_t} mode 。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t line_chart_set_mode(widget_t* widget, int32_t mode);

/**
 * @method line_chart_set_max_point
 * 设置 。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {int32_t} max_point 。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t line_chart_set_max_point(widget_t* widget, int32_t max_point);

/**
 * @method line_chart_set_divide_value
 * 设置 。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {float_t} divide_value 。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t line_chart_set_divide_value(widget_t* widget, float_t divide_value);

/**
 * @method line_chart_set_align
 * 设置 。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {int32_t} align 。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t line_chart_set_align(widget_t* widget, int32_t align);


#define LINE_CHART_PROP_FG_COLOR "fg_color"
#define LINE_CHART_PROP_SECD_COLOR "secd_color"
#define LINE_CHART_PROP_MODE "mode"
#define LINE_CHART_PROP_MAX_POINT "max_point"
#define LINE_CHART_PROP_DIVIDE_VALUE "divide_value"
#define LINE_CHART_PROP_ALIGN "align"

#define WIDGET_TYPE_LINE_CHART "line_chart"

#define LINE_CHART(widget) ((line_chart_t*)(line_chart_cast(WIDGET(widget))))

/*public for subclass and runtime type check*/
TK_EXTERN_VTABLE(line_chart);

END_C_DECLS

#endif /*TK_LINE_CHART_H*/
