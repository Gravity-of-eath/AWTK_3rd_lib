#include "conner_gradient_view.h"
#include "tkc/color_parser.h"
#include "tkc/utils.h"
#include <math.h>
#include <stdio.h>

/* 获取指定相对坐标的角度 */
static inline float polar_lut_get_angle(const polar_lut_t *lut, int32_t rel_x, int32_t rel_y)
{
    // 计算绝对坐标（相对于查询表的原点）
    int32_t abs_x = rel_x + lut->center_x;
    int32_t abs_y = rel_y + lut->center_y;

    // 边界检查
    if (abs_x < 0 || abs_x >= lut->width || abs_y < 0 || abs_y >= lut->height)
    {
        // 越界时回退到实时计算
        return atan2f((float)rel_y, (float)rel_x);
    }

    // 从查询表中获取预计算的角度值
    uint32_t index = abs_y * lut->width + abs_x;
    return lut->angle_table[index];
}

/* 获取指定相对坐标的距离 */
static inline float polar_lut_get_distance(const polar_lut_t *lut, int32_t rel_x, int32_t rel_y)
{
    int32_t abs_x = rel_x + lut->center_x;
    int32_t abs_y = rel_y + lut->center_y;

    if (abs_x < 0 || abs_x >= lut->width || abs_y < 0 || abs_y >= lut->height)
    {
        // 越界时回退到实时计算
        return sqrtf(rel_x * rel_x + rel_y * rel_y);
    }

    uint32_t index = abs_y * lut->width + abs_x;
    return lut->distance_table[index];
}

#ifdef __ARM_NEON__
#include <arm_neon.h>

/* NEON优化的弧形渐变绘制 */
static ret_t draw_arc_gradient_neon(canvas_t *c, int32_t cx, int32_t cy,
                                    int32_t radius, float start_angle, float end_angle,
                                    bool_t ant_clock, arc_gradient_renderer_t *renderer,
                                    float full_ratio)
{
    return_value_if_fail(c != NULL && renderer != NULL && renderer->lut != NULL, RET_BAD_PARAMS);
    
    polar_lut_t *lut = renderer->lut;
    
    // 确保角度在合理范围内
    while (start_angle < 0) start_angle += 2 * M_PI;
    while (end_angle < 0) end_angle += 2 * M_PI;
    while (start_angle >= 2 * M_PI) start_angle -= 2 * M_PI;
    while (end_angle >= 2 * M_PI) end_angle -= 2 * M_PI;
    
    // 处理逆时针情况
    float angle_range = 0;
    if (ant_clock) {
        if (start_angle <= end_angle) {
            angle_range = (2 * M_PI - end_angle) + start_angle;
        } else {
            angle_range = start_angle - end_angle;
        }
    } else {
        if (end_angle <= start_angle) {
            angle_range = (2 * M_PI - start_angle) + end_angle;
        } else {
            angle_range = end_angle - start_angle;
        }
    }
    
    // 计算内圆半径（用于绘制弧环）
    int32_t inner_radius = (int32_t)(radius * (1.0f - full_ratio));
    float32x4_t inner_radius_vec = vdupq_n_f32((float)inner_radius);
    float32x4_t radius_vec = vdupq_n_f32((float)radius);
    
    // 使用NEON优化处理4个像素
    for (int32_t rel_y = -radius; rel_y <= radius; rel_y++)
    {
        for (int32_t rel_x = -radius; rel_x <= radius; rel_x += 4)
        {
            // 创建4个x坐标
            int32x4_t x_vec = {rel_x, rel_x+1, rel_x+2, rel_x+3};
            int32x4_t y_vec = {rel_y, rel_y, rel_y, rel_y};
            
            // 计算绝对坐标
            int32x4_t abs_x_vec = vaddq_s32(x_vec, vdupq_n_s32(lut->center_x));
            int32x4_t abs_y_vec = vaddq_s32(y_vec, vdupq_n_s32(lut->center_y));
            
            // 边界检查
            uint32x4_t x_valid = vandq_u32(
                vcgeq_s32(abs_x_vec, vdupq_n_s32(0)),
                vcltq_s32(abs_x_vec, vdupq_n_s32(lut->width))
            );
            uint32x4_t y_valid = vandq_u32(
                vcgeq_s32(abs_y_vec, vdupq_n_s32(0)),
                vcltq_s32(abs_y_vec, vdupq_n_s32(lut->height))
            );
            uint32x4_t valid_mask = vandq_u32(x_valid, y_valid);
            
            // 计算索引
            uint32x4_t width_vec = vdupq_n_u32(lut->width);
            uint32x4_t index_vec = vmlaq_u32(
                vmulq_u32(vcvtq_u32_s32(abs_y_vec), width_vec),
                vcvtq_u32_s32(abs_x_vec),
                vdupq_n_u32(1)
            );
            
            // 获取距离和角度
            float32x4_t distance_vec, angle_vec;
            for (int i = 0; i < 4; i++) {
                if (vgetq_lane_u32(valid_mask, i)) {
                    uint32_t idx = vgetq_lane_u32(index_vec, i);
                    ((float*)&distance_vec)[i] = lut->distance_table[idx];
                    ((float*)&angle_vec)[i] = lut->angle_table[idx];
                } else {
                    // 越界时回退到实时计算
                    float x = (float)vgetq_lane_s32(x_vec, i);
                    float y = (float)vgetq_lane_s32(y_vec, i);
                    ((float*)&distance_vec)[i] = sqrtf(x * x + y * y);
                    ((float*)&angle_vec)[i] = atan2f(y, x);
                }
            }
            
            // 检查是否在圆环范围内
            uint32x4_t in_outer_circle = vcleq_f32(distance_vec, radius_vec);
            uint32x4_t in_inner_circle = vcgeq_f32(distance_vec, inner_radius_vec);
            uint32x4_t in_ring = vandq_u32(in_outer_circle, in_inner_circle);
            
            // 处理每个像素
            for (int i = 0; i < 4; i++) {
                if (vgetq_lane_u32(in_ring, i) && 
                    (rel_x + i) <= radius) {
                    float distance = vgetq_lane_f32(distance_vec, i);
                    float angle = vgetq_lane_f32(angle_vec, i);
                    
                    // 将角度归一化到 [0, 2π)
                    if (angle < 0)
                        angle += 2 * M_PI;
                    
                    // 注意：AWTK坐标系统中Y轴向下，所以角度需要调整
                    // 将角度转换为符合AWTK坐标系统的角度
                    // atan2返回的角度是以X轴正方向为0度，逆时针为正
                    // 在AWTK中，我们希望0度在正上方（Y轴负方向）
                    float adjusted_angle = angle - M_PI_2; // 减去90度，使0度指向正上方
                    if (adjusted_angle < 0) adjusted_angle += 2 * M_PI;
                    
                    // 处理角度范围检查
                    bool_t in_arc = FALSE;
                    if (ant_clock) {
                        // 逆时针方向
                        if (start_angle <= end_angle) {
                            in_arc = (adjusted_angle >= start_angle && adjusted_angle <= end_angle);
                        } else {
                            in_arc = (adjusted_angle >= start_angle || adjusted_angle <= end_angle);
                        }
                    } else {
                        // 顺时针方向
                        if (start_angle <= end_angle) {
                            in_arc = (adjusted_angle >= start_angle && adjusted_angle <= end_angle);
                        } else {
                            in_arc = (adjusted_angle >= start_angle || adjusted_angle <= end_angle);
                        }
                    }

                    if (in_arc) {
                        // 计算在弧形中的位置比例
                        float position_ratio = 0;
                        if (ant_clock) {
                            if (start_angle <= end_angle) {
                                if (adjusted_angle >= start_angle && adjusted_angle <= end_angle) {
                                    position_ratio = (adjusted_angle - start_angle) / angle_range;
                                }
                            } else {
                                if (adjusted_angle >= start_angle) {
                                    position_ratio = (adjusted_angle - start_angle) / angle_range;
                                } else if (adjusted_angle <= end_angle) {
                                    position_ratio = ((2 * M_PI - start_angle) + adjusted_angle) / angle_range;
                                }
                            }
                        } else {
                            if (start_angle <= end_angle) {
                                if (adjusted_angle >= start_angle && adjusted_angle <= end_angle) {
                                    position_ratio = (adjusted_angle - start_angle) / angle_range;
                                }
                            } else {
                                if (adjusted_angle >= start_angle) {
                                    position_ratio = (adjusted_angle - start_angle) / angle_range;
                                } else if (adjusted_angle <= end_angle) {
                                    position_ratio = ((2 * M_PI - start_angle) + adjusted_angle) / angle_range;
                                }
                            }
                        }
                        
                        // 确保比例在[0,1]范围内
                        if (position_ratio < 0.0f) position_ratio = 0.0f;
                        if (position_ratio > 1.0f) position_ratio = 1.0f;
                        
                        // 查找预计算的颜色
                        uint32_t angle_index = (uint32_t)(position_ratio * (renderer->color_table_size - 1));
                        color_t pixel_color = renderer->color_table[angle_index];

                        // 绘制像素
                        canvas_set_fill_color(c, pixel_color);
                        canvas_fill_rect(c, cx + rel_x + i, cy + rel_y, 1, 1);
                    }
                }
            }
        }
    }

    return RET_OK;
}
#endif // __ARM_NEON__

/* 颜色插值 */
static color_t color_interpolate(color_t c1, color_t c2, float ratio)
{
    color_t result;
    // 确保ratio在[0,1]范围内
    ratio = tk_clamp(ratio, 0.0f, 1.0f);
    
    result.rgba.r = (uint8_t)(c1.rgba.r + (float)(c2.rgba.r - c1.rgba.r) * ratio);
    result.rgba.g = (uint8_t)(c1.rgba.g + (float)(c2.rgba.g - c1.rgba.g) * ratio);
    result.rgba.b = (uint8_t)(c1.rgba.b + (float)(c2.rgba.b - c1.rgba.b) * ratio);
    result.rgba.a = (uint8_t)(c1.rgba.a + (float)(c2.rgba.a - c1.rgba.a) * ratio);
    return result;
}

static polar_lut_t *polar_lut_create(int32_t max_width, int32_t max_height)
{
    polar_lut_t *lut = (polar_lut_t *)TKMEM_ZALLOC(polar_lut_t);
    return_value_if_fail(lut != NULL, NULL);
    
    lut->width = max_width;
    lut->height = max_height;
    lut->center_x = max_width / 2;
    lut->center_y = max_height / 2;

    // 分配内存
    size_t table_size = max_width * max_height * sizeof(float);
    lut->angle_table = (float *)TKMEM_ALLOC(table_size);
    lut->distance_table = (float *)TKMEM_ALLOC(table_size);
    
    // 检查内存分配是否成功
    if (lut->angle_table == NULL || lut->distance_table == NULL) {
        TKMEM_FREE(lut->angle_table);
        TKMEM_FREE(lut->distance_table);
        TKMEM_FREE(lut);
        return NULL;
    }

    // 预计算所有坐标的极坐标值
    for (int32_t y = 0; y < max_height; y++)
    {
        for (int32_t x = 0; x < max_width; x++)
        {
            int32_t idx = y * max_width + x;
            float dx = (float)(x - lut->center_x);
            float dy = (float)(y - lut->center_y);

            // 预计算角度和距离
            lut->angle_table[idx] = atan2f(dy, dx); // [-π, π]
            lut->distance_table[idx] = sqrtf(dx * dx + dy * dy);
        }
    }

    return lut;
}

static ret_t build_color_table(arc_gradient_renderer_t *renderer, 
                               color_t start_color, color_t stop_color)
{
    // 初始化渲染器
    if (renderer->color_table == NULL)
    {
        renderer->color_table_size = 360; // 1度精度
        renderer->color_table = (color_t *)TKMEM_ALLOC(renderer->color_table_size * sizeof(color_t));
        return_value_if_fail(renderer->color_table != NULL, RET_OOM);
    }

    // 预计算所有角度的颜色值
    for (uint32_t i = 0; i < renderer->color_table_size; i++)
    {
        float angle_ratio = (float)i / renderer->color_table_size; // [0, 1)
        renderer->color_table[i] = color_interpolate(start_color, stop_color, angle_ratio);
    }

    renderer->cache_valid = TRUE;
    return RET_OK;
}

static ret_t arc_gradient_renderer_init(arc_gradient_renderer_t *renderer, 
                                        int32_t width, int32_t height,
                                        color_t start_color, color_t stop_color)
{
    return_value_if_fail(renderer != NULL, RET_BAD_PARAMS);
    
    // 创建极坐标查询表
    renderer->lut = polar_lut_create(width, height);
    return_value_if_fail(renderer->lut != NULL, RET_FAIL);
    
    // 构建颜色表
    return_value_if_fail(build_color_table(renderer, start_color, stop_color) == RET_OK, RET_FAIL);
    
    return RET_OK;
}

static ret_t arc_gradient_renderer_deinit(arc_gradient_renderer_t *renderer)
{
    return_value_if_fail(renderer != NULL, RET_BAD_PARAMS);
    
    if (renderer->lut) {
        TKMEM_FREE(renderer->lut->angle_table);
        TKMEM_FREE(renderer->lut->distance_table);
        TKMEM_FREE(renderer->lut);
        renderer->lut = NULL;
    }
    
    if (renderer->color_table) {
        TKMEM_FREE(renderer->color_table);
        renderer->color_table = NULL;
    }
    
    renderer->color_table_size = 0;
    renderer->cache_valid = FALSE;
    
    return RET_OK;
}

/* 绘制弧形渐变 */
static ret_t draw_arc_gradient(canvas_t *c, int32_t cx, int32_t cy,
                               int32_t radius, float start_angle, float end_angle,
                               bool_t ant_clock, arc_gradient_renderer_t *renderer,
                               float full_ratio)
{
    return_value_if_fail(c != NULL && renderer != NULL && renderer->lut != NULL, RET_BAD_PARAMS);
    
    polar_lut_t *lut = renderer->lut;
    
    // 确保角度在合理范围内
    while (start_angle < 0) start_angle += 2 * M_PI;
    while (end_angle < 0) end_angle += 2 * M_PI;
    while (start_angle >= 2 * M_PI) start_angle -= 2 * M_PI;
    while (end_angle >= 2 * M_PI) end_angle -= 2 * M_PI;
    
    // 处理逆时针情况
    float angle_range = 0;
    if (ant_clock) {
        if (start_angle <= end_angle) {
            angle_range = (2 * M_PI - end_angle) + start_angle;
        } else {
            angle_range = start_angle - end_angle;
        }
    } else {
        if (end_angle <= start_angle) {
            angle_range = (2 * M_PI - start_angle) + end_angle;
        } else {
            angle_range = end_angle - start_angle;
        }
    }
    
    // 计算内圆半径（用于绘制弧环）
    int32_t inner_radius = (int32_t)(radius * (1.0f - full_ratio));
    
    // 遍历弧形区域内的像素
    for (int32_t rel_y = -radius; rel_y <= radius; rel_y++)
    {
        for (int32_t rel_x = -radius; rel_x <= radius; rel_x++)
        {
            float distance = polar_lut_get_distance(lut, rel_x, rel_y);

            // 检查是否在圆环范围内
            if (distance <= radius && distance >= inner_radius)
            {
                // 使用查询表获取角度（避免实时atan2计算）
                float angle = polar_lut_get_angle(lut, rel_x, rel_y);

                // 将角度归一化到 [0, 2π)
                if (angle < 0)
                    angle += 2 * M_PI;
                
                // 注意：AWTK坐标系统中Y轴向下，所以角度需要调整
                // 将角度转换为符合AWTK坐标系统的角度
                // atan2返回的角度是以X轴正方向为0度，逆时针为正
                // 在AWTK中，我们希望0度在正上方（Y轴负方向）
                float adjusted_angle = angle - M_PI_2; // 减去90度，使0度指向正上方
                if (adjusted_angle < 0) adjusted_angle += 2 * M_PI;
                
                // 处理角度范围检查
                bool_t in_arc = FALSE;
                if (ant_clock) {
                    // 逆时针方向
                    if (start_angle <= end_angle) {
                        in_arc = (adjusted_angle >= start_angle && adjusted_angle <= end_angle);
                    } else {
                        in_arc = (adjusted_angle >= start_angle || adjusted_angle <= end_angle);
                    }
                } else {
                    // 顺时针方向
                    if (start_angle <= end_angle) {
                        in_arc = (adjusted_angle >= start_angle && adjusted_angle <= end_angle);
                    } else {
                        in_arc = (adjusted_angle >= start_angle || adjusted_angle <= end_angle);
                    }
                }

                if (in_arc) {
                    // 计算在弧形中的位置比例
                    float position_ratio = 0;
                    if (ant_clock) {
                        if (start_angle <= end_angle) {
                            if (adjusted_angle >= start_angle && adjusted_angle <= end_angle) {
                                position_ratio = (adjusted_angle - start_angle) / angle_range;
                            }
                        } else {
                            if (adjusted_angle >= start_angle) {
                                position_ratio = (adjusted_angle - start_angle) / angle_range;
                            } else if (adjusted_angle <= end_angle) {
                                position_ratio = ((2 * M_PI - start_angle) + adjusted_angle) / angle_range;
                            }
                        }
                    } else {
                        if (start_angle <= end_angle) {
                            if (adjusted_angle >= start_angle && adjusted_angle <= end_angle) {
                                position_ratio = (adjusted_angle - start_angle) / angle_range;
                            }
                        } else {
                            if (adjusted_angle >= start_angle) {
                                position_ratio = (adjusted_angle - start_angle) / angle_range;
                            } else if (adjusted_angle <= end_angle) {
                                position_ratio = ((2 * M_PI - start_angle) + adjusted_angle) / angle_range;
                            }
                        }
                    }
                    
                    // 确保比例在[0,1]范围内
                    if (position_ratio < 0.0f) position_ratio = 0.0f;
                    if (position_ratio > 1.0f) position_ratio = 1.0f;
                    
                    // 查找预计算的颜色
                    uint32_t angle_index = (uint32_t)(position_ratio * (renderer->color_table_size - 1));
                    color_t pixel_color = renderer->color_table[angle_index];

                    // 绘制像素
                    canvas_set_fill_color(c, pixel_color);
                    canvas_fill_rect(c, cx + rel_x, cy + rel_y, 1, 1);
                }
            }
        }
    }

    return RET_OK;
}

/* 将角度转换为弧度 */
static inline float degrees_to_radians(float degrees) {
    return degrees * M_PI / 180.0f;
}

/* 绘制弧形渐变 */
static ret_t conner_gradient_view_on_paint_self(widget_t *widget, canvas_t *c)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    int32_t cx = widget->w / 2;
    int32_t cy = widget->h / 2;
    int32_t radius = tk_min(cx, cy) - 2;
    // 将角度转换为弧度
    float_t start_angle = degrees_to_radians(view->start_angle);
    float_t stop_angle = degrees_to_radians(view->stop_angle);
    float_t end_angle = stop_angle;

    /* 根据当前值计算实际结束角度 */
    if (view->max > 0)
    {
        float_t progress = (float_t)view->current / (float_t)view->max;
        end_angle = start_angle + (stop_angle - start_angle) * progress;
    }

    /* 根据平台选择合适的绘制函数 */
    ret_t ret = RET_NOT_IMPL;
    
#ifdef __ARM_NEON__
    ret = draw_arc_gradient_neon(c, cx, cy, radius, start_angle, end_angle, 
                                 view->ant_clock, &view->arc_gradient_renderer, view->full_ratio);
#endif
    
    // 如果没有NEON优化或NEON优化失败，则使用普通版本
    if (ret == RET_NOT_IMPL) {
        ret = draw_arc_gradient(c, cx, cy, radius, start_angle, end_angle, 
                                view->ant_clock, &view->arc_gradient_renderer, view->full_ratio);
    }
    
    return ret;
}

/* 设置属性 */
static ret_t conner_gradient_view_set_prop(widget_t *widget, const char *name, const value_t *v)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_START_ANGLE))
    {
        view->start_angle = value_float(v);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_STOP_ANGLE))
    {
        view->stop_angle = value_float(v);
        build_color_table(&view->arc_gradient_renderer, view->start_color, view->stop_color);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_ANT_CLOCK))
    {
        view->ant_clock = value_bool(v);
        build_color_table(&view->arc_gradient_renderer, view->start_color, view->stop_color);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_MAX))
    {
        view->max = value_int(v);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_CURRENT))
    {
        view->current = value_int(v);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_START_COLOR))
    {
        view->start_color = color_parse(value_str(v));
        // 重新初始化颜色表
        build_color_table(&view->arc_gradient_renderer, view->start_color, view->stop_color);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_STOP_COLOR))
    {
        view->stop_color = color_parse(value_str(v));
        // 重新初始化颜色表
        build_color_table(&view->arc_gradient_renderer, view->start_color, view->stop_color);
        return RET_OK;
    }
    else if (tk_str_eq(name, "full_ratio"))
    {
        view->full_ratio = value_float(v);
        return RET_OK;
    }

    return RET_NOT_FOUND;
}

/* 获取属性 */
static ret_t conner_gradient_view_get_prop(widget_t *widget, const char *name, value_t *v)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_START_ANGLE))
    {
        value_set_float(v, view->start_angle);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_STOP_ANGLE))
    {
        value_set_float(v, view->stop_angle);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_ANT_CLOCK))
    {
        value_set_bool(v, view->ant_clock);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_MAX))
    {
        value_set_int(v, view->max);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_CURRENT))
    {
        value_set_int(v, view->current);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_START_COLOR))
    {
        char color_str[16];
        snprintf(color_str, sizeof(color_str), "#%02X%02X%02X%02X",
                view->start_color.rgba.r, view->start_color.rgba.g,
                view->start_color.rgba.b, view->start_color.rgba.a);
        value_set_str(v, color_str);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_STOP_COLOR))
    {
        char color_str[16];
        snprintf(color_str, sizeof(color_str), "#%02X%02X%02X%02X",
                view->stop_color.rgba.r, view->stop_color.rgba.g,
                view->stop_color.rgba.b, view->stop_color.rgba.a);
        value_set_str(v, color_str);
        return RET_OK;
    }
    else if (tk_str_eq(name, "full_ratio"))
    {
        value_set_float(v, view->full_ratio);
        return RET_OK;
    }

    return RET_NOT_FOUND;
}

/* 销毁控件 */
static ret_t conner_gradient_view_on_destroy(widget_t *widget)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);
    
    // 释放弧形渐变渲染器资源
    arc_gradient_renderer_deinit(&view->arc_gradient_renderer);
    
    return RET_OK;
}

/* 控件虚表 */
static const widget_vtable_t s_conner_gradient_view_vtable = {
    .size = sizeof(conner_gradient_view_t),
    .type = WIDGET_TYPE_CONNER_GRADIENT_VIEW,
    .create = conner_gradient_view_create,
    .set_prop = conner_gradient_view_set_prop,
    .get_prop = conner_gradient_view_get_prop,
    .on_paint_self = conner_gradient_view_on_paint_self,
    .on_destroy = conner_gradient_view_on_destroy};

/* 创建控件 */
widget_t *conner_gradient_view_create(widget_t *parent, xy_t x, xy_t y, wh_t w, wh_t h)
{
    widget_t *widget = widget_create(parent, &s_conner_gradient_view_vtable, x, y, w, h);
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, NULL);

    /* 初始化默认值 */
    view->start_angle = 0;
    view->stop_angle = 2 * M_PI;
    view->ant_clock = FALSE;
    view->max = 100;
    view->current = 50;
    view->start_color = color_init(255, 0, 0, 255);    // 红色
    view->stop_color = color_init(0, 0, 255, 255);     // 蓝色
    view->full_ratio = 1.0f;  // 默认绘制整个扇形

    /* 初始化弧形渐变渲染器 */
    ret_t ret = arc_gradient_renderer_init(&view->arc_gradient_renderer, 
                                           w, h,
                                           view->start_color, 
                                           view->stop_color);
    if (ret != RET_OK) {
        widget_destroy(widget);
        return NULL;
    }

    return widget;
}

/* 类型转换 */
widget_t *conner_gradient_view_cast(widget_t *widget)
{
    return_value_if_fail(widget != NULL && widget->vt == &s_conner_gradient_view_vtable, NULL);
    return widget;
}

/* 设置角度范围 */
ret_t conner_gradient_view_set_angles(widget_t *widget, float_t start_angle, float_t stop_angle)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    view->start_angle = start_angle;
    view->stop_angle = stop_angle;

    widget_invalidate(widget, NULL);
    return RET_OK;
}

/* 设置方向 */
ret_t conner_gradient_view_set_direction(widget_t *widget, bool_t ant_clock)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    view->ant_clock = ant_clock;
    widget_invalidate(widget, NULL);
    return RET_OK;
}

/* 设置最大值 */
ret_t conner_gradient_view_set_max(widget_t *widget, int32_t max)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    view->max = max;

    widget_invalidate(widget, NULL);
    return RET_OK;
}

/* 设置当前值 */
ret_t conner_gradient_view_set_current(widget_t *widget, int32_t current)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    view->current = current;
    widget_invalidate(widget, NULL);
    return RET_OK;
}
