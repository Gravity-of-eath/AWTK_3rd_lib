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
#include "float_queue.h"

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
typedef struct _line_chart_t
{
  widget_t widget;

  /**
   * @property {char*} fg_color
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 。前景色 即折线值高于阈值（divide_value）部分的颜色
   */
  char *fg_color;

  /**
   * @property {char*} secd_color
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 。与fg_color相反 即折线值底于阈值（divide_value）部分的颜色
   */
  char *secd_color;
  
  /**
   * 引导线的颜色 即背景三条线（中间是虚线）的颜色
   */
  char *guide_line_color;

  /**
   * @property {int32_t} mode
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 。暂时无用
   */
  int32_t mode;

  /**
   * @property {int32_t} max_point
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 。表格最大缓存多少条数据（即多少个折线段）
   */
  int32_t max_point;

  /**
   * @property {float_t} divide_value
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 。阈值，fg_color与secd_color的分解比例（高度）
   */
  float_t divide_value;


  /***
   * 
   * 私有变量不要访问
   */
  FloatQueue *queue;

  /**
   * 背景参考线占总控件高度的偏移比率（即顶部和底部参考线距离控件上下边界的百分百）
   */
  float_t guide_line_offset;

  /**
   * @property {int32_t} align
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 。暂时无用
   */
  int32_t align;

  /**
   * @property {int32_t} line_width
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 。
   * 折线的宽度
   */
  int32_t line_width;
  /**
   *调试 显示控件绘制区域
   */

  bool_t DEBUG;

  /***
   * 
   * fg_color在X 方向上可用的比例（用于动画）
   */
  float_t secd_percent;

  /***
   * 背景参考线占总控件宽度的比率（用于动画）
   * 
   */

  float_t guide_line_percent;

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
widget_t *line_chart_create(widget_t *parent, xy_t x, xy_t y, wh_t w, wh_t h);

/**
 * @method line_chart_cast
 * 转换为line_chart对象(供脚本语言使用)。
 * @annotation ["cast", "scriptable"]
 * @param {widget_t*} widget line_chart对象。
 *
 * @return {widget_t*} line_chart对象。
 */
widget_t *line_chart_cast(widget_t *widget);

/**
 * @method line_chart_set_fg_color
 * 设置 。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {const char*} fg_color 。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t line_chart_set_fg_color(widget_t *widget, const char *fg_color);

/**
 * @method line_chart_set_secd_color
 * 设置 。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {const char*} secd_color 。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t line_chart_set_secd_color(widget_t *widget, const char *secd_color);
ret_t line_chart_set_guide_line_color(widget_t *widget, const char *guide_line_color);

/**
 * @method line_chart_set_mode
 * 设置 。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {int32_t} mode 。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t line_chart_set_mode(widget_t *widget, int32_t mode);

/**
 * @method line_chart_set_max_point
 * 设置 。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {int32_t} max_point 。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t line_chart_set_max_point(widget_t *widget, int32_t max_point);

/**
 * @method line_chart_set_divide_value
 * 设置 。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {float_t} divide_value 。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t line_chart_set_divide_value(widget_t *widget, float_t divide_value);

/**
 * @method line_chart_set_align
 * 设置 。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {int32_t} align 。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t line_chart_set_align(widget_t *widget, int32_t align);

ret_t line_chart_set_line_width(widget_t *widget, int32_t line_width);

ret_t line_chart_add_point(widget_t *widget, float_t point);

#define LINE_CHART_PROP_FG_COLOR "fg_color"
#define LINE_CHART_PROP_SECD_COLOR "secd_color"
#define LINE_CHART_PROP_GUIDE_LINE_COLOR "guide_line_color"
#define LINE_CHART_PROP_MODE "mode"
#define LINE_CHART_PROP_MAX_POINT "max_point"
#define LINE_CHART_PROP_DIVIDE_VALUE "divide_value"
#define LINE_CHART_PROP_ALIGN "align"
#define LINE_CHART_PROP_LINE_WIDTH "line_width"
#define LINE_CHART_PROP_GUIDE_LINE_OFFSET "guide_line_offset"
#define LINE_CHART_PROP_SECD_PERCENT "secd_percent"
#define LINE_CHART_PROP_GUIDE_LINE_PERCENT "guide_line_percent"
#define LINE_CHART_PROP_DEBUG "debug"

#define WIDGET_TYPE_LINE_CHART "line_chart"

#define LINE_CHART(widget) ((line_chart_t *)(line_chart_cast(WIDGET(widget))))

/*public for subclass and runtime type check*/
TK_EXTERN_VTABLE(line_chart);

END_C_DECLS

#endif /*TK_LINE_CHART_H*/
