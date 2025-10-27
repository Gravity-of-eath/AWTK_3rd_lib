
#ifndef TK_CONNER_GRADIENT_VIEW_H
#define TK_CONNER_GRADIENT_VIEW_H

#include "base/widget.h"
#include "base/window.h"
#include "tkc/utils.h"
#include "tkc/color.h"
#include "tkc/mem.h"
#include <math.h>

/* 多颜色渐变结构 */
typedef struct _color_point_t {
  float position;  /* 位置 0.0-1.0 */
  color_t color;   /* 颜色 */
} color_point_t;

/* 极坐标查询表 */
typedef struct _polar_lut_t {
  int32_t width;          // 表宽度
  int32_t height;         // 表高度  
  int32_t center_x;       // 中心点X（相对坐标）
  int32_t center_y;       // 中心点Y（相对坐标）
  float* angle_table;     // 角度查询表 [y][x]
  float* distance_table;  // 距离查询表 [y][x]
} polar_lut_t;


/* 弧形渐变渲染器 */
typedef struct _arc_gradient_renderer_t {
  polar_lut_t* lut;           // 极坐标查询表
  color_t* color_table;       // 颜色查找表 [angle_index]
  uint32_t color_table_size;  // 颜色表大小（通常360）
  bool_t cache_valid;         // 缓存是否有效
} arc_gradient_renderer_t;

typedef struct  _conner_gradient_view_t
{
    widget_t widget;
    /* data */
    arc_gradient_renderer_t arc_gradient_renderer;
    float_t start_angle;
    float_t stop_angle;
    bool_t ant_clock;
    int32_t max;
    int32_t current;
    color_t start_color;
    color_t stop_color;


}conner_gradient_view_t;

/**
 * @method conner_gradient_view_create
 * 创建conner_gradient_view对象
 * @param {widget_t*} parent 父控件
 * @param {xy_t} x x坐标
 * @param {xy_t} y y坐标
 * @param {wh_t} w 宽度
 * @param {wh_t} h 高度
 * @return {widget_t*} 对象。
 */
widget_t* conner_gradient_view_create(widget_t *parent, xy_t x, xy_t y, wh_t w, wh_t h);

/**
 * @method conner_gradient_view_cast
 * 转换为conner_gradient_view对象(供脚本语言使用)。
 * @param {widget_t*} widget conner_gradient_view对象。
 * @return {widget_t*} conner_gradient_view对象。
 */
widget_t* conner_gradient_view_cast(widget_t* widget);

/**
 * @method conner_gradient_view_set_angles
 * 设置角度范围
 * @param {widget_t*} widget 控件对象
 * @param {float_t} start_angle 起始角度（弧度）
 * @param {float_t} stop_angle 结束角度（弧度）
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t conner_gradient_view_set_angles(widget_t* widget, float_t start_angle, float_t stop_angle);

/**
 * @method conner_gradient_view_set_direction
 * 设置绘制方向
 * @param {widget_t*} widget 控件对象
 * @param {bool_t} ant_clock 是否逆时针
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t conner_gradient_view_set_direction(widget_t* widget, bool_t ant_clock);

/**
 * @method conner_gradient_view_set_range
 * 设置数值范围
 * @param {widget_t*} widget 控件对象
 * @param {int32_t} max 最大值
 * @param {int32_t} current 当前值
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t conner_gradient_view_set_max(widget_t* widget, int32_t max );

/**
 * @method conner_gradient_view_set_current
 * 设置当前值
 * @param {widget_t*} widget 控件对象
 * @param {int32_t} current 当前值
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t conner_gradient_view_set_current(widget_t* widget, int32_t current);

/* 属性定义 */
#define WIDGET_TYPE_CONNER_GRADIENT_VIEW "conner_gradient_view"
#define CONNER_GRADIENT_VIEW_PROP_START_ANGLE "start_angle"
#define CONNER_GRADIENT_VIEW_PROP_STOP_ANGLE "stop_angle"
#define CONNER_GRADIENT_VIEW_PROP_ANT_CLOCK "ant_clock"
#define CONNER_GRADIENT_VIEW_PROP_START_COLOR "start_color"
#define CONNER_GRADIENT_VIEW_PROP_STOP_COLOR "stop_color"
#define CONNER_GRADIENT_VIEW_PROP_MAX "max"
#define CONNER_GRADIENT_VIEW_PROP_CURRENT "current"

/* 类型转换宏 */
#define CONNER_GRADIENT_VIEW(widget) ((conner_gradient_view_t*)(widget));




#endif//TK_CONNER_GRADIENT_VIEW_H