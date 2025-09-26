/**
 * File:   yps_banner_menu.h
 * Author: yps
 * Brief:  yps_banner_menu
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
 * 2025-6-3 yk created
 *
 */

#ifndef TK_YPS_BANNER_MENU_H
#define TK_YPS_BANNER_MENU_H

#include "base/widget.h"

BEGIN_C_DECLS
/**
 * @class yps_banner_menu_t
 * @parent widget_t
 * @annotation ["scriptable","design","widget"]
 * yps_banner_menu
 * 在xml中使用"yps\_banner\_menu"标签创建控件。如：
 *
 * ```xml
 * <!-- ui -->
 * <yps_banner_menu x="c" y="50" w="100" h="100"/>
 * ```
 *
 * 可用通过style来设置控件的显示风格，如字体的大小和颜色等等。如：
 *
 * ```xml
 * <!-- style -->
 * <yps_banner_menu>
 *   <style name="default" font_size="32">
 *     <normal text_color="black" />
 *   </style>
 * </yps_banner_menu>
 * ```
 */
#define FPS 30

typedef struct _yps_banner_menu_t yps_banner_menu_t;

typedef struct _layout_manager
{
  void (*on_layout)(yps_banner_menu_t *parent, rect_t *reference_position, widget_t **childrens, int32_t count, int32_t focused, int32_t focus_lossed);
  void (*on_scroll)(yps_banner_menu_t *parent, rect_t *reference_position, widget_t **childrens, int32_t count, widget_t *focus_lossing, widget_t *focus_next, float_t progress);
} layout_manager;

typedef struct _yps_banner_menu_t
{
  widget_t widget;

  /**
   * @property {int32_t} animtor_duration
   * @annotation ["set_prop","get_prop","readable","persitent","design","scriptable"]
   * 。私有变量，不要访问，会在layout_manager.on_layout和on_scroll中回调，代表参考位置和大小（即第一个控件在yps_banner_menu容器中的默认值<XML中的值>）
   */
  rect_t *reference_position;

  /**
   * 动画时长（通过yps_banner_menu_focus_next/yps_banner_menu_focus_perv操作时有用），
   * 直接yps_banner_menu_set_focus_index无效
   */
  int32_t animtor_duration;

  /**
   * 私有变量，不要访问
   */
  int32_t children_count;

  /**
   * 私有变量，不要访问
   */
  int32_t focus_index;
  /**
   * 私有变量，不要访问
   */
  int32_t target_index;
  /**
   * 私有变量，不要访问
   */
  bool_t on_animation;
  /**
   * 私有变量，不要访问
   */
  bool_t next_or_prev;
  /**
   * 私有变量，不要访问
   */
  widget_t **childrens;

  /**
   * 布局管理器，可以自己实现其中的两个回调函数来更改默认的布局逻辑
   */
  layout_manager *layout_manager;
  /**
   * 私有变量，不要访问
   */
  int32_t step_progress;
  /**
   * 私有变量，不要访问
   */
  int32_t current_progress;
  /**
   * 私有变量，不要访问
   */
  float_t progress;

  // 最小缩放比率(小于1)，即控件失去焦点时缩小动画最小缩小到<scale_ratio>倍于原大小（参考控件，第一个控件）时动画结束并隐藏控件
  float_t scale_ratio;

  /**
   * 刷新控件
   */
  int32_t refresh;

} yps_banner_menu_t;

/**
 * @method yps_banner_menu_create
 * @annotation ["constructor", "scriptable"]
 * 创建yps_banner_menu对象
 * @param {widget_t*} parent 父控件
 * @param {xy_t} x x坐标
 * @param {xy_t} y y坐标
 * @param {wh_t} w 宽度
 * @param {wh_t} h 高度
 *
 * @return {widget_t*} yps_banner_menu对象。
 */
widget_t *yps_banner_menu_create(widget_t *parent, xy_t x, xy_t y, wh_t w, wh_t h);

/**
 * @method yps_banner_menu_cast
 * 转换为yps_banner_menu对象(供脚本语言使用)。
 * @annotation ["cast", "scriptable"]
 * @param {widget_t*} widget yps_banner_menu对象。
 *
 * @return {widget_t*} yps_banner_menu对象。
 */
widget_t *yps_banner_menu_cast(widget_t *widget);

/**
 * @method yps_banner_menu_set_animtor_duration
 * 设置 。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {int32_t} animtor_duration 。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_banner_menu_set_animtor_duration(widget_t *widget, int32_t animtor_duration);

ret_t yps_banner_menu_set_scale_ratio(widget_t *widget, float_t scale_ratio);

ret_t yps_banner_menu_set_focus_index(widget_t *widget, float_t focus_index);

int32_t yps_banner_menu_get_focus_index(widget_t *widget);

ret_t yps_banner_menu_set_layout_manager(widget_t *widget, layout_manager *layout_manager);

ret_t yps_banner_menu_focus_next(widget_t *widget);

ret_t yps_banner_menu_focus_prev(widget_t *widget);

#define YPS_BANNER_MENU_PROP_ANIMTOR_DURATION "animtor_duration"
#define YPS_BANNER_MENU_PROP_SCALE_RATIO "scale_ratio"
#define YPS_BANNER_MENU_PROP_FOCUS_INDEX "focus_index"
#define YPS_BANNER_MENU_PROP_REFRESH "refresh"

#define WIDGET_TYPE_YPS_BANNER_MENU "banner_menu"

#define YPS_BANNER_MENU(widget) ((yps_banner_menu_t *)(yps_banner_menu_cast(WIDGET(widget))))

/*public for subclass and runtime type check*/
TK_EXTERN_VTABLE(yps_banner_menu);

void scale_widget_group(widget_t *group, rect_t *scale);

END_C_DECLS

#endif /*TK_YPS_BANNER_MENU_H*/
