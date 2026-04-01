#ifndef TK_BREATH_ELLIPSE_VIEW_H
#define TK_BREATH_ELLIPSE_VIEW_H

#include "base/widget.h"
#include "tkc/color.h"

BEGIN_C_DECLS

typedef struct _breath_ellipse_view_t {
  widget_t widget;
  /* 椭圆中心颜色配置字符串（支持单色或颜色序列：#RRGGBB,#RRGGBB,...） */
  char* center_color;
  color_t* center_colors;
  uint32_t center_color_count;
  /* 呼吸频率：每分钟循环次数（次/分钟） */
  float_t frequency_bpm;
  /* 最小缩放比例（呼吸收缩到的下限） */
  float_t min_scale;
  /* 最大缩放比例（呼吸扩张到的上限） */
  float_t max_scale;
  /* 单个呼吸周期时长（毫秒）；0 表示根据 frequency_bpm 自动换算 */
  uint32_t duration_ms;
  /* 帧更新间隔（毫秒）；值越小越平滑但刷新更频繁 */
  uint32_t frame_interval_ms;
  /* 当前实时缩放比例（动画驱动更新） */
  float_t current_scale;
  float_t current_phase;
  uint64_t current_cycle;
  /* 内部定时器 ID（用于驱动动画与停止时释放） */
  uint32_t timer_id;
  /* 动画运行状态：TRUE 运行中，FALSE 已停止 */
  bool_t running;
  /* 动画暂停状态：TRUE 暂停中，FALSE 正常播放 */
  bool_t paused;
  /* 动画启动时间戳（毫秒） */
  uint64_t start_time_ms;
  /* 最近一次暂停时间戳（毫秒） */
  uint64_t pause_time_ms;
  /* 累计暂停时长（毫秒），用于恢复后保持相位连续 */
  uint64_t paused_total_ms;
  /* FPS 统计窗口内累计帧数 */
  uint32_t frame_count;
  /* FPS 统计窗口起始时间戳（毫秒） */
  uint64_t fps_sample_start_ms;
  /* 当前估算帧率（仅用于监控，不影响渲染） */
  float_t fps;
} breath_ellipse_view_t;

widget_t* breath_ellipse_view_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h);
widget_t* breath_ellipse_view_cast(widget_t* widget);

/* 设置中心颜色 */
ret_t breath_ellipse_view_set_center_color(widget_t* widget, const char* center_color);
/* 设置呼吸频率（次/分钟） */
ret_t breath_ellipse_view_set_frequency_bpm(widget_t* widget, float_t frequency_bpm);
/* 同时设置最小/最大缩放比例 */
ret_t breath_ellipse_view_set_scale_range(widget_t* widget, float_t min_scale, float_t max_scale);
/* 设置最小缩放比例 */
ret_t breath_ellipse_view_set_min_scale(widget_t* widget, float_t min_scale);
/* 设置最大缩放比例 */
ret_t breath_ellipse_view_set_max_scale(widget_t* widget, float_t max_scale);
/* 设置周期时长（ms），0 表示自动按 frequency_bpm 计算 */
ret_t breath_ellipse_view_set_duration_ms(widget_t* widget, uint32_t duration_ms);
/* 设置帧更新间隔（ms） */
ret_t breath_ellipse_view_set_frame_interval_ms(widget_t* widget, uint32_t frame_interval_ms);

/* 启动动画 */
ret_t breath_ellipse_view_start(widget_t* widget);
/* 暂停动画 */
ret_t breath_ellipse_view_pause(widget_t* widget);
/* 恢复动画 */
ret_t breath_ellipse_view_resume(widget_t* widget);
/* 停止动画并复位到最小缩放 */
ret_t breath_ellipse_view_stop(widget_t* widget);

/* 获取当前缩放值 */
float_t breath_ellipse_view_get_current_scale(widget_t* widget);
/* 获取当前 FPS 估算值 */
float_t breath_ellipse_view_get_fps(widget_t* widget);
/* 查询是否运行中 */
bool_t breath_ellipse_view_is_running(widget_t* widget);
/* 查询是否暂停中 */
bool_t breath_ellipse_view_is_paused(widget_t* widget);

float_t breath_ellipse_view_eval_scale(float_t min_scale, float_t max_scale, float_t phase);
color_t breath_ellipse_view_eval_color(const color_t* colors, uint32_t color_count, float_t phase);
color_t breath_ellipse_view_eval_color_near_min(const color_t* colors, uint32_t color_count,
                                                uint64_t cycle, float_t phase, float_t window);
uint32_t breath_ellipse_view_effective_duration_ms(float_t frequency_bpm, uint32_t duration_ms);
float_t breath_ellipse_view_calc_fps(uint32_t frame_count, uint32_t elapsed_ms);

/* XML 属性名：中心颜色 */
#define BREATH_ELLIPSE_VIEW_PROP_CENTER_COLOR "center_color"
/* XML 属性名：中心颜色序列（兼容别名） */
#define BREATH_ELLIPSE_VIEW_PROP_CENTER_COLORS "center_colors"
/* XML 属性名：呼吸频率（次/分钟） */
#define BREATH_ELLIPSE_VIEW_PROP_FREQUENCY_BPM "frequency_bpm"
/* XML 属性名：最小缩放比例 */
#define BREATH_ELLIPSE_VIEW_PROP_MIN_SCALE "min_scale"
/* XML 属性名：最大缩放比例 */
#define BREATH_ELLIPSE_VIEW_PROP_MAX_SCALE "max_scale"
/* XML 属性名：周期时长（ms），0=自动换算 */
#define BREATH_ELLIPSE_VIEW_PROP_DURATION_MS "duration_ms"
/* XML 属性名：帧更新间隔（ms） */
#define BREATH_ELLIPSE_VIEW_PROP_FRAME_INTERVAL_MS "frame_interval_ms"

#define WIDGET_TYPE_BREATH_ELLIPSE_VIEW "breath_ellipse_view"

#define BREATH_ELLIPSE_VIEW(widget) \
  ((breath_ellipse_view_t*)(breath_ellipse_view_cast(WIDGET(widget))))

TK_EXTERN_VTABLE(breath_ellipse_view);

END_C_DECLS

#endif
