/**
 * File:   shadow_label.h
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
 * 2025-2-21  created
 *
 */


#ifndef TK_SHADOW_LABEL_H
#define TK_SHADOW_LABEL_H

#include "base/widget.h"
#include "widgets/label.h"

BEGIN_C_DECLS
/**
 * @class shadow_label_t
 * @parent widget_t
 * @annotation ["scriptable","design","widget"]
 * 
 * 在xml中使用"shadow\_label"标签创建控件。如：
 *
 * ```xml
 * <!-- ui -->
 * <shadow_label x="c" y="50" w="100" h="100"/>
 * ```
 *
 * 可用通过style来设置控件的显示风格，如字体的大小和颜色等等。如：
 * 
 * ```xml
 * <!-- style -->
 * <shadow_label>
 *   <style name="default" font_size="32">
 *     <normal text_color="black" />
 *   </style>
 * </shadow_label>
 * ```
 */
typedef struct _shadow_label_t {
  label_t widget;


  
  /**
   * @property {int32_t} length
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 显示字符的个数(小余0时全部显示)。
   * 主要用于动态改变显示字符的个数，来实现类似[拨号中...]的动画效果。
   */
  // int32_t length;

  /**
   * @property {bool_t} line_wrap
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 是否自动换行(默认FALSE)。
   */
  bool_t line_wrap;

  /**
   * @property {bool_t} word_wrap
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 是否允许整个单词换行(默认FALSE)。
   * > 需要开启自动换行才有效果
   */
  bool_t word_wrap;

  /**
   * @property {int32_t} max_w
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 当auto_adjust_size为TRUE时，用于控制控件的最大宽度，超出该宽度后才自动换行。
   * >为0表示忽略该参数。小于0时取父控件宽度加上max_w。
   */
  // int32_t max_w;

  /**
   * @property {int32_t} shadow_offset
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 。
   */
  int32_t shadow_offset;

  /**
   * @property {char*} shadow_color
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 。
   */
  char* shadow_color;


  /**
   * @property {float_t} exponent
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 。
   */
  float_t exponent;

  //绘制文字的具体区域考虑文字换行
  point_t text_lt;
  point_t text_rb;
  bitmap_t* bitmap_copy;
  canvas_t* c;
  bool_t mirror_enable;
  float_t mirror_offset_ratio;

  int32_t mirror_hight;
  bool_t debug_enable;

} shadow_label_t;

/**
 * @method shadow_label_create
 * @annotation ["constructor", "scriptable"]
 * 创建shadow_label对象
 * @param {widget_t*} parent 父控件
 * @param {xy_t} x x坐标
 * @param {xy_t} y y坐标
 * @param {wh_t} w 宽度
 * @param {wh_t} h 高度
 *
 * @return {widget_t*} shadow_label对象。
 */
widget_t* shadow_label_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h);

/**
 * @method shadow_label_cast
 * 转换为shadow_label对象(供脚本语言使用)。
 * @annotation ["cast", "scriptable"]
 * @param {widget_t*} widget shadow_label对象。
 *
 * @return {widget_t*} shadow_label对象。
 */
widget_t* shadow_label_cast(widget_t* widget);


/**
 * @method exponent
 * 设置 。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {int32_t} exponent 。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t shadow_label_set_exponent(widget_t* widget, float_t exponent);


/**
 * @method shadow_label_set_shadow_offset
 * 设置 。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {int32_t} shadow_offset 。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t shadow_label_set_shadow_offset(widget_t* widget, int32_t shadow_offset);


/**
 * @method shadow_label_set_shadow_color
 * 设置 。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {const char*} shadow_color 。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t shadow_label_set_shadow_color(widget_t* widget, const char* shadow_color);


#define SHADOW_LABEL_PROP_SHADOW_OFFSET "shadow_offset"
#define SHADOW_LABEL_PROP_EXPONENT "exponent"
#define SHADOW_LABEL_PROP_SHADOW_COLOR "shadow_color"

#define WIDGET_TYPE_SHADOW_LABEL "shadow_label"
#define SHADOW_LABEL_PROP_MIRROR_ENABLE "mirror_enable"
#define SHADOW_LABEL_PROP_MIRROR_OFFSET_RATIO "mirror_offset_ratio"
#define SHADOW_LABEL_PROP_MIRROR_HIGHT "mirror_hight"
#define SHADOW_LABEL_PROP_DEBUG_ENABLE "debug_enable"



#define SHADOW_LABEL(widget) ((shadow_label_t*)(shadow_label_cast(WIDGET(widget))))

/*public for subclass and runtime type check*/
TK_EXTERN_VTABLE(shadow_label);

END_C_DECLS

#endif /*TK_SHADOW_LABEL_H*/
