
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

/* 一整圈对应的定点角单位数。角度以 2PI/65536 为单位存放在 uint16 里，
 * 分辨率 0.0055 度，远细于 360 级色表的一个色阶。 */
#define ARC_ANGLE_UNITS 65536

/* 极坐标查询表
 *
 * angle_table 存放的是"已经调整过的"定点角：建表时就把 atan2 结果归一化到
 * [0,2PI) 并减去 90 度（AWTK 的 Y 轴向下），绘制时不再需要任何浮点归一化。
 *
 * 距离表已移除：绘制改为逐行解析求 x 区间（整数判定 x*x+y*y <= R*R），
 * 不再需要逐像素的距离查询。相比 float 双表，内存降到 1/4。
 */
typedef struct _polar_lut_t {
  int32_t width;           // 表宽度
  int32_t height;          // 表高度
  int32_t center_x;        // 中心点X（相对坐标）
  int32_t center_y;        // 中心点Y（相对坐标）
  uint16_t* angle_table;   // 定点角查询表 [y][x]，单位 2PI/ARC_ANGLE_UNITS
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
    float_t full_ratio;//占满率，1时绘制整个扇形区域，0.5则绘制弧环（在扇形基础上去掉半径*0.5的圆心部分）

    /* 帧间缓存：渲染结果直接写进 RGBA8888 位图，参数没变时下一帧只贴图。
     * 直写像素同时省掉了每段游程一次的 canvas_fill_rect 调用——那个固定开销
     * 才是实测中的主要瓶颈。另外 AWTK 里任何与本控件重叠的脏矩形都会触发
     * 重绘，而仪表盘场景下绝大多数重绘的参数其实没有变化。 */
    bitmap_t* cache_bitmap;  // RGBA8888 缓存位图，尺寸与控件一致
    bool_t cache_dirty;      // 参数变更后置位，下次绘制时重新渲染
    bool_t cache_enable;     // 关掉则每帧直接画到 canvas（省内存，但更慢）

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
 * @param {float_t} start_angle 起始角度（度）
 * @param {float_t} stop_angle 结束角度（度）
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
/* 是否启用帧间缓存，默认开。关掉可省一张 w*h*4 的位图，但每帧都要重画。 */
#define CONNER_GRADIENT_VIEW_PROP_CACHE "cache"
#define CONNER_GRADIENT_VIEW_PROP_MAX "max"
#define CONNER_GRADIENT_VIEW_PROP_CURRENT "current"

/* 类型转换宏 */
#define CONNER_GRADIENT_VIEW(widget) ((conner_gradient_view_t*)(widget));




#endif//TK_CONNER_GRADIENT_VIEW_H