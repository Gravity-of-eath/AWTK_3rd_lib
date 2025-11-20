/**
 * File:   yps_cirle_gauge.h
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

BEGIN_C_DECLS
/**
 * @class yps_cirle_gauge_t
 * @parent widget_t
 * @annotation ["scriptable","design","widget"]
 * 圆形表盘光带
 * 在xml中使用"yps\_cirle\_gauge"标签创建控件。如：
 *
 * ```xml
 * <!-- ui -->
 * <yps_cirle_gauge x="c" y="50" w="100" h="100"/>
 * ```
 *
 * 可用通过style来设置控件的显示风格，如字体的大小和颜色等等。如：
 * 
 * ```xml
 * <!-- style -->
 * <yps_cirle_gauge>
 *   <style name="default" font_size="32">
 *     <normal text_color="black" />
 *   </style>
 * </yps_cirle_gauge>
 * ```
 */
typedef struct _yps_cirle_gauge_t {
  widget_t widget;
  float_t last_angle;
  int32_t value;
  rect_t dirty_rect;
  rect_t pointer_dirty_rect;

  /**
   * @property {float_t} angle
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 指针角度。6点钟方向为0°， 9点方向为180°，顺时钟方向为正，单位为度。
   */
  float_t angle;

  /**
   * @property {float_t} min_angle
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 指针可以旋转的最小角度
   */
  float_t min_angle;

  /**
   * @property {float_t} max_angle
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 指针可以旋转的最大角度
   */
  float_t max_angle;

  /**
   * @property {int32_t} min_value
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 表盘最小值
   */
  int32_t min_value;

  /**
   * @property {int32_t} max_value
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 表盘最大值
   */
  int32_t max_value;

  /**
   * @property {int32_t} r1
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 光带外环半径。。
   */
  int32_t r1;

  /**
   * @property {int32_t} r2
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 光带内环半径。。
   */
  int32_t r2;

  /**
   * @property {char*} image1
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 光带图片1
   */
  char* image1;

  /**
   * @property {char*} image2
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 光带图片2
   */
  char* image2;

  /**
   * @property {float_t} total_degree
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 光带总角度，单位为度。
   */
  float_t total_degree;

  /**
   * @property {float_t} critical
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 临界值，大于临界值使用image2。
   */
  float_t critical;

  /**
   * @property {int32_t} anchor_x
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 圆心锚点x坐标。。
   */
  int32_t anchor_x;

  /**
   * @property {int32_t} anchor_y
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 圆心锚点y坐标。。
   */
  int32_t anchor_y;

  /**
   * @property {char*} pointer_image
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 表盘指针图片
   */
  char* pointer_image;

  /**
   * @property {char*} pointer_image2
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 表盘指针图片
   */
  char* pointer_image2;

  /**
   * @property {int32_t} pointer_offset_x
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 表盘指针偏移x
   */
  int32_t pointer_offset_x;

   /**
   * @property {int32_t} pointer_offset_y
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 表盘指针偏移y
   */
  int32_t pointer_offset_y;

  
  int32_t pointer_offset_angle;

  /**
   * @property {bitmap_t*} pre_rotated_bitmaps
   * 预旋转的指针位图数组
   */
  bitmap_t** pre_rotated_bitmaps;
  
  /**
   * @property {int} num_pre_rotated_bitmaps
   * 预旋转位图数组的大小
   */
  int num_pre_rotated_bitmaps;
  
  /**
   * @property {bool_t} pre_rotated_bitmaps_loaded
   * 标记预旋转位图是否已加载
   */
  bool_t pre_rotated_bitmaps_loaded;

  point_t* positions;
 

  int32_t last_degge_index;

} yps_cirle_gauge_t;

/**
 * @method yps_cirle_gauge_create
 * @annotation ["constructor", "scriptable"]
 * 创建yps_cirle_gauge对象
 * @param {widget_t*} parent 父控件
 * @param {xy_t} x x坐标
 * @param {xy_t} y y坐标
 * @param {wh_t} w 宽度
 * @param {wh_t} h 高度
 *
 * @return {widget_t*} yps_cirle_gauge对象。
 */
widget_t* yps_cirle_gauge_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h);

/**
 * @method yps_cirle_gauge_cast
 * 转换为yps_cirle_gauge对象(供脚本语言使用)。
 * @annotation ["cast", "scriptable"]
 * @param {widget_t*} widget yps_cirle_gauge对象。
 *
 * @return {widget_t*} yps_cirle_gauge对象。
 */
widget_t* yps_cirle_gauge_cast(widget_t* widget);


/**
 * @method yps_cirle_gauge_set_angle
 * 设置 指针角度。12点钟方向为0度，顺时钟方向为正，单位为度。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {float_t} angle 指针角度。12点钟方向为0度，顺时钟方向为正，单位为度。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_cirle_gauge_set_angle(widget_t* widget, float_t angle);

/**
 * @method yps_cirle_gauge_set_min_angle
 * 设置 表盘起始角度
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {float_t} 起始角度
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_cirle_gauge_set_min_angle(widget_t* widget, float_t min_angle);

/**
 * @method yps_cirle_gauge_set_max_angle
 * 设置 表盘最大角度
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {float_t} 最大角度
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_cirle_gauge_set_max_angle(widget_t* widget, float_t max_angle);


/**
 * @method yps_cirle_gauge_set_min_value
 * 设置 最小值
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {int32_t} 最小值
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_cirle_gauge_set_min_value(widget_t* widget, int32_t min_value);

/**
 * @method yps_cirle_gauge_set_max_value
 * 设置 最大值
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {int32_t} 最大值
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_cirle_gauge_set_max_value(widget_t* widget, int32_t max_value);

/**
 * @method yps_cirle_gauge_set_r1
 * 设置 光带外环半径。。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {int32_t} r1 光带外环半径。。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_cirle_gauge_set_r1(widget_t* widget, int32_t r1);

/**
 * @method yps_cirle_gauge_set_r2
 * 设置 光带内环半径。。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {int32_t} r2 光带内环半径。。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_cirle_gauge_set_r2(widget_t* widget, int32_t r2);

/**
 * @method yps_cirle_gauge_set_image1
 * 设置 光带图片1
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {char*} image1 光带图片1
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_cirle_gauge_set_image1(widget_t* widget, const char* image1);

/**
 * @method yps_cirle_gauge_set_image2
 * 设置 光带图片2
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {char*} image2 光带图片2
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_cirle_gauge_set_image2(widget_t* widget, const char* image2);


/**
 * @method yps_cirle_gauge_set_total_degree
 * 设置 光带总角度，单位为度。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {float_t} total_degree 光带总角度，单位为度。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_cirle_gauge_set_total_degree(widget_t* widget, float_t total_degree);

/**
 * @method yps_cirle_gauge_set_critical
 * 设置 临界值，大于临界值使用image2。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {float_t} critical 临界值，大于临界值使用image2。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_cirle_gauge_set_critical(widget_t* widget, float_t critical);

/**
 * @method yps_cirle_gauge_set_anchor_x
 * 设置 圆心锚点x坐标。。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {int32_t} anchor_x 圆心锚点x坐标。。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_cirle_gauge_set_anchor_x(widget_t* widget, int32_t anchor_x);

/**
 * @method yps_cirle_gauge_set_anchor_y
 * 设置 圆心锚点y坐标。。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {int32_t} anchor_y 圆心锚点y坐标。。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_cirle_gauge_set_anchor_y(widget_t* widget, int32_t anchor_y);

/**
 * @method yps_cirle_gauge_set_pointer_image
 * 设置 指针图片
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {char*} pointer_image 指针图片
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_cirle_gauge_set_pointer_image(widget_t* widget, const char* pointer_image);

/**
 * @method yps_cirle_gauge_set_pointer_offset_x
 * 设置 指针图片
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {int32_t} pointer_offset_x 指针x偏移
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_cirle_gauge_set_pointer_offset_x(widget_t* widget, int32_t pointer_offset_x);

/**
 * @method yps_cirle_gauge_set_pointer_offset_y
 * 设置 指针图片
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {int32_t} pointer_offset_y 指针y偏移
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_cirle_gauge_set_pointer_offset_y(widget_t* widget, int32_t pointer_offset_y);


#define YPS_CIRLE_GAUGE_PROP_ANGLE "angle"
#define YPS_CIRLE_GAUGE_PROP_MIN_ANGLE "min_angle"
#define YPS_CIRLE_GAUGE_PROP_MAX_ANGLE "max_angle"
#define YPS_CIRLE_GAUGE_PROP_MIN_VALUE "min_value"
#define YPS_CIRLE_GAUGE_PROP_MAX_VALUE "max_value"
#define YPS_CIRLE_GAUGE_PROP_R1 "r1"
#define YPS_CIRLE_GAUGE_PROP_R2 "r2"
#define YPS_CIRLE_GAUGE_PROP_IMAGE1 "image1"
#define YPS_CIRLE_GAUGE_PROP_IMAGE2 "image2"
#define YPS_CIRLE_GAUGE_PROP_TOTAL_DEGREE "total_degree"
#define YPS_CIRLE_GAUGE_PROP_CRITICAL "critical"
#define YPS_CIRLE_GAUGE_PROP_ANCHOR_X "anchor_x"
#define YPS_CIRLE_GAUGE_PROP_ANCHOR_Y "anchor_y"
#define YPS_CIRLE_GAUGE_PROP_POINTER_IMAGE "pointer_image"
#define YPS_CIRLE_GAUGE_PROP_POINTER_IMAGE2 "pointer_image2"
#define YPS_CIRLE_GAUGE_PROP_POINTER_OFFSET_X "pointer_offset_x"
#define YPS_CIRLE_GAUGE_PROP_POINTER_OFFSET_Y "pointer_offset_y"
#define YPS_CIRLE_GAUGE_PROP_POINTER_OFFSET_ANGLE "pointer_offset_angle"



#define WIDGET_TYPE_YPS_CIRLE_GAUGE "yps_cirle_gauge"

#define YPS_CIRLE_GAUGE(widget) ((yps_cirle_gauge_t*)(yps_cirle_gauge_cast(WIDGET(widget))))

/*public for subclass and runtime type check*/
TK_EXTERN_VTABLE(yps_cirle_gauge);

END_C_DECLS

#endif /*TK_YPS_CIRLE_GAUGE_H*/
